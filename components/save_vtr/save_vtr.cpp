
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

void SaveVTRClimate::set_modbus(modbus_controller::ModbusController *modbus) {
  this->modbus_ = modbus;
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
    case 0: return "AUTO";
    case 1: return "MANUAL";
    case 2: return "CROWDED";
    case 3: return "REFRESH";
    case 4: return "FIREPLACE";
    case 5: return "AWAY";
    case 6: return "HOLIDAY";
    case 7: return "COOKERHOOD";
    default: return "AUTO";
  }
}

// Map custom fan mode string to register value
static int fan_mode_to_reg(const std::string &mode) {
  if (mode == "AUTO") return 1;
  if (mode == "MANUAL") return 2;
  if (mode == "CROWDED") return 3;
  if (mode == "REFRESH") return 4;
  if (mode == "FIREPLACE") return 5;
  if (mode == "AWAY") return 6;
  if (mode == "HOLIDAY") return 7;
  return 0;
}

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_OFF});
  traits.set_supported_custom_fan_modes({
    "AUTO", "MANUAL", "CROWDED", "REFRESH", "FIREPLACE", "AWAY", "HOLIDAY", "COOKERHOOD"
  });
  return traits;
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (this->modbus_ == nullptr) return;

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
  if (call.get_custom_fan_mode().has_value()) {
    std::string mode = *call.get_custom_fan_mode();
    ESP_LOGI(TAG, "Requested custom fan mode change to: %s", mode.c_str());
    int reg_val = fan_mode_to_reg(mode);
    if (reg_val != 8) { // 8 = COOKERHOOD, read-only
      auto cmd = modbus_controller::ModbusCommandItem::create_write_single_command(
        this->modbus_, REG_FAN_MODE_REQ, static_cast<uint16_t>(reg_val)
      );
      this->modbus_->queue_command(cmd);
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // Read room temperature - signed 16-bit with /10.0 scaling
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_ROOM_TEMP, &this->current_temperature, "room temperature");
    
    // Read setpoint - signed 16-bit with /10.0 scaling
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_SETPOINT, &this->target_temperature, "setpoint");
    
    // Read outdoor air temperature - signed 16-bit with /10.0 scaling
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_OUTDOOR_TEMP, &this->outdoor_air_temp_, "outdoor air temperature");
    
    // Read supply air temperature - signed 16-bit with /10.0 scaling
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_SUPPLY_TEMP, &this->supply_air_temp_, "supply air temperature");
    
    // Read extract air temperature - signed 16-bit with /10.0 scaling
    create_temperature_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 
                                  REG_EXTRACT_TEMP, &this->extract_air_temp_, "extract air temperature");
    
    // Read heat demand - unsigned 16-bit percentage
    create_uint16_read_command(this, this->modbus_, modbus_controller::ModbusRegisterType::READ,
                             REG_HEAT_DEMAND, &this->heat_demand_percent_, "heat demand", "%");
    
    // Read supply air flow volume - unsigned 16-bit, direct value in m³/h
    auto cmd_saf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_SUPPLY_AIRFLOW, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t saf_raw = (data[0] << 8) | data[1];
          this->saf_volume_ = static_cast<float>(saf_raw) * 3.0f;
          this->saf_percent_ = static_cast<float>(saf_raw); 
          ESP_LOGD(TAG, "Read supply air flow: %.1f m³/h (raw: %u)", this->saf_volume_, saf_raw);
        } else {
          ESP_LOGE(TAG, "Insufficient data for supply air flow: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_saf);
    
    // Read extract air flow volume - unsigned 16-bit, direct value in m³/h
    auto cmd_eaf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_EXTRACT_AIRFLOW, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t eaf_raw = (data[0] << 8) | data[1];
          this->eaf_volume_ = static_cast<float>(eaf_raw) * 3.0f;
          this->eaf_percent_ = static_cast<float>(eaf_raw); 
          ESP_LOGD(TAG, "Read extract air flow: %.1f m³/h (raw: %u)", this->eaf_volume_, eaf_raw);
        } else {
          ESP_LOGE(TAG, "Insufficient data for extract air flow: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_eaf);
    
    // Read fan mode - unsigned 16-bit enum value
    auto cmd_fan = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, REG_FAN_MODE, 1,
      [this](modbus_controller::ModbusRegisterType, uint16_t, const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t fanmode_raw = (data[0] << 8) | data[1];
          this->custom_fan_mode = reg_to_fan_mode_string(fanmode_raw);
          ESP_LOGD(TAG, "Read fan mode: %s (%d)", this->custom_fan_mode.value().c_str(), fanmode_raw);
        } else {
          ESP_LOGE(TAG, "Insufficient data for fan mode: got %d bytes, expected 2", data.size());
        }
      }
    );
    this->modbus_->queue_command(cmd_fan);
  }
  
  // Publish state after a short delay to allow async operations to complete
  this->set_timeout(100, [this]() {
    this->publish_state();
  });
}

}  // namespace save_vtr
}  // namespace esphome