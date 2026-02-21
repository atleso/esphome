#include "save_vtr.h"
#include "esphome/core/log.h"

namespace esphome {
namespace save_vtr {

// Modbus register addresses
static constexpr uint16_t REG_FAN_MODE = 1160;          // Fan mode read (read input register)
static constexpr uint16_t REG_FAN_MODE_REQ = 1161;          // Fan mode request (holding)
static constexpr uint16_t REG_ROOM_TEMP = 2001;         // Room temperature read (holding)
static constexpr uint16_t REG_SETPOINT = 2000;     // Target temperature read/write (holding)
static constexpr uint16_t REG_HEAT_DEMAND = 2148;       // Heat demand percentage read (read)
static constexpr uint16_t REG_OUTDOOR_TEMP = 12101;     // Outdoor air temperature (holding)
static constexpr uint16_t REG_SUPPLY_TEMP = 12102;      // Supply air temperature (holding)
static constexpr uint16_t REG_EXTRACT_TEMP = 12543;     // Extract air temperature (holding)
static constexpr uint16_t REG_SUPPLY_AIRFLOW = 14000;   // Supply air flow volume (read)
static constexpr uint16_t REG_EXTRACT_AIRFLOW = 14001;  // Extract air flow volume (read)
static constexpr uint16_t REG_SENSOR_RPM_SAF = 12400;   // Supply air fan RPM (input register)
static constexpr uint16_t REG_SENSOR_RPM_EAF = 12401;   // Extract air fan RPM (input register)
static constexpr uint16_t REG_OUTPUT_Y2_ANALOG = 14102; // Heat exchanger utilisation 0-100% (read input register)
static constexpr uint16_t REG_SENSOR_RHS_PDM = 12135;   // PDM RHS sensor 0-100 (holding register)

// Alarm register addresses (read input registers)
static constexpr uint16_t REG_FILTER_ALARM_WAS_DETECTED = 7006;   // Boolean
static constexpr uint16_t REG_OUTPUT_ALARM = 14002;                // 0-1
static constexpr uint16_t REG_ALARM_SAF_CTRL = 15001;             // 0-3
static constexpr uint16_t REG_ALARM_EAF_CTRL = 15008;             // 0-3
static constexpr uint16_t REG_ALARM_FROST_PROT = 15015;           // 0-3
static constexpr uint16_t REG_ALARM_DEFROSTING = 15022;           // 0-3
static constexpr uint16_t REG_ALARM_SAF_RPM = 15029;              // 0-3
static constexpr uint16_t REG_ALARM_EAF_RPM = 15036;              // 0-3
static constexpr uint16_t REG_ALARM_FPT = 15057;                  // 0-3
static constexpr uint16_t REG_ALARM_OAT = 15064;                  // 0-3
static constexpr uint16_t REG_ALARM_SAT = 15071;                  // 0-3
static constexpr uint16_t REG_ALARM_RAT = 15078;                  // 0-3
static constexpr uint16_t REG_ALARM_EAT = 15085;                  // 0-3
static constexpr uint16_t REG_ALARM_ECT = 15092;                  // 0-3
static constexpr uint16_t REG_ALARM_EFT = 15099;                  // 0-3
static constexpr uint16_t REG_ALARM_OHT = 15106;                  // 0-3
static constexpr uint16_t REG_ALARM_EMT = 15113;                  // 0-3
static constexpr uint16_t REG_ALARM_RGS = 15120;                  // 0-3
static constexpr uint16_t REG_ALARM_BYS = 15127;                  // 0-3
static constexpr uint16_t REG_ALARM_SECONDARY_AIR = 15134;        // 0-3
static constexpr uint16_t REG_ALARM_FILTER = 15141;               // 0-3
static constexpr uint16_t REG_ALARM_EXTRA_CONTROLLER = 15148;     // 0-3
static constexpr uint16_t REG_ALARM_EXTERNAL_STOP = 15155;        // 0-3
static constexpr uint16_t REG_ALARM_RH = 15162;                   // 0-3
static constexpr uint16_t REG_ALARM_CO2 = 15169;                  // 0-3
static constexpr uint16_t REG_ALARM_LOW_SAT = 15176;              // 0-3
static constexpr uint16_t REG_ALARM_BYF = 15183;                  // 0-3
static constexpr uint16_t REG_ALARM_MANUAL_OVERRIDE_OUTPUTS = 15501; // 0-3
static constexpr uint16_t REG_ALARM_PDM_RHS = 15508;              // 0-3
static constexpr uint16_t REG_ALARM_PDM_EAT = 15515;              // 0-3
static constexpr uint16_t REG_ALARM_MANUAL_FAN_STOP = 15522;      // 0-3
static constexpr uint16_t REG_ALARM_OVERHEAT_TEMPERATURE = 15529; // 0-3
static constexpr uint16_t REG_ALARM_FIRE_ALARM = 15536;           // 0-3
static constexpr uint16_t REG_ALARM_FILTER_WARNING = 15543;       // 0-3
static constexpr uint16_t REG_ALARM_FILTER_WARNING_COUNTER = 15548; // Counter
static constexpr uint16_t REG_ALARM_TYPE_A = 15900;               // 0-1
static constexpr uint16_t REG_ALARM_TYPE_B = 15901;               // 0-1
static constexpr uint16_t REG_ALARM_TYPE_C = 15902;               // 0-1

// Ordered list of 0-3 alarm text sensor registers (must match ALARM_TEXT_SENSORS in text_sensor.py)
static constexpr uint16_t ALARM_TEXT_REGS[ALARM_TEXT_SENSOR_COUNT] = {
  REG_ALARM_SAF_CTRL, REG_ALARM_EAF_CTRL, REG_ALARM_FROST_PROT, REG_ALARM_DEFROSTING,
  REG_ALARM_SAF_RPM, REG_ALARM_EAF_RPM, REG_ALARM_FPT, REG_ALARM_OAT,
  REG_ALARM_SAT, REG_ALARM_RAT, REG_ALARM_EAT, REG_ALARM_ECT,
  REG_ALARM_EFT, REG_ALARM_OHT, REG_ALARM_EMT, REG_ALARM_RGS,
  REG_ALARM_BYS, REG_ALARM_SECONDARY_AIR, REG_ALARM_FILTER, REG_ALARM_EXTRA_CONTROLLER,
  REG_ALARM_EXTERNAL_STOP, REG_ALARM_RH, REG_ALARM_CO2, REG_ALARM_LOW_SAT,
  REG_ALARM_BYF, REG_ALARM_MANUAL_OVERRIDE_OUTPUTS, REG_ALARM_PDM_RHS, REG_ALARM_PDM_EAT,
  REG_ALARM_MANUAL_FAN_STOP, REG_ALARM_OVERHEAT_TEMPERATURE, REG_ALARM_FIRE_ALARM,
  REG_ALARM_FILTER_WARNING,
};

// Ordered list of 0-1 alarm binary sensor registers (must match ALARM_BINARY_SENSORS in binary_sensor.py)
static constexpr uint16_t ALARM_BINARY_REGS[ALARM_BINARY_SENSOR_COUNT] = {
  REG_FILTER_ALARM_WAS_DETECTED, REG_OUTPUT_ALARM,
  REG_ALARM_TYPE_A, REG_ALARM_TYPE_B, REG_ALARM_TYPE_C,
};

static const char *alarm_state_string(uint16_t value) {
  switch (value) {
    case 0: return "Inactive";
    case 1: return "Active";
    case 2: return "Waiting";
    case 3: return "Cleared Error Active";
    default: return "Unknown";
  }
}

void SaveVTRClimate::set_modbus(modbus_controller::ModbusController *modbus) {
  this->modbus_ = modbus;
}

// Ensure we start in HEAT when no state is restored and never expose OFF internally
void SaveVTRClimate::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->mode = climate::CLIMATE_MODE_HEAT;
  }
  this->publish_state();
  this->set_interval("alarms", this->alarm_update_interval_, [this]() { this->update_alarms(); });
}

static const char *const TAG = "save_vtr.climate";

// Helper function to create temperature read command with /10.0 scaling (for signed 16-bit values)
template<typename T>
void create_temperature_read_command(SaveVTRClimate* instance, modbus_controller::ModbusController* modbus,
                                   modbus_controller::ModbusRegisterType register_type, uint16_t address,
                                   T* target_variable, const char* log_name) {
  auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
    modbus, register_type, address, 1,
    [instance, target_variable, log_name](modbus_controller::ModbusRegisterType, uint16_t,
                                        const std::vector<uint8_t> &data) {
      if (data.size() >= 2) {
        int16_t temp_raw = static_cast<int16_t>((data[0] << 8) | data[1]);
        *target_variable = temp_raw / 10.0f;
        ESP_LOGD(TAG, "Read %s: %.1f°C (raw: %d)", log_name, *target_variable, temp_raw);
      } else {
        ESP_LOGE(TAG, "Insufficient data for %s: got %d bytes, expected 2", log_name, data.size());
      }
    }
  );
  modbus->queue_command(cmd);
}

// Helper function to create unsigned 16-bit value read command
template<typename T>
void create_uint16_read_command(SaveVTRClimate* instance, modbus_controller::ModbusController* modbus,
                              modbus_controller::ModbusRegisterType register_type, uint16_t address,
                              T* target_variable, const char* log_name, const char* unit = "") {
  auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
    modbus, register_type, address, 1,
    [instance, target_variable, log_name, unit](modbus_controller::ModbusRegisterType, uint16_t,
                                              const std::vector<uint8_t> &data) {
      if (data.size() >= 2) {
        uint16_t raw_value = (data[0] << 8) | data[1];
        *target_variable = static_cast<float>(raw_value);
        ESP_LOGD(TAG, "Read %s: %.0f%s (raw: %u)", log_name, *target_variable, unit, raw_value);
      } else {
        ESP_LOGE(TAG, "Insufficient data for %s: got %d bytes, expected 2", log_name, data.size());
      }
    }
  );
  modbus->queue_command(cmd);
}

void SaveVTRClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "SaveVTRClimate:");
  LOG_CLIMATE("  ", "SaveVTRClimate", this);
  ESP_LOGCONFIG(TAG, "  Using Modbus for temperature, setpoint, and fan mode control");
  ESP_LOGCONFIG(TAG, "  Heat demand: %.0f%%", this->heat_demand_percent_);
  ESP_LOGCONFIG(TAG, "  Supply Air Flow: %.1f m³/h", this->saf_volume_);
  ESP_LOGCONFIG(TAG, "  Extract Air Flow: %.1f m³/h", this->eaf_volume_);
  ESP_LOGCONFIG(TAG, "  Outdoor Air Temp: %.1f°C", this->outdoor_air_temp_);
  ESP_LOGCONFIG(TAG, "  Supply Air Temp: %.1f°C", this->supply_air_temp_);
  ESP_LOGCONFIG(TAG, "  Extract Air Temp: %.1f°C", this->extract_air_temp_);
}


// Map register value to custom fan mode string
static std::string reg_to_fan_mode_string(int reg) {
  switch (reg) {
    case 0: return "AUTO-VENT";
    case 1: return "MANUAL";
    case 2: return "CROWDED";
    case 3: return "REFRESH";
    case 4: return "FIREPLACE";
    case 5: return "AWAY";
    case 6: return "HOLIDAY";
    case 7: return "COOKERHOOD";
    default: return "AUTO-VENT";
  }
}

// Map custom fan mode string to register value
static int fan_mode_to_reg(const std::string &mode) {
  if (mode == "AUTO-VENT") return 1;
  if (mode == "MANUAL") return 2;
  if (mode == "CROWDED") return 3;
  if (mode == "REFRESH") return 4;
  if (mode == "FIREPLACE") return 5;
  if (mode == "AWAY") return 6;
  if (mode == "HOLIDAY") return 7;
  if (mode == "COOKERHOOD") return 8; // read-only, do not write
  return 0;
}

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_temperature_step(1.0f);
  traits.set_visual_min_temperature(12.0f); 
  traits.set_visual_max_temperature(30.0f);
  //traits.set_supported_modes({climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_OFF});
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
  traits.set_supported_custom_fan_modes({
    "AUTO-VENT", "MANUAL", "CROWDED", "REFRESH", "FIREPLACE", "AWAY", "HOLIDAY", "COOKERHOOD"
  });
  return traits;
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (this->modbus_ == nullptr) return;

  // Handle requested HVAC mode (we only support HEAT; coerce/ignore OFF)
  if (call.get_mode().has_value()) {
    auto m = *call.get_mode();
    if (m == climate::CLIMATE_MODE_HEAT) {
      this->mode = climate::CLIMATE_MODE_HEAT;
    } else if (m == climate::CLIMATE_MODE_OFF) {
      ESP_LOGW(TAG, "OFF mode not supported; staying in HEAT");
      this->mode = climate::CLIMATE_MODE_HEAT;
    }
  }

  // Write setpoint to Modbus if requested
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    ESP_LOGI(TAG, "Setting target temperature: %.1f", temp);
    auto cmd = modbus_controller::ModbusCommandItem::create_write_single_command(
      this->modbus_, REG_SETPOINT, static_cast<uint16_t>(temp * 10)
    );
    this->modbus_->queue_command(cmd);
    this->target_temperature = temp;
  }

  // Write custom fan mode to Modbus if requested
  if (call.has_custom_fan_mode()) {
    std::string mode = call.get_custom_fan_mode().str();
    ESP_LOGI(TAG, "Requested custom fan mode change to: %s", mode.c_str());
    int reg_val = fan_mode_to_reg(mode);
    if (reg_val >= 1 && reg_val <= 7) {
      auto cmd = modbus_controller::ModbusCommandItem::create_write_single_command(
        this->modbus_, REG_FAN_MODE_REQ, static_cast<uint16_t>(reg_val)
      );
      this->modbus_->queue_command(cmd);
    } else if (reg_val == 8) {
      ESP_LOGW(TAG, "COOKERHOOD mode is read-only on Modbus; skipping write");
    } else {
      ESP_LOGW(TAG, "Unsupported fan mode request: %s; skipping", mode.c_str());
    }
  }

  this->publish_state();
}


void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // ...existing code for modbus reads...
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_SETPOINT, &this->target_temperature, "setpoint");
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_OUTDOOR_TEMP, &this->outdoor_air_temp_, "outdoor air temperature");
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING,
                                  REG_SUPPLY_TEMP, &this->supply_air_temp_, "supply air temperature");
    this->current_temperature = this->supply_air_temp_;
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_EXTRACT_TEMP, &this->extract_air_temp_, "extract air temperature");
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::READ,
                             REG_HEAT_DEMAND, &this->heat_demand_percent_, "heat demand", "%");
    auto cmd_saf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_SUPPLY_AIRFLOW, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t saf_raw = (data[0] << 8) | data[1];
          this->saf_volume_ = static_cast<float>(saf_raw) * 3.0f;
          this->saf_percent_ = static_cast<float>(saf_raw);
          ESP_LOGD(TAG, "Read supply air flow: %.1f m³/h (raw: %u)", this->saf_volume_, saf_raw);
          if (this->saf_volume_sensor_ != nullptr)
            this->saf_volume_sensor_->publish_state(this->saf_volume_);
          if (this->saf_percent_sensor_ != nullptr)
            this->saf_percent_sensor_->publish_state(this->saf_percent_);
        } else {
          ESP_LOGE(TAG, "Insufficient data for supply air flow: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_saf);
    auto cmd_eaf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_EXTRACT_AIRFLOW, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t eaf_raw = (data[0] << 8) | data[1];
          this->eaf_volume_ = static_cast<float>(eaf_raw) * 3.0f;
          this->eaf_percent_ = static_cast<float>(eaf_raw);
          ESP_LOGD(TAG, "Read extract air flow: %.1f m³/h (raw: %u)", this->eaf_volume_, eaf_raw);
          if (this->eaf_volume_sensor_ != nullptr)
            this->eaf_volume_sensor_->publish_state(this->eaf_volume_);
          if (this->eaf_percent_sensor_ != nullptr)
            this->eaf_percent_sensor_->publish_state(this->eaf_percent_);
        } else {
          ESP_LOGE(TAG, "Insufficient data for extract air flow: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_eaf);
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::READ,
                             REG_SENSOR_RPM_SAF, &this->rpm_saf_, "supply air fan RPM", " RPM");
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::READ,
                             REG_SENSOR_RPM_EAF, &this->rpm_eaf_, "extract air fan RPM", " RPM");
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::READ,
                             REG_OUTPUT_Y2_ANALOG, &this->heat_exchanger_utilisation_,
                             "heat exchanger utilisation", "%");
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING,
                             REG_SENSOR_RHS_PDM, &this->pdm_rhs_, "PDM RHS sensor");
    auto cmd_fan = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_FAN_MODE, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t fanmode_raw = (data[0] << 8) | data[1];
          this->set_custom_fan_mode_(reg_to_fan_mode_string(fanmode_raw).c_str());
          ESP_LOGD(TAG, "Read fan mode: %s (%d)", this->get_custom_fan_mode().c_str(), fanmode_raw);
        } else {
          ESP_LOGE(TAG, "Insufficient data for fan mode: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_fan);
  }
  this->set_timeout(100, [this]() {
    this->current_temperature = this->supply_air_temp_;
    // Publish sensors for other values
    if (this->heat_demand_sensor_ != nullptr)
      this->heat_demand_sensor_->publish_state(this->heat_demand_percent_);
    if (this->outdoor_air_temp_sensor_ != nullptr)
      this->outdoor_air_temp_sensor_->publish_state(this->outdoor_air_temp_);
    if (this->supply_air_temp_sensor_ != nullptr)
      this->supply_air_temp_sensor_->publish_state(this->supply_air_temp_);
    if (this->extract_air_temp_sensor_ != nullptr)
      this->extract_air_temp_sensor_->publish_state(this->extract_air_temp_);
    if (this->rpm_saf_sensor_ != nullptr)
      this->rpm_saf_sensor_->publish_state(this->rpm_saf_);
    if (this->rpm_eaf_sensor_ != nullptr)
      this->rpm_eaf_sensor_->publish_state(this->rpm_eaf_);
    if (this->heat_exchanger_utilisation_sensor_ != nullptr)
      this->heat_exchanger_utilisation_sensor_->publish_state(this->heat_exchanger_utilisation_);
    if (this->pdm_rhs_sensor_ != nullptr)
      this->pdm_rhs_sensor_->publish_state(this->pdm_rhs_);
    this->publish_state();
  });
}


void SaveVTRClimate::update_alarms() {
  if (this->modbus_ == nullptr)
    return;

  // Read 0-3 alarm text sensors
  for (size_t i = 0; i < ALARM_TEXT_SENSOR_COUNT; i++) {
    if (this->alarm_text_sensors_[i] == nullptr)
      continue;
    uint16_t reg = ALARM_TEXT_REGS[i];
    auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, reg, 1,
      [this, i, reg](modbus_controller::ModbusRegisterType, uint16_t,
                     const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t raw = (data[0] << 8) | data[1];
          const char *state = alarm_state_string(raw);
          ESP_LOGD(TAG, "Alarm[%d] reg %u: %s (%u)", i, reg, state, raw);
          this->alarm_text_sensors_[i]->publish_state(state);
        } else {
          ESP_LOGE(TAG, "Insufficient data for alarm[%d] reg %u", i, reg);
        }
      }
    );
    this->modbus_->queue_command(cmd);
  }

  // Read 0-1 alarm binary sensors
  for (size_t i = 0; i < ALARM_BINARY_SENSOR_COUNT; i++) {
    if (this->alarm_binary_sensors_[i] == nullptr)
      continue;
    uint16_t reg = ALARM_BINARY_REGS[i];
    auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, reg, 1,
      [this, i, reg](modbus_controller::ModbusRegisterType, uint16_t,
                     const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t raw = (data[0] << 8) | data[1];
          bool state = (raw != 0);
          ESP_LOGD(TAG, "Alarm binary[%d] reg %u: %s (%u)", i, reg, state ? "Alarm" : "No alarm", raw);
          this->alarm_binary_sensors_[i]->publish_state(state);
        } else {
          ESP_LOGE(TAG, "Insufficient data for alarm binary[%d] reg %u", i, reg);
        }
      }
    );
    this->modbus_->queue_command(cmd);
  }

  // Read alarm filter warning counter
  if (this->alarm_filter_warning_counter_sensor_ != nullptr) {
    auto cmd = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_ALARM_FILTER_WARNING_COUNTER, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t raw = (data[0] << 8) | data[1];
          ESP_LOGD(TAG, "Alarm filter warning counter: %u", raw);
          this->alarm_filter_warning_counter_sensor_->publish_state(static_cast<float>(raw));
        } else {
          ESP_LOGE(TAG, "Insufficient data for alarm filter warning counter");
        }
      }
    );
    this->modbus_->queue_command(cmd);
  }
}


}  // namespace save_vtr
}  // namespace esphome