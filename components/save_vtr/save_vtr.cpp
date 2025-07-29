#include "save_vtr.h"
#include "esphome/core/log.h"

namespace esphome {
namespace save_vtr {

static const char *const TAG = "save_vtr.climate";

climate::ClimateTraits SaveVTRClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_AUTO});
  traits.set_supported_custom_fan_modes({
    "AUTO", "MANUAL", "CROWDED", "REFRESH", "FIREPLACE", "AWAY", "HOLIDAY", "COOKERHOOD"
  });
  return traits;
}

void SaveVTRClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    modbus_->queue_write_register(2001, static_cast<uint16_t>(temp * 10));
    this->target_temperature = temp;
  }

  if (call.get_fan_mode().has_value()) {
    std::string mode = *call.get_fan_mode();
    uint16_t val = 1;
    if (mode == "AUTO") val = 1;
    else if (mode == "MANUAL") val = 2;
    else if (mode == "CROWDED") val = 3;
    else if (mode == "REFRESH") val = 4;
    else if (mode == "FIREPLACE") val = 5;
    else if (mode == "AWAY") val = 6;
    else if (mode == "HOLIDAY") val = 7;

    modbus_->queue_write_register(1162, val);
    this->custom_fan_mode = mode;
  }

  this->publish_state();
}

void SaveVTRClimate::update() {
  float room_temp = modbus_->read_register(12104) * 0.1f;
  float setpoint = modbus_->read_register(2054) * 0.1f;
  uint16_t mode_val = modbus_->read_register(1161);
  uint16_t saf = modbus_->read_register(14001);
  uint16_t eaf = modbus_->read_register(14002);

  this->current_temperature = room_temp;
  this->target_temperature = setpoint;

  std::string mode = "AUTO";
  if (mode_val == 2) mode = "MANUAL";
  else if (mode_val == 3) mode = "CROWDED";
  else if (mode_val == 4) mode = "REFRESH";
  else if (mode_val == 5) mode = "FIREPLACE";
  else if (mode_val == 6) mode = "AWAY";
  else if (mode_val == 7) mode = "HOLIDAY";
  else if (mode_val == 8) mode = "COOKERHOOD";
  this->custom_fan_mode = mode;

  this->add_custom_state_value("supply_air_percent", to_string(saf));
  this->add_custom_state_value("extract_air_percent", to_string(eaf));

  this->publish_state();
}

}  // namespace save_vtr
}  // namespace esphome

// REGISTER!
#include "esphome/core/application.h"

using namespace esphome;
using namespace save_vtr;

ESPHOME_COMPONENT_REGISTER("save_vtr", SaveVTRClimate)
