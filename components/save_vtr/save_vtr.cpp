
#include "save_vtr.h"
#include "esphome/core/log.h"

namespace esphome {
namespace save_vtr {

void SaveVTRClimate::set_modbus(modbus_controller::ModbusController *modbus) {
  this->modbus_ = modbus;
}

static const char *const TAG = "save_vtr.climate";

void SaveVTRClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "SaveVTRClimate:");
  LOG_CLIMATE("  ", "SaveVTRClimate", this);
  ESP_LOGCONFIG(TAG, "  Using direct Modbus for temperature, setpoint, and fan mode");
  ESP_LOGCONFIG(TAG, "  Heat demand: %.0f%%", this->heat_demand_percent_);
  ESP_LOGCONFIG(TAG, "  Supply Air Flow: %.0f%% / %.1f m³/h", this->saf_percent_, this->saf_volume_);
  ESP_LOGCONFIG(TAG, "  Extract Air Flow: %.0f%% / %.1f m³/h", this->eaf_percent_, this->eaf_volume_);
}


// Map register value to custom fan mode string
static std::string reg_to_fan_mode_string(int reg) {
  switch (reg) {
    case 1: return "AUTO";
    case 2: return "MANUAL";
    case 3: return "CROWDED";
    case 4: return "REFRESH";
    case 5: return "FIREPLACE";
    case 6: return "AWAY";
    case 7: return "HOLIDAY";
    case 8: return "COOKERHOOD";
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
  if (mode == "COOKERHOOD") return 8;
  return 1;
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
      this->modbus_, 1001, static_cast<uint16_t>(temp * 10)
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
        this->modbus_, 1162, static_cast<uint16_t>(reg_val)
      );
      this->modbus_->queue_command(cmd);
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // Read room temperature (register 2001) - signed 16-bit
    auto cmd_temp = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 2001, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          // Convert to signed 16-bit integer
          int16_t temp_raw = static_cast<int16_t>((data[0] << 8) | data[1]);
          this->current_temperature = temp_raw / 10.0f;
          ESP_LOGD(TAG, "Read room temperature: %.1f (raw: %d)", this->current_temperature, temp_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_temp);
    
    // Read setpoint (register 2054) - signed 16-bit
    auto cmd_setpoint = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, 2054, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          // Convert to signed 16-bit integer
          int16_t setpoint_raw = static_cast<int16_t>((data[0] << 8) | data[1]);
          this->target_temperature = setpoint_raw / 10.0f;
          ESP_LOGD(TAG, "Read setpoint: %.1f (raw: %d)", this->target_temperature, setpoint_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_setpoint);
    
    // Read fan mode (register 1162)
    auto cmd_fan = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::HOLDING, 1162, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t fanmode_raw = (data[0] << 8) | data[1];
          this->custom_fan_mode = reg_to_fan_mode_string(fanmode_raw);
          ESP_LOGD(TAG, "Read fan mode: %s (%d)", this->custom_fan_mode.value().c_str(), fanmode_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_fan);
    
    // Read heat demand (register 2055) - uint16, read-only
    auto cmd_heat_demand = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, 2055, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          // Convert to unsigned 16-bit integer (should be 0-100)
          uint16_t heat_demand_raw = (data[0] << 8) | data[1];
          this->heat_demand_percent_ = static_cast<float>(heat_demand_raw);
          ESP_LOGD(TAG, "Read heat demand: %.0f%% (raw: %u)", this->heat_demand_percent_, heat_demand_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_heat_demand);
    
    // Read Supply Air Flow (register 14001) - uint16, read-only
    auto cmd_saf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, 14001, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          // Convert to unsigned 16-bit integer
          uint16_t saf_raw = (data[0] << 8) | data[1];
          this->saf_percent_ = static_cast<float>(saf_raw);
          this->saf_volume_ = static_cast<float>(saf_raw);  // Direct value, no scaling
          ESP_LOGD(TAG, "Read SAF: %.0f%% / %.1f m³/h (raw: %u)", this->saf_percent_, this->saf_volume_, saf_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_saf);
    
    // Read Extract Air Flow (register 14002) - uint16, read-only
    auto cmd_eaf = modbus_controller::ModbusCommandItem::create_read_command(
      this->modbus_, modbus_controller::ModbusRegisterType::READ, 14002, 1,
      [this](modbus_controller::ModbusRegisterType register_type, uint16_t start_address, 
              const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          // Convert to unsigned 16-bit integer
          uint16_t eaf_raw = (data[0] << 8) | data[1];
          this->eaf_percent_ = static_cast<float>(eaf_raw);
          this->eaf_volume_ = static_cast<float>(eaf_raw);  // Direct value, no scaling
          ESP_LOGD(TAG, "Read EAF: %.0f%% / %.1f m³/h (raw: %u)", this->eaf_percent_, this->eaf_volume_, eaf_raw);
        }
      }
    );
    this->modbus_->queue_command(cmd_eaf);
  }
  
  // Publish state after a short delay to allow async operations to complete
  this->set_timeout(100, [this]() {
    this->publish_state();
  });
}

}  // namespace save_vtr
}  // namespace esphome