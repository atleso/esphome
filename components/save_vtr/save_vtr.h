#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace save_vtr {

class SaveVTRClimate : public climate::Climate, public PollingComponent {
 public:
  void setup() override {}
  void update() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  void set_room_temp_sensor(sensor::Sensor *sensor) { this->room_temp_sensor_ = sensor; }
  void set_setpoint_sensor(sensor::Sensor *sensor) { this->setpoint_sensor_ = sensor; }
  void set_setpoint_number(number::Number *num) { this->setpoint_number_ = num; }

 protected:
  sensor::Sensor *room_temp_sensor_{nullptr};
  sensor::Sensor *setpoint_sensor_{nullptr};
  number::Number *setpoint_number_{nullptr};
};

}  // namespace save_vtr
}  // namespace esphome
