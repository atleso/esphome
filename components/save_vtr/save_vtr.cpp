// save_vtr.cpp
#include "save_vtr.h"
#include "esphome/core/log.h"

namespace esphome {
namespace save_vtr {

static const char *const TAG = "save_vtr.climate";

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_AUTO});
  traits.set_supported_custom_fan_modes({"AUTO", "CROWDED", "REFRESH"});
  return traits;
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value() && this->setpoint_number_ != nullptr) {
    float temp = *call.get_target_temperature();
    ESP_LOGI(TAG, "Setting target temperature: %.1f", temp);
    this->target_temperature = temp;
    this->setpoint_number_->publish_state(temp);
  }

  if (call.get_fan_mode().has_value()) {
    auto mode = *call.get_fan_mode();
    ESP_LOGI(TAG, "Requested fan mode change to: %s", climate::fan_mode_to_string(mode).c_str());
    // Optional: Set a select or binary switch here if needed
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  if (this->room_temp_sensor_ != nullptr && this->room_temp_sensor_->has_state()) {
    this->current_temperature = this->room_temp_sensor_->state;
  }
  if (this->setpoint_sensor_ != nullptr && this->setpoint_sensor_->has_state()) {
    this->target_temperature = this->setpoint_sensor_->state;
  }
  this->publish_state();
}

}  // namespace save_vtr
}  // namespace esphome