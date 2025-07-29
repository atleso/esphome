
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

using climate::ClimateFanMode;

// Map register value to ClimateFanMode (custom modes)
static ClimateFanMode reg_to_fan_mode(int reg) {
  switch (reg) {
    case 1: return ClimateFanMode::CLIMATE_FAN_MODE_AUTO;
    case 2: return ClimateFanMode::CLIMATE_FAN_MODE_1; // MANUAL
    case 3: return ClimateFanMode::CLIMATE_FAN_MODE_2; // CROWDED
    case 4: return ClimateFanMode::CLIMATE_FAN_MODE_3; // REFRESH
    case 5: return ClimateFanMode::CLIMATE_FAN_MODE_4; // FIREPLACE
    case 6: return ClimateFanMode::CLIMATE_FAN_MODE_5; // AWAY
    case 7: return ClimateFanMode::CLIMATE_FAN_MODE_6; // HOLIDAY
    case 8: return ClimateFanMode::CLIMATE_FAN_MODE_7; // COOKERHOOD
    default: return ClimateFanMode::CLIMATE_FAN_MODE_AUTO;
  }
}

// Map ClimateFanMode to register value (custom modes)
static int fan_mode_to_reg(ClimateFanMode mode) {
  switch (mode) {
    case ClimateFanMode::CLIMATE_FAN_MODE_AUTO: return 1;
    case ClimateFanMode::CLIMATE_FAN_MODE_1: return 2; // MANUAL
    case ClimateFanMode::CLIMATE_FAN_MODE_2: return 3; // CROWDED
    case ClimateFanMode::CLIMATE_FAN_MODE_3: return 4; // REFRESH
    case ClimateFanMode::CLIMATE_FAN_MODE_4: return 5; // FIREPLACE
    case ClimateFanMode::CLIMATE_FAN_MODE_5: return 6; // AWAY
    case ClimateFanMode::CLIMATE_FAN_MODE_6: return 7; // HOLIDAY
    case ClimateFanMode::CLIMATE_FAN_MODE_7: return 8; // COOKERHOOD
    default: return 1;
  }
}

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_OFF});
  traits.set_supported_fan_modes({
    ClimateFanMode::CLIMATE_FAN_MODE_AUTO,   // AUTO
    ClimateFanMode::CLIMATE_FAN_MODE_1,      // MANUAL
    ClimateFanMode::CLIMATE_FAN_MODE_2,      // CROWDED
    ClimateFanMode::CLIMATE_FAN_MODE_3,      // REFRESH
    ClimateFanMode::CLIMATE_FAN_MODE_4,      // FIREPLACE
    ClimateFanMode::CLIMATE_FAN_MODE_5,      // AWAY
    ClimateFanMode::CLIMATE_FAN_MODE_6,      // HOLIDAY
    ClimateFanMode::CLIMATE_FAN_MODE_7       // COOKERHOOD
  });
  return traits;
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (this->modbus_ == nullptr) return;

  // Write setpoint to Modbus if requested
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    ESP_LOGI(TAG, "Setting target temperature: %.1f", temp);
    auto *cmd = new modbus_controller::ModbusCommandItem();
    cmd->setup_write_single_register(1001, static_cast<uint16_t>(temp * 10));
    this->modbus_->queue_command(cmd);
    this->target_temperature = temp;
  }

  // Write fan mode to Modbus if requested
  if (call.get_fan_mode().has_value()) {
    ClimateFanMode mode = *call.get_fan_mode();
    ESP_LOGI(TAG, "Requested fan mode change to: %s", climate::climate_fan_mode_to_string(mode));
    int reg_val = fan_mode_to_reg(mode);
    if (reg_val != 8) { // 8 = COOKERHOOD, read-only
      auto *cmd = new modbus_controller::ModbusCommandItem();
      cmd->setup_write_single_register(1162, reg_val);
      this->modbus_->queue_command(cmd);
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // Read room temperature (REG_ROOM_TEMP = 1000)
    auto *cmd_temp = new modbus_controller::ModbusCommandItem();
    cmd_temp->setup_read_holding_register(1000, 1);
    cmd_temp->on_data([this](const std::vector<uint8_t> &data) {
      uint16_t temp_raw = (data[0] << 8) | data[1];
      this->current_temperature = temp_raw / 10.0f;
    });
    this->modbus_->queue_command(cmd_temp);

    // Read setpoint (REG_SETPOINT = 1001)
    auto *cmd_setpoint = new modbus_controller::ModbusCommandItem();
    cmd_setpoint->setup_read_holding_register(1001, 1);
    cmd_setpoint->on_data([this](const std::vector<uint8_t> &data) {
      uint16_t setpoint_raw = (data[0] << 8) | data[1];
      this->target_temperature = setpoint_raw / 10.0f;
    });
    this->modbus_->queue_command(cmd_setpoint);

    // Read active fan mode from REG_USERMODE_MODE (1162)
    auto *cmd_fan = new modbus_controller::ModbusCommandItem();
    cmd_fan->setup_read_holding_register(1162, 1);
    cmd_fan->on_data([this](const std::vector<uint8_t> &data) {
      uint16_t fanmode_raw = (data[0] << 8) | data[1];
      this->fan_mode = reg_to_fan_mode(fanmode_raw);
    });
    this->modbus_->queue_command(cmd_fan);
  }
  this->publish_state();
}

}  // namespace save_vtr
}  // namespace esphome