
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/modbus_controller/modbus_controller.h"

namespace esphome {
namespace save_vtr {

class SaveVTRClimate : public climate::Climate, public PollingComponent {
 public:
  void update() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  void dump_config() override;
  void set_modbus(modbus_controller::ModbusController *modbus);

 protected:
  modbus_controller::ModbusController *modbus_{nullptr};
  float heat_demand_percent_{0.0f};     // Heat demand percentage (0-100%)
  float saf_percent_{0.0f};             // Supply Air Flow (same as volume)
  float saf_volume_{0.0f};              // Supply Air Flow volume (m³/h)
  float eaf_percent_{0.0f};             // Extract Air Flow (same as volume)
  float eaf_volume_{0.0f};              // Extract Air Flow volume (m³/h)
  float outdoor_air_temp_{0.0f};        // Outdoor air temperature (°C, scaled /10)
  float supply_air_temp_{0.0f};         // Supply air temperature (°C, scaled /10)
  float extract_air_temp_{0.0f};        // Extract air temperature (°C, scaled /10)
};

}  // namespace save_vtr
}  // namespace esphome
