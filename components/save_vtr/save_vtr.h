
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/modbus_controller/modbus_controller.h"

namespace esphome {
namespace save_vtr {

class SaveVTRClimate : public climate::Climate, public PollingComponent {
 public:
  void setup() override {}
  void update() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  void dump_config() override;
  void set_modbus(modbus_controller::ModbusController *modbus);

 protected:
  modbus_controller::ModbusController *modbus_{nullptr};
};

}  // namespace save_vtr
}  // namespace esphome
