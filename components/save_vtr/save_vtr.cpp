
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
    
    // Use the async write method
    this->modbus_->send_command_async([this, temp](uint8_t function_code, uint16_t start_address, 
                                                   const std::vector<uint8_t> &data, 
                                                   const std::function<void(const std::vector<uint8_t>&)> &on_data) {
      // Prepare write single register command
      std::vector<uint8_t> payload;
      uint16_t reg_addr = 1001;
      uint16_t reg_value = static_cast<uint16_t>(temp * 10);
      
      payload.push_back((reg_addr >> 8) & 0xFF);  // High byte of register address
      payload.push_back(reg_addr & 0xFF);         // Low byte of register address
      payload.push_back((reg_value >> 8) & 0xFF); // High byte of value
      payload.push_back(reg_value & 0xFF);        // Low byte of value
      
      return this->modbus_->send_raw_command(0x06, payload, [this, temp](const std::vector<uint8_t> &response) {
        this->target_temperature = temp;
        ESP_LOGD(TAG, "Setpoint written successfully: %.1f", temp);
      });
    });
  }

  // Write custom fan mode to Modbus if requested
  if (call.get_custom_fan_mode().has_value()) {
    std::string mode = *call.get_custom_fan_mode();
    ESP_LOGI(TAG, "Requested custom fan mode change to: %s", mode.c_str());
    int reg_val = fan_mode_to_reg(mode);
    if (reg_val != 8) { // 8 = COOKERHOOD, read-only
      
      this->modbus_->send_command_async([this, reg_val](uint8_t function_code, uint16_t start_address, 
                                                        const std::vector<uint8_t> &data, 
                                                        const std::function<void(const std::vector<uint8_t>&)> &on_data) {
        // Prepare write single register command
        std::vector<uint8_t> payload;
        uint16_t reg_addr = 1162;
        uint16_t reg_value = static_cast<uint16_t>(reg_val);
        
        payload.push_back((reg_addr >> 8) & 0xFF);  // High byte of register address
        payload.push_back(reg_addr & 0xFF);         // Low byte of register address
        payload.push_back((reg_value >> 8) & 0xFF); // High byte of value
        payload.push_back(reg_value & 0xFF);        // Low byte of value
        
        return this->modbus_->send_raw_command(0x06, payload, [this, reg_val](const std::vector<uint8_t> &response) {
          ESP_LOGD(TAG, "Fan mode written successfully: %d", reg_val);
        });
      });
    }
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->modbus_ != nullptr) {
    // Read room temperature (register 1000) asynchronously
    this->modbus_->send_command_async([this](uint8_t function_code, uint16_t start_address, 
                                             const std::vector<uint8_t> &data, 
                                             const std::function<void(const std::vector<uint8_t>&)> &on_data) {
      std::vector<uint8_t> payload;
      uint16_t reg_addr = 1000;
      uint16_t reg_count = 1;
      
      payload.push_back((reg_addr >> 8) & 0xFF);   // High byte of register address
      payload.push_back(reg_addr & 0xFF);          // Low byte of register address
      payload.push_back((reg_count >> 8) & 0xFF);  // High byte of register count
      payload.push_back(reg_count & 0xFF);         // Low byte of register count
      
      return this->modbus_->send_raw_command(0x03, payload, [this](const std::vector<uint8_t> &response) {
        if (response.size() >= 3 && response[0] >= 2) {  // Check if we have at least 2 data bytes
          uint16_t temp_raw = (response[1] << 8) | response[2];
          this->current_temperature = temp_raw / 10.0f;
          ESP_LOGD(TAG, "Read room temperature: %.1f", this->current_temperature);
        }
      });
    });
    
    // Read setpoint (register 1001) asynchronously
    this->modbus_->send_command_async([this](uint8_t function_code, uint16_t start_address, 
                                             const std::vector<uint8_t> &data, 
                                             const std::function<void(const std::vector<uint8_t>&)> &on_data) {
      std::vector<uint8_t> payload;
      uint16_t reg_addr = 1001;
      uint16_t reg_count = 1;
      
      payload.push_back((reg_addr >> 8) & 0xFF);   // High byte of register address
      payload.push_back(reg_addr & 0xFF);          // Low byte of register address
      payload.push_back((reg_count >> 8) & 0xFF);  // High byte of register count
      payload.push_back(reg_count & 0xFF);         // Low byte of register count
      
      return this->modbus_->send_raw_command(0x03, payload, [this](const std::vector<uint8_t> &response) {
        if (response.size() >= 3 && response[0] >= 2) {  // Check if we have at least 2 data bytes
          uint16_t setpoint_raw = (response[1] << 8) | response[2];
          this->target_temperature = setpoint_raw / 10.0f;
          ESP_LOGD(TAG, "Read setpoint: %.1f", this->target_temperature);
        }
      });
    });
    
    // Read fan mode (register 1162) asynchronously
    this->modbus_->send_command_async([this](uint8_t function_code, uint16_t start_address, 
                                             const std::vector<uint8_t> &data, 
                                             const std::function<void(const std::vector<uint8_t>&)> &on_data) {
      std::vector<uint8_t> payload;
      uint16_t reg_addr = 1162;
      uint16_t reg_count = 1;
      
      payload.push_back((reg_addr >> 8) & 0xFF);   // High byte of register address
      payload.push_back(reg_addr & 0xFF);          // Low byte of register address
      payload.push_back((reg_count >> 8) & 0xFF);  // High byte of register count
      payload.push_back(reg_count & 0xFF);         // Low byte of register count
      
      return this->modbus_->send_raw_command(0x03, payload, [this](const std::vector<uint8_t> &response) {
        if (response.size() >= 3 && response[0] >= 2) {  // Check if we have at least 2 data bytes
          uint16_t fanmode_raw = (response[1] << 8) | response[2];
          this->custom_fan_mode = reg_to_fan_mode_string(fanmode_raw);
          ESP_LOGD(TAG, "Read fan mode: %s (%d)", this->custom_fan_mode.c_str(), fanmode_raw);
        }
      });
    });
  }
  
  // Publish state after a short delay to allow async operations to complete
  this->set_timeout(100, [this]() {
    this->publish_state();
  });
}

}  // namespace save_vtr
}  // namespace esphome