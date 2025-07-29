
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
}


// Helper: Map ESPHome fan mode string to register value
static int fan_mode_to_reg(const std::string &mode) {
  if (mode == "AUTO") return 1;
  if (mode == "MANUAL") return 2;
  if (mode == "CROWDED") return 3;
  if (mode == "REFRESH") return 4;
  if (mode == "FIREPLACE") return 5;
  if (mode == "AWAY") return 6;
  if (mode == "HOLIDAY") return 7;
  if (mode == "COOKERHOOD") return 8;
  return 1; // default to AUTO
}

// Helper: Map register value to ESPHome fan mode string
static std::string reg_to_fan_mode(int reg) {
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
  // Write setpoint to Modbus if requested
  if (call.get_target_temperature().has_value() && this->modbus_ != nullptr) {
    float temp = *call.get_target_temperature();
    ESP_LOGI(TAG, "Setting target temperature: %.1f", temp);
    this->target_temperature = temp;
    this->modbus_->write_single_register(1001, static_cast<uint16_t>(temp * 10)); // REG_SETPOINT
  }

  // Write fan mode to Modbus if requested
  if (call.get_fan_mode().has_value()) {
    auto mode = *call.get_fan_mode();
    ESP_LOGI(TAG, "Requested fan mode change to: %s", climate::climate_fan_mode_to_string(mode));
    std::string mode_str = mode;
    int reg_val = fan_mode_to_reg(mode_str);
    if (reg_val != 8 && this->modbus_ != nullptr) { // 8 = COOKERHOOD, read-only
      this->modbus_->write_single_register(1162, reg_val); // REG_USERMODE_HMI_CHANGE
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    uint16_t temp_raw = 0, setpoint_raw = 0, fanmode_raw = 1;
    // Read room temperature (REG_ROOM_TEMP = 1000)
    if (this->modbus_->read_holding_register(1000, &temp_raw)) {
      this->current_temperature = temp_raw / 10.0f;
    }
    // Read setpoint (REG_SETPOINT = 1001)
    if (this->modbus_->read_holding_register(1001, &setpoint_raw)) {
      this->target_temperature = setpoint_raw / 10.0f;
    }
    // Read active fan mode from REG_USERMODE_MODE (1162)
    if (this->modbus_->read_holding_register(1162, &fanmode_raw)) {
      this->fan_mode = reg_to_fan_mode(fanmode_raw);
    }
  }
  this->publish_state();
}

}  // namespace save_vtr
}  // namespace esphome