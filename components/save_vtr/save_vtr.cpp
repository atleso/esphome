#include "save_vtr.h"
#include "esphome/core/log.h"

#include <cinttypes>
#include <cstring>

namespace esphome {
namespace save_vtr {

static const char *const TAG = "save_vtr";

// ---------------------------------------------------------------------------
// Modbus register map (Systemair SAVE VTR300).
//
// Register classification (holding vs input) follows the addresses already
// committed in this repository (main + PR #5). Registers marked "extension"
// come from the Systemair SAVE Modbus specification and are NOT verified
// against a physical unit yet - see README before using them.
// ---------------------------------------------------------------------------

// Holding registers (FC3 read / FC6 write)
static constexpr uint16_t REG_HOLIDAY_TIME = 1100;     // extension: user mode duration (days)
static constexpr uint16_t REG_AWAY_TIME = 1101;        // extension: user mode duration (hours)
static constexpr uint16_t REG_FIREPLACE_TIME = 1102;   // extension: user mode duration (minutes)
static constexpr uint16_t REG_REFRESH_TIME = 1103;    // extension: user mode duration (minutes)
static constexpr uint16_t REG_CROWDED_TIME = 1104;     // extension: user mode duration (hours)
static constexpr uint16_t REG_USERMODE_REQ = 1161;      // user mode request (write)
static constexpr uint16_t REG_SETPOINT = 2000;         // supply air setpoint (degC * 10, read/write)
static constexpr uint16_t REG_OUTDOOR_TEMP = 12101;     // outdoor air temperature (degC * 10)
static constexpr uint16_t REG_SUPPLY_TEMP = 12102;     // supply air temperature (degC * 10)
static constexpr uint16_t REG_SENSOR_RHS_PDM = 12135;  // PDM RHS sensor (%, PR #5)
static constexpr uint16_t REG_ECO_MODE = 2505;         // extension: eco function on/off
static constexpr uint16_t REG_MOISTURE_TRANSFER = 3001;  // extension: moisture transfer on/off
static constexpr uint16_t REG_FILTER_PERIOD = 7000;    // extension: filter replacement period (months)
static constexpr uint16_t REG_EXTRACT_TEMP = 12543;     // extract air temperature (degC * 10)

// Input registers (FC4 read)
static constexpr uint16_t REG_USERMODE = 1160;             // current user mode
static constexpr uint16_t REG_HEAT_DEMAND = 2148;           // heat demand (%)
static constexpr uint16_t REG_SENSOR_RPM_SAF = 12400;       // supply air fan RPM
static constexpr uint16_t REG_SENSOR_RPM_EAF = 12401;       // extract air fan RPM
static constexpr uint16_t REG_AIRFLOW_SAF = 14000;          // supply airflow (%)
static constexpr uint16_t REG_AIRFLOW_EAF = 14001;          // extract airflow (%)
static constexpr uint16_t REG_UTIL_HEATER = 14100;          // extension: heater utilisation (Y0, %)
static constexpr uint16_t REG_UTIL_COOLER = 14101;          // extension: cooler utilisation (Y1, %)
static constexpr uint16_t REG_UTIL_HEAT_EXCHANGER = 14102;  // heat exchanger utilisation (Y2, %)
static constexpr uint16_t REG_FILTER_ALARM = 7006;          // filter alarm was detected (PR #5)
static constexpr uint16_t REG_OUTPUT_ALARM = 14002;         // output alarm (PR #5)
static constexpr uint16_t REG_ALARM_TYPE_A = 15900;         // alarm type A (PR #5)
static constexpr uint16_t REG_ALARM_FILTER_WARNING_COUNTER = 15548;  // filter warning counter (PR #5)

/// Detail alarm registers, 0-3 state each. Order/index MUST match
/// ALARM_TEXT_SENSORS in text_sensor.py.
static constexpr uint16_t ALARM_TEXT_REGS[ALARM_TEXT_SENSOR_COUNT] = {
  15001, 15008, 15015, 15022, 15029, 15036, 15057, 15064,
  15071, 15078, 15085, 15092, 15099, 15106, 15113, 15120,
  15127, 15134, 15141, 15148, 15155, 15162, 15169, 15176,
  15183, 15501, 15508, 15515, 15522, 15529, 15536, 15543,
};

/// Summary alarm flags, 0-1 each. Order/index MUST match
/// ALARM_BINARY_SENSORS in binary_sensor.py.
static constexpr uint16_t ALARM_BINARY_REGS[ALARM_BINARY_SENSOR_COUNT] = {
  REG_FILTER_ALARM, REG_OUTPUT_ALARM, REG_ALARM_TYPE_A, 15901, 15902,
};

static const char *const ALARM_TEXT_NAMES[ALARM_TEXT_SENSOR_COUNT] = {
  "SAF control",   "EAF control",   "Frost protection", "Defrosting",
  "SAF RPM",       "EAF RPM",       "Air frost protection (FPT)", "OAT sensor",
  "SAT sensor",    "RAT sensor",     "EAT sensor",       "ECT sensor",
  "EFT sensor",    "OHT sensor",     "EMT sensor",        "RGS sensor",
  "BYS sensor",    "Secondary air",  "Filter",           "Extra controller",
  "External stop", "RH sensor",      "CO2 sensor",       "Low SAT",
  "BYF sensor",    "Manual override outputs", "PDM RHS sensor", "PDM EAT sensor",
  "Manual fan stop", "Overheat temperature", "Fire alarm", "Filter warning",
};

static const char *const ALARM_BINARY_NAMES[ALARM_BINARY_SENSOR_COUNT] = {
  "Filter alarm was detected", "Output alarm", "Alarm type A", "Alarm type B", "Alarm type C",
};

/// User mode names shared by the climate custom fan modes, the user_mode
/// select and register 1160/1161. Index == register value.
/// COOKERHOOD (7) is read-only on Modbus.
static const char *const USER_MODE_NAMES[8] = {
  "AUTO-VENT", "MANUAL", "CROWDED", "REFRESH", "FIREPLACE", "AWAY", "HOLIDAY", "COOKERHOOD",
};
static constexpr uint16_t USER_MODE_WRITE_MAX = 6;

/// Alarm register states (Systemair SAVE alarm state machine)
static const char *alarm_state_string(uint16_t value) {
  switch (value) {
    case 0: return "Inactive";
    case 1: return "Active";
    case 2: return "Waiting";
    case 3: return "Cleared Error Active";
    default: return "Unknown";
  }
}

/// Minimum spacing between change-triggered alarm detail scans.
static constexpr uint32_t MIN_ALARM_RESCAN_MS = 30000;

// ---------------------------------------------------------------------------
// Setup / traits / control
// ---------------------------------------------------------------------------

void SaveVTRClimate::set_modbus(modbus_controller::ModbusController *modbus) {
  this->modbus_ = modbus;
  if (modbus != nullptr)
    this->set_controller(modbus);
}

void SaveVTRClimate::setup() {
  // Custom fan modes live on the Climate entity since ESPHome 2026.4
  // (ClimateTraits::set_supported_custom_fan_modes is deprecated).
  this->set_supported_custom_fan_modes(
      std::vector<const char *>(USER_MODE_NAMES, USER_MODE_NAMES + 8));

  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }
  // The unit has no Modbus OFF; never expose OFF internally (matches previous
  // behaviour of this component).
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }
  this->publish_state();

  // Alarms poll on their own, slower cadence (Atle: poll alarms less
  // frequently than the UI values).
  this->set_interval("save_vtr_alarms", this->alarm_update_interval_, [this]() { this->update_alarms(); });
}

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_visual_min_temperature(12.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_target_temperature_step(1.0f);
  traits.set_visual_current_temperature_step(0.1f);
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
  return traits;
}

bool SaveVTRClimate::write_holding_register(uint16_t address, uint16_t value) {
  if (this->modbus_ == nullptr)
    return false;
  ESP_LOGD(TAG, "Write holding register %u = %u", address, value);
  return this->write_single_register(address, value);
}

bool SaveVTRClimate::request_user_mode(uint16_t mode_value) {
  if (mode_value > USER_MODE_WRITE_MAX) {
    ESP_LOGW(TAG, "User mode %u is read-only on Modbus; skipping write", mode_value);
    return false;
  }
  if (!this->write_holding_register(REG_USERMODE_REQ, mode_value))
    return false;
  this->publish_user_mode_(mode_value);
  return true;
}

void SaveVTRClimate::trigger_button_action(uint8_t action_id) {
  switch (action_id) {
    case ACTION_ACTIVATE_FIREPLACE:
      this->request_user_mode(4);
      break;
    case ACTION_ACTIVATE_REFRESH:
      this->request_user_mode(3);
      break;
    case ACTION_ACTIVATE_CROWDED:
      this->request_user_mode(2);
      break;
    default:
      ESP_LOGW(TAG, "Unknown button action %u", action_id);
      break;
  }
}

void SaveVTRClimate::publish_user_mode_(uint16_t raw) {
  if (raw >= sizeof(USER_MODE_NAMES) / sizeof(USER_MODE_NAMES[0])) {
    ESP_LOGW(TAG, "Unknown user mode register value %u", raw);
    return;
  }
  const char *name = USER_MODE_NAMES[raw];
  ESP_LOGD(TAG, "User mode: %s (%u)", name, raw);
  this->set_custom_fan_mode_(name);
  this->climate_dirty_ = true;
#ifdef USE_TEXT_SENSOR
  if (this->user_mode_text_sensor_ != nullptr)
    this->user_mode_text_sensor_->publish_state(name);
#endif
#ifdef USE_SELECT
  if (this->user_mode_select_ != nullptr) {
    auto index = this->user_mode_select_->index_of(name);
    if (index.has_value()) {
      this->user_mode_select_->publish_state(name);
    } else {
      ESP_LOGD(TAG, "User mode %s not in select options; not published", name);
    }
  }
#endif
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (this->modbus_ == nullptr)
    return;

  if (call.get_mode().has_value()) {
    auto m = *call.get_mode();
    if (m == climate::CLIMATE_MODE_OFF) {
      ESP_LOGW(TAG, "OFF mode not supported; staying in HEAT");
      m = climate::CLIMATE_MODE_HEAT;
    }
    this->mode = m;
    this->climate_dirty_ = true;
  }

  if (call.get_target_temperature().has_value()) {
    // Clamp to the visual range and write degC*10, rounded (the previous
    // code truncated: 20.9 -> 208 instead of 209).
    float temp = *call.get_target_temperature();
    temp = clamp(temp, this->traits().get_visual_min_temperature(),
                 this->traits().get_visual_max_temperature());
    const uint16_t raw = static_cast<uint16_t>(lroundf(temp * 10.0f));
    ESP_LOGI(TAG, "Set target temperature %.1f (raw %u)", temp, raw);
    if (this->write_holding_register(REG_SETPOINT, raw)) {
      this->target_temperature = temp;
      this->climate_dirty_ = true;
    }
  }

  if (call.has_custom_fan_mode()) {
    const std::string mode = call.get_custom_fan_mode().str();
    ESP_LOGI(TAG, "Requested user mode change to %s", mode.c_str());
    uint16_t value = 0xFFFF;
    for (uint16_t i = 0; i < 8; i++) {
      if (mode == USER_MODE_NAMES[i]) {
        value = i;
        break;
      }
    }
    if (value == 0xFFFF) {
      ESP_LOGW(TAG, "Unsupported user mode '%s'; skipping", mode.c_str());
    } else {
      this->request_user_mode(value);
    }
  }

  if (this->climate_dirty_) {
    this->climate_dirty_ = false;
    this->publish_state();
  }
}

// ---------------------------------------------------------------------------
// Polling
// ---------------------------------------------------------------------------

bool SaveVTRClimate::queue_read_registers_(modbus::EntityType entity_type, uint16_t start_address,
                                            uint16_t count) {
  const bool ok = this->read_entities(entity_type, start_address, count);
  if (ok) {
    this->pending_reads_++;
  } else {
    ESP_LOGD(TAG, "Modbus hub refused read type=%" PRIu32 " start=%u count=%u",
             static_cast<uint32_t>(entity_type), start_address, count);
  }
  return ok;
}

void SaveVTRClimate::update() {
  if (this->modbus_ == nullptr)
    return;
  // Never stack a new cycle on top of one that is still in flight: values
  // publish as they arrive, so there is nothing a second cycle would add.
  if (this->pending_reads_ != 0) {
    ESP_LOGV(TAG, "Previous cycle still in flight; skipping poll");
    return;
  }
  // modbus_controller only probes an offline device through its own
  // registered ranges, and this component registers none - so probe here,
  // or the device never comes back online after an outage.
  if (this->modbus_->get_module_offline()) {
    ESP_LOGV(TAG, "Module offline; probing");
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_SETPOINT, 1);
    return;
  }

  ESP_LOGV(TAG, "Polling UI registers");

  // --- Holding registers ---
  this->queue_read_registers_(modbus::EntityType::HOLDING, REG_SETPOINT, 1);
  this->queue_read_registers_(modbus::EntityType::HOLDING, REG_OUTDOOR_TEMP, 2);
  this->queue_read_registers_(modbus::EntityType::HOLDING, REG_EXTRACT_TEMP, 1);

  bool want_eco = false;
  bool want_moisture = false;
  bool want_filter_period = false;
  bool want_durations = false;
  bool want_utils = false;
#ifdef USE_SWITCH
  want_moisture = this->moisture_transfer_switch_ != nullptr;
#endif
#ifdef USE_SENSOR
  want_utils = want_utils || this->heater_utilisation_sensor_ != nullptr ||
               this->cooler_utilisation_sensor_ != nullptr ||
               this->heat_exchanger_utilisation_sensor_ != nullptr;
#endif
#ifdef USE_BINARY_SENSOR
  want_utils = want_utils || this->heater_active_binary_sensor_ != nullptr ||
               this->heat_exchanger_active_binary_sensor_ != nullptr;
#endif
#ifdef USE_SWITCH
  want_eco = this->eco_mode_switch_ != nullptr;
#endif
#ifdef USE_BINARY_SENSOR
  want_eco = want_eco || this->eco_function_active_binary_sensor_ != nullptr;
#endif
#ifdef USE_SENSOR
  if (this->pdm_rhs_sensor_ != nullptr)
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_SENSOR_RHS_PDM, 1);
#endif
  if (want_eco)
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_ECO_MODE, 1);
  if (want_moisture)
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_MOISTURE_TRANSFER, 1);
#ifdef USE_NUMBER
  want_filter_period = this->filter_period_number_ != nullptr;
  want_durations = this->holiday_time_number_ != nullptr || this->away_time_number_ != nullptr ||
                   this->fireplace_time_number_ != nullptr || this->refresh_time_number_ != nullptr ||
                   this->crowded_time_number_ != nullptr;
#endif
  if (want_filter_period)
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_FILTER_PERIOD, 1);
  if (want_durations)
    this->queue_read_registers_(modbus::EntityType::HOLDING, REG_HOLIDAY_TIME, 5);

  // --- Input registers ---
  this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_USERMODE, 1);
  this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_HEAT_DEMAND, 1);
  this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_SENSOR_RPM_SAF, 2);
  this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_AIRFLOW_SAF, 2);
  if (want_utils)
    this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_UTIL_HEATER, 3);

  // Summary alarm flags feed the correlation triggers every cycle.
  if (this->has_any_alarm_entity_()) {
    for (size_t i = 0; i < ALARM_BINARY_SENSOR_COUNT; i++) {
      const size_t chunk = (i == 2) ? 3 : 1;  // type A/B/C are contiguous
      if (i == 2) {
        this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, ALARM_BINARY_REGS[i], chunk);
      } else {
        this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, ALARM_BINARY_REGS[i], chunk);
      }
    }
  }
}

void SaveVTRClimate::update_alarms() {
  if (this->modbus_ == nullptr || !this->needs_alarm_detail_scan_())
    return;
  if (this->modbus_->get_module_offline())
    return;
  this->alarm_scan_done_ = true;
  this->last_alarm_scan_ms_ = millis();
  ESP_LOGV(TAG, "Alarm detail scan (%u registers)", static_cast<unsigned>(ALARM_TEXT_SENSOR_COUNT));
  for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++)
    this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, ALARM_TEXT_REGS[i], 1);
#ifdef USE_SENSOR
  if (this->alarm_filter_warning_counter_sensor_ != nullptr)
    this->queue_read_registers_(modbus::EntityType::INPUT_REGISTER, REG_ALARM_FILTER_WARNING_COUNTER, 1);
#endif
}

// ---------------------------------------------------------------------------
// ControllerDevice response routing
// ---------------------------------------------------------------------------

void SaveVTRClimate::on_response(std::span<const uint8_t> request_pdu, std::span<const uint8_t> response_pdu) {
  this->ControllerDevice::on_response(request_pdu, response_pdu);
  this->dispatch_response_(request_pdu, response_pdu, std::nullopt);
}

void SaveVTRClimate::on_error(std::span<const uint8_t> request_pdu, modbus::ExceptionCode exception_code) {
  this->ControllerDevice::on_error(request_pdu, exception_code);
  this->dispatch_response_(request_pdu, {}, exception_code);
}

void SaveVTRClimate::on_read_registers(modbus::EntityType entity_type, uint16_t start_address,
                                       std::span<const uint16_t> registers, modbus::ResponseStatus status) {
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "Modbus read failed: type=%u start=%u exception=%u",
             static_cast<unsigned>(entity_type), start_address,
             status.has_value() ? static_cast<unsigned>(status.value()) : 0u);
    this->complete_read_request_();
    return;
  }
  if (!registers.empty())
    this->handle_registers_read_(entity_type, start_address, registers);
  this->complete_read_request_();
}

bool SaveVTRClimate::on_no_response(std::span<const uint8_t> request_pdu) {
  const bool retry = this->ControllerDevice::on_no_response(request_pdu);
  // Writes are not counted in pending_reads_; only complete reads.
  const bool is_read = !request_pdu.empty() && modbus::helpers::is_function_code_read(request_pdu[0]);
  if (!retry && is_read)
    this->complete_read_request_();
  return retry;
}

void SaveVTRClimate::on_not_sent(std::span<const uint8_t> request_pdu) {
  this->ControllerDevice::on_not_sent(request_pdu);
  const bool is_read = !request_pdu.empty() && modbus::helpers::is_function_code_read(request_pdu[0]);
  if (is_read)
    this->complete_read_request_();
}

void SaveVTRClimate::on_write_single_register(uint16_t address, uint16_t value, modbus::ResponseStatus status) {
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "Write register %u failed (exception %u)", address,
             status.has_value() ? static_cast<unsigned>(status.value()) : 0u);
    return;
  }
  ESP_LOGD(TAG, "Write register %u acknowledged, echo=%u", address, value);
  if (address == REG_SETPOINT) {
    this->target_temperature = value / 10.0f;
    this->climate_dirty_ = false;
    this->publish_state();
  }
}

void SaveVTRClimate::complete_read_request_() {
  if (this->pending_reads_ == 0)
    return;
  this->pending_reads_--;
  if (this->pending_reads_ == 0)
    this->finalize_cycle_();
}

void SaveVTRClimate::finalize_cycle_() {
  this->update_climate_action_();
  this->update_thermal_efficiency_();
  this->correlate_alarms_();
  if (this->climate_dirty_) {
    this->climate_dirty_ = false;
    this->publish_state();
  }

  // A summary alarm flag changed: rescan the detail registers immediately
  // (rate-limited), so a new alarm is described within one cycle instead of
  // waiting for the slow alarm interval.
  if (this->needs_alarm_detail_scan_() && !this->alarm_scan_done_) {
    ESP_LOGD(TAG, "Initial alarm detail scan");
    this->update_alarms();
  } else if (this->alarm_rescan_requested_ && this->needs_alarm_detail_scan_() &&
             millis() - this->last_alarm_scan_ms_ >= MIN_ALARM_RESCAN_MS) {
    ESP_LOGD(TAG, "Alarm flag changed; immediate detail rescan");
    this->alarm_rescan_requested_ = false;
    this->update_alarms();
  }
}

// ---------------------------------------------------------------------------
// Register decode
// ---------------------------------------------------------------------------

void SaveVTRClimate::handle_registers_read_(modbus::EntityType entity_type, uint16_t start_address,
                                             std::span<const uint16_t> registers) {
  const auto value = [&](size_t index) -> uint16_t {
    return index < registers.size() ? registers[index] : 0;
  };

  if (entity_type == modbus::EntityType::HOLDING) {
    switch (start_address) {
      case REG_SETPOINT: {
        this->target_temperature = value(0) / 10.0f;
        this->climate_dirty_ = true;
        break;
      }
      case REG_OUTDOOR_TEMP: {
        this->outdoor_air_temp_ = value(0) / 10.0f;
        this->supply_air_temp_ = value(1) / 10.0f;
        this->current_temperature = this->supply_air_temp_;
        this->climate_dirty_ = true;
        ESP_LOGD(TAG, "Outdoor %.1f, supply %.1f", this->outdoor_air_temp_, this->supply_air_temp_);
#ifdef USE_SENSOR
        if (this->outdoor_air_temp_sensor_ != nullptr)
          this->outdoor_air_temp_sensor_->publish_state(this->outdoor_air_temp_);
        if (this->supply_air_temp_sensor_ != nullptr)
          this->supply_air_temp_sensor_->publish_state(this->supply_air_temp_);
#endif
        this->update_thermal_efficiency_();
        break;
      }
      case REG_EXTRACT_TEMP: {
        this->extract_air_temp_ = value(0) / 10.0f;
        ESP_LOGD(TAG, "Extract %.1f", this->extract_air_temp_);
#ifdef USE_SENSOR
        if (this->extract_air_temp_sensor_ != nullptr)
          this->extract_air_temp_sensor_->publish_state(this->extract_air_temp_);
#endif
        this->update_thermal_efficiency_();
        break;
      }
      case REG_SENSOR_RHS_PDM: {
        this->pdm_rhs_ = static_cast<float>(value(0));
        this->current_humidity = this->pdm_rhs_;
        this->climate_dirty_ = true;
#ifdef USE_SENSOR
        if (this->pdm_rhs_sensor_ != nullptr)
          this->pdm_rhs_sensor_->publish_state(this->pdm_rhs_);
#endif
        break;
      }
      case REG_ECO_MODE: {
        const bool eco_on = value(0) != 0;
        ESP_LOGD(TAG, "Eco mode %s", eco_on ? "ON" : "OFF");
#ifdef USE_BINARY_SENSOR
        if (this->eco_function_active_binary_sensor_ != nullptr)
          this->eco_function_active_binary_sensor_->publish_state(eco_on);
#endif
#ifdef USE_SWITCH
        if (this->eco_mode_switch_ != nullptr)
          this->eco_mode_switch_->publish_state(eco_on);
#endif
        break;
      }
      case REG_MOISTURE_TRANSFER: {
#ifdef USE_SWITCH
        if (this->moisture_transfer_switch_ != nullptr)
          this->moisture_transfer_switch_->publish_state(value(0) != 0);
#endif
        break;
      }
      case REG_FILTER_PERIOD: {
#ifdef USE_NUMBER
        if (this->filter_period_number_ != nullptr)
          this->filter_period_number_->publish_state(static_cast<float>(value(0)));
#endif
        break;
      }
      case REG_HOLIDAY_TIME: {
#ifdef USE_NUMBER
        if (this->holiday_time_number_ != nullptr)
          this->holiday_time_number_->publish_state(static_cast<float>(value(0)));
        if (this->away_time_number_ != nullptr)
          this->away_time_number_->publish_state(static_cast<float>(value(1)));
        if (this->fireplace_time_number_ != nullptr)
          this->fireplace_time_number_->publish_state(static_cast<float>(value(2)));
        if (this->refresh_time_number_ != nullptr)
          this->refresh_time_number_->publish_state(static_cast<float>(value(3)));
        if (this->crowded_time_number_ != nullptr)
          this->crowded_time_number_->publish_state(static_cast<float>(value(4)));
#endif
        break;
      }
      default:
        ESP_LOGV(TAG, "Unmapped holding register block %u", start_address);
        break;
    }
    return;
  }

  if (entity_type == modbus::EntityType::INPUT_REGISTER) {
    switch (start_address) {
      case REG_USERMODE:
        this->publish_user_mode_(value(0));
        break;
      case REG_HEAT_DEMAND: {
        this->heat_demand_percent_ = static_cast<float>(value(0));
        ESP_LOGD(TAG, "Heat demand %.0f%%", this->heat_demand_percent_);
#ifdef USE_SENSOR
        if (this->heat_demand_sensor_ != nullptr)
          this->heat_demand_sensor_->publish_state(this->heat_demand_percent_);
#endif
        this->update_climate_action_();
        break;
      }
      case REG_SENSOR_RPM_SAF: {
        this->rpm_saf_ = static_cast<float>(value(0));
        this->rpm_eaf_ = static_cast<float>(value(1));
#ifdef USE_SENSOR
        if (this->rpm_saf_sensor_ != nullptr)
          this->rpm_saf_sensor_->publish_state(this->rpm_saf_);
        if (this->rpm_eaf_sensor_ != nullptr)
          this->rpm_eaf_sensor_->publish_state(this->rpm_eaf_);
#endif
        break;
      }
      case REG_AIRFLOW_SAF: {
        this->saf_percent_ = static_cast<float>(value(0));
        this->eaf_percent_ = static_cast<float>(value(1));
        this->saf_volume_ = this->saf_percent_ / 100.0f * this->max_airflow_;
        this->eaf_volume_ = this->eaf_percent_ / 100.0f * this->max_airflow_;
        ESP_LOGD(TAG, "Airflow SAF %.0f%% (%.1f m3/h), EAF %.0f%% (%.1f m3/h)", this->saf_percent_,
                 this->saf_volume_, this->eaf_percent_, this->eaf_volume_);
#ifdef USE_SENSOR
        if (this->saf_percent_sensor_ != nullptr)
          this->saf_percent_sensor_->publish_state(this->saf_percent_);
        if (this->saf_volume_sensor_ != nullptr)
          this->saf_volume_sensor_->publish_state(this->saf_volume_);
        if (this->eaf_percent_sensor_ != nullptr)
          this->eaf_percent_sensor_->publish_state(this->eaf_percent_);
        if (this->eaf_volume_sensor_ != nullptr)
          this->eaf_volume_sensor_->publish_state(this->eaf_volume_);
#endif
        this->update_climate_action_();
        break;
      }
      case REG_UTIL_HEATER: {
        this->heater_utilisation_ = static_cast<float>(value(0));
        this->cooler_utilisation_ = static_cast<float>(value(1));
        this->heat_exchanger_utilisation_ = static_cast<float>(value(2));
#ifdef USE_SENSOR
        if (this->heater_utilisation_sensor_ != nullptr)
          this->heater_utilisation_sensor_->publish_state(this->heater_utilisation_);
        if (this->cooler_utilisation_sensor_ != nullptr)
          this->cooler_utilisation_sensor_->publish_state(this->cooler_utilisation_);
        if (this->heat_exchanger_utilisation_sensor_ != nullptr)
          this->heat_exchanger_utilisation_sensor_->publish_state(this->heat_exchanger_utilisation_);
#endif
#ifdef USE_BINARY_SENSOR
        if (this->heater_active_binary_sensor_ != nullptr)
          this->heater_active_binary_sensor_->publish_state(this->heater_utilisation_ > 0.0f);
        if (this->heat_exchanger_active_binary_sensor_ != nullptr)
          this->heat_exchanger_active_binary_sensor_->publish_state(this->heat_exchanger_utilisation_ > 0.0f);
#endif
        break;
      }
      case REG_ALARM_FILTER_WARNING_COUNTER: {
#ifdef USE_SENSOR
        if (this->alarm_filter_warning_counter_sensor_ != nullptr)
          this->alarm_filter_warning_counter_sensor_->publish_state(static_cast<float>(value(0)));
#endif
        break;
      }
      default: {
        // Summary alarm flags (0-1 registers)
        for (size_t i = 0; i < ALARM_BINARY_SENSOR_COUNT; i++) {
          if (start_address == ALARM_BINARY_REGS[i]) {
            for (size_t j = 0; j < registers.size() && (i + j) < ALARM_BINARY_SENSOR_COUNT; j++)
              this->update_alarm_binary_state_(i + j, registers[j]);
            return;
          }
        }
        // Detail alarm registers (0-3 registers)
        for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++) {
          if (start_address == ALARM_TEXT_REGS[i]) {
            this->update_alarm_text_state_(i, value(0));
            return;
          }
        }
        ESP_LOGV(TAG, "Unmapped input register block %u", start_address);
        break;
      }
    }
    return;
  }
}

// ---------------------------------------------------------------------------
// Correlation
// ---------------------------------------------------------------------------

void SaveVTRClimate::update_climate_action_() {
  climate::ClimateAction action = climate::CLIMATE_ACTION_IDLE;
  if (this->heat_demand_percent_ > 0.5f) {
    action = climate::CLIMATE_ACTION_HEATING;
  } else if (this->saf_percent_ > 0.0f || this->eaf_percent_ > 0.0f) {
    action = climate::CLIMATE_ACTION_FAN;
  }
  if (this->action != action) {
    ESP_LOGD(TAG, "Climate action -> %d", static_cast<int>(action));
    this->action = action;
    this->climate_dirty_ = true;
  }
}

void SaveVTRClimate::update_thermal_efficiency_() {
  if (std::isnan(this->outdoor_air_temp_) || std::isnan(this->extract_air_temp_) ||
      std::isnan(this->supply_air_temp_)) {
    return;
  }
  // Temperature efficiency of the heat exchanger:
  //   eta = (T_supply - T_outdoor) / (T_extract - T_outdoor)
  // Only meaningful with a decent temperature difference across the unit.
  const float denom = this->extract_air_temp_ - this->outdoor_air_temp_;
  if (denom < 2.0f)
    return;
  float eta = (this->supply_air_temp_ - this->outdoor_air_temp_) / denom * 100.0f;
  eta = clamp(eta, 0.0f, 100.0f);
  if (std::isnan(this->thermal_efficiency_) || std::abs(eta - this->thermal_efficiency_) > 0.05f) {
    this->thermal_efficiency_ = eta;
#ifdef USE_SENSOR
    if (this->thermal_efficiency_sensor_ != nullptr)
      this->thermal_efficiency_sensor_->publish_state(eta);
#endif
    ESP_LOGD(TAG, "Heat exchanger temperature efficiency %.1f%%", eta);
  }
}

void SaveVTRClimate::update_alarm_binary_state_(size_t idx, uint16_t raw) {
  if (idx >= ALARM_BINARY_SENSOR_COUNT)
    return;
  const bool state = raw != 0;
  const uint8_t new_value = state ? 1 : 0;
  const bool changed = this->alarm_binary_states_[idx] != new_value;
  this->alarm_binary_states_[idx] = new_value;
#ifdef USE_BINARY_SENSOR
  // Also publish the first reading: the cache starts at 0, so an inactive
  // flag would otherwise never get a state.
  if (this->alarm_binary_sensors_[idx] != nullptr && (changed || !this->alarm_binary_sensors_[idx]->has_state()))
    this->alarm_binary_sensors_[idx]->publish_state(state);
#endif
  if (changed) {
    if (state) {
      ESP_LOGW(TAG, "ALARM %s: ACTIVE", ALARM_BINARY_NAMES[idx]);
    } else {
      ESP_LOGI(TAG, "Alarm %s cleared", ALARM_BINARY_NAMES[idx]);
    }
    // Rescan the detail registers so the summary can name what changed.
    this->alarm_rescan_requested_ = true;
  }
}

void SaveVTRClimate::update_alarm_text_state_(size_t idx, uint16_t raw) {
  if (idx >= ALARM_TEXT_SENSOR_COUNT)
    return;
  const uint8_t previous = this->alarm_text_states_[idx];
  this->alarm_text_states_[idx] = static_cast<uint8_t>(raw > 0xFF ? 0xFF : raw);
  if (raw == 1 && previous != 1) {
    ESP_LOGW(TAG, "ALARM %s: %s", ALARM_TEXT_NAMES[idx], alarm_state_string(raw));
  }
#ifdef USE_TEXT_SENSOR
  if (this->alarm_text_sensors_[idx] != nullptr &&
      (!this->alarm_text_published_[idx] || previous != this->alarm_text_states_[idx])) {
    this->alarm_text_sensors_[idx]->publish_state(alarm_state_string(raw));
    this->alarm_text_published_[idx] = true;
  }
#endif
}

void SaveVTRClimate::correlate_alarms_() {
  int active = 0;
  int waiting = 0;
  bool any_known = false;
  std::string summary{};

  for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++) {
    const uint8_t state = this->alarm_text_states_[i];
    if (state == 0xFF)
      continue;
    any_known = true;
    if (state == 1) {
      active++;
      if (!summary.empty())
        summary += ", ";
      summary += ALARM_TEXT_NAMES[i];
    } else if (state == 2 || state == 3) {
      waiting++;
    }
  }
  for (size_t i = 0; i < ALARM_BINARY_SENSOR_COUNT; i++) {
    const uint8_t state = this->alarm_binary_states_[i];
    if (state == 0xFF)
      continue;
    any_known = true;
    if (state != 0) {
      active++;
      if (!summary.empty())
        summary += ", ";
      summary += ALARM_BINARY_NAMES[i];
    }
  }

  const bool filter_warning = (this->alarm_text_states_[31] == 1) || (this->alarm_binary_states_[0] == 1);
  const std::string status =
      active > 0 ? "Alarm" : (waiting > 0 ? "Warning" : (any_known ? "OK" : "Unknown"));
  const std::string text = active > 0 ? summary : (any_known ? "None" : "Unknown");

  if (text == this->last_alarm_summary_ && status == this->last_system_status_ &&
      active == this->last_active_alarm_count_) {
    return;  // nothing moved; stay quiet on the bus/API/MQTT
  }
  this->last_alarm_summary_ = text;
  this->last_system_status_ = status;
  this->last_active_alarm_count_ = active;

  ESP_LOGI(TAG, "Alarm status: %s, active=%d (%s)", status.c_str(), active, text.c_str());

#ifdef USE_SENSOR
  if (this->active_alarm_count_sensor_ != nullptr)
    this->active_alarm_count_sensor_->publish_state(static_cast<float>(active));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->any_alarm_active_binary_sensor_ != nullptr)
    this->any_alarm_active_binary_sensor_->publish_state(active > 0);
  if (this->filter_warning_active_binary_sensor_ != nullptr)
    this->filter_warning_active_binary_sensor_->publish_state(filter_warning);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->alarm_summary_text_sensor_ != nullptr)
    this->alarm_summary_text_sensor_->publish_state(text);
  if (this->system_status_text_sensor_ != nullptr)
    this->system_status_text_sensor_->publish_state(status);
#endif
}

bool SaveVTRClimate::has_any_alarm_entity_() const {
  bool any = false;
#ifdef USE_BINARY_SENSOR
  for (size_t i = 0; i < ALARM_BINARY_SENSOR_COUNT; i++)
    any = any || this->alarm_binary_sensors_[i] != nullptr;
  any = any || this->any_alarm_active_binary_sensor_ != nullptr ||
        this->filter_warning_active_binary_sensor_ != nullptr;
#endif
#ifdef USE_TEXT_SENSOR
  for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++)
    any = any || this->alarm_text_sensors_[i] != nullptr;
  any = any || this->alarm_summary_text_sensor_ != nullptr ||
        this->system_status_text_sensor_ != nullptr;
#endif
#ifdef USE_SENSOR
  any = any || this->active_alarm_count_sensor_ != nullptr;
#endif
  return any;
}

bool SaveVTRClimate::needs_alarm_detail_scan_() const {
  bool any = false;
#ifdef USE_TEXT_SENSOR
  for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++)
    any = any || this->alarm_text_sensors_[i] != nullptr;
  any = any || this->alarm_summary_text_sensor_ != nullptr ||
        this->system_status_text_sensor_ != nullptr;
#endif
#ifdef USE_BINARY_SENSOR
  any = any || this->filter_warning_active_binary_sensor_ != nullptr;
#endif
#ifdef USE_SENSOR
  any = any || this->alarm_filter_warning_counter_sensor_ != nullptr ||
        this->active_alarm_count_sensor_ != nullptr;
#endif
  return any;
}

// ---------------------------------------------------------------------------
// Child entities
// ---------------------------------------------------------------------------

#ifdef USE_SWITCH
void SaveVTRSwitch::write_state(bool state) {
  ESP_LOGI(TAG, "Switch register %u -> %s", this->reg_, state ? "ON" : "OFF");
  if (this->parent_->write_holding_register(this->reg_, state ? 1 : 0)) {
    this->publish_state(state);  // optimistic; next poll confirms
  }
}
#endif

#ifdef USE_NUMBER
void SaveVTRNumber::control(float value) {
  ESP_LOGI(TAG, "Number register %u -> %.1f", this->reg_, value);
  if (this->parent_->write_holding_register(this->reg_, static_cast<uint16_t>(lroundf(value)))) {
    this->publish_state(value);  // optimistic; next poll confirms
  }
}
#endif

#ifdef USE_SELECT
void SaveVTRUserModeSelect::control(const std::string &value) {
  auto index = this->index_of(value);
  if (!index.has_value()) {
    ESP_LOGW(TAG, "Unknown select option '%s'", value.c_str());
    return;
  }
  if (this->parent_->request_user_mode(static_cast<uint16_t>(*index)))
    this->publish_state(value);
}
#endif

#ifdef USE_BUTTON
void SaveVTRButton::press_action() {
  ESP_LOGI(TAG, "Button action %u", this->action_id_);
  this->parent_->trigger_button_action(this->action_id_);
}
#endif

// ---------------------------------------------------------------------------
// Config dump
// ---------------------------------------------------------------------------

void SaveVTRClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "SAVE VTR (Systemair) climate:");
  LOG_CLIMATE("  ", "SaveVTRClimate", this);
  ESP_LOGCONFIG(TAG, "  Modbus address: %u", this->modbus_ != nullptr ? this->modbus_->device_address() : 0);
  ESP_LOGCONFIG(TAG, "  Max airflow: %.0f m3/h", this->max_airflow_);
  ESP_LOGCONFIG(TAG, "  Alarm detail interval: %" PRIu32 "ms", this->alarm_update_interval_);
  ESP_LOGCONFIG(TAG, "  Alarm summary flags: %u, detail registers: %u",
                static_cast<unsigned>(ALARM_BINARY_SENSOR_COUNT),
                static_cast<unsigned>(ALARM_TEXT_SENSOR_COUNT));
}

}  // namespace save_vtr
}  // namespace esphome