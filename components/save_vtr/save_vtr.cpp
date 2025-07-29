
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
    this->modbus_->queue_custom_command(
      1, // unit id
      modbus_controller::ModbusFunctionCode::WRITE_SINGLE_REGISTER,
      1001, // setpoint register
      {static_cast<uint16_t>(temp * 10)},
      [this, temp](const std::vector<uint8_t> &data) {
        this->target_temperature = temp;
        ESP_LOGD(TAG, "Setpoint written, confirmed: %.1f", temp);
      },
      [this](modbus_controller::ModbusCommandItem *item, modbus_controller::ModbusError err) {
        ESP_LOGW(TAG, "Failed to write setpoint: %d", err);
      }
    );
  }

  // Write custom fan mode to Modbus if requested
  if (call.get_custom_fan_mode().has_value()) {
    std::string mode = *call.get_custom_fan_mode();
    ESP_LOGI(TAG, "Requested custom fan mode change to: %s", mode.c_str());
    int reg_val = fan_mode_to_reg(mode);
    if (reg_val != 8) { // 8 = COOKERHOOD, read-only
      this->modbus_->queue_custom_command(
        1, // unit id
        modbus_controller::ModbusFunctionCode::WRITE_SINGLE_REGISTER,
        1162, // fan mode register
        {static_cast<uint16_t>(reg_val)},
        [this, reg_val](const std::vector<uint8_t> &data) {
          ESP_LOGD(TAG, "Fan mode written, confirmed: %d", reg_val);
        },
        [this](modbus_controller::ModbusCommandItem *item, modbus_controller::ModbusError err) {
          ESP_LOGW(TAG, "Failed to write fan mode: %d", err);
        }
      );
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // Read room temperature (REG_ROOM_TEMP = 1000)
    this->modbus_->queue_custom_command(
      1, // unit id
      modbus_controller::ModbusFunctionCode::READ_HOLDING_REGISTERS,
      1000, // room temp register
      1, // quantity
      [this](const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t temp_raw = (data[0] << 8) | data[1];
          this->current_temperature = temp_raw / 10.0f;
        }
      },
      [this](modbus_controller::ModbusCommandItem *item, modbus_controller::ModbusError err) {
        ESP_LOGW(TAG, "Failed to read room temperature: %d", err);
      }
    );

    // Read setpoint (REG_SETPOINT = 1001)
    this->modbus_->queue_custom_command(
      1, // unit id
      modbus_controller::ModbusFunctionCode::READ_HOLDING_REGISTERS,
      1001, // setpoint register
      1, // quantity
      [this](const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t setpoint_raw = (data[0] << 8) | data[1];
          this->target_temperature = setpoint_raw / 10.0f;
        }
      },
      [this](modbus_controller::ModbusCommandItem *item, modbus_controller::ModbusError err) {
        ESP_LOGW(TAG, "Failed to read setpoint: %d", err);
      }
    );

    // Read active fan mode from REG_USERMODE_MODE (1162)
    this->modbus_->queue_custom_command(
      1, // unit id
      modbus_controller::ModbusFunctionCode::READ_HOLDING_REGISTERS,
      1162, // fan mode register
      1, // quantity
      [this](const std::vector<uint8_t> &data) {
        if (data.size() >= 2) {
          uint16_t fanmode_raw = (data[0] << 8) | data[1];
          this->custom_fan_mode = reg_to_fan_mode_string(fanmode_raw);
        }
      },
      [this](modbus_controller::ModbusCommandItem *item, modbus_controller::ModbusError err) {
        ESP_LOGW(TAG, "Failed to read fan mode: %d", err);
      }
    );
  }
  this->publish_state();
}

}  // namespace save_vtr
}  // namespace esphome