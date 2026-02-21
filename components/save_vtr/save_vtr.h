#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/modbus_controller/modbus_controller.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace save_vtr {

static const size_t ALARM_TEXT_SENSOR_COUNT = 32;
static const size_t ALARM_BINARY_SENSOR_COUNT = 5;

class SaveVTRClimate : public climate::Climate, public PollingComponent {
 public:
  void update() override;
  void update_alarms();
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  void dump_config() override;
  void set_modbus(modbus_controller::ModbusController *modbus);
  void setup() override;

  // Sensor setter methods
  void set_saf_percent_sensor(esphome::sensor::Sensor *sensor) { saf_percent_sensor_ = sensor; }
  void set_saf_volume_sensor(esphome::sensor::Sensor *sensor) { saf_volume_sensor_ = sensor; }
  void set_eaf_percent_sensor(esphome::sensor::Sensor *sensor) { eaf_percent_sensor_ = sensor; }
  void set_eaf_volume_sensor(esphome::sensor::Sensor *sensor) { eaf_volume_sensor_ = sensor; }
  void set_heat_demand_sensor(esphome::sensor::Sensor *sensor) { heat_demand_sensor_ = sensor; }
  void set_outdoor_air_temp_sensor(esphome::sensor::Sensor *sensor) { outdoor_air_temp_sensor_ = sensor; }
  void set_supply_air_temp_sensor(esphome::sensor::Sensor *sensor) { supply_air_temp_sensor_ = sensor; }
  void set_extract_air_temp_sensor(esphome::sensor::Sensor *sensor) { extract_air_temp_sensor_ = sensor; }
  void set_rpm_saf_sensor(esphome::sensor::Sensor *sensor) { rpm_saf_sensor_ = sensor; }
  void set_rpm_eaf_sensor(esphome::sensor::Sensor *sensor) { rpm_eaf_sensor_ = sensor; }
  void set_heat_exchanger_utilisation_sensor(esphome::sensor::Sensor *sensor) {
    heat_exchanger_utilisation_sensor_ = sensor;
  }
  void set_pdm_rhs_sensor(esphome::sensor::Sensor *sensor) { pdm_rhs_sensor_ = sensor; }

  // Alarm setter methods
  void set_alarm_update_interval(uint32_t interval_ms) { alarm_update_interval_ = interval_ms; }
  void set_alarm_text_sensor(size_t idx, esphome::text_sensor::TextSensor *s) {
    if (idx < ALARM_TEXT_SENSOR_COUNT)
      alarm_text_sensors_[idx] = s;
  }
  void set_alarm_binary_sensor(size_t idx, esphome::binary_sensor::BinarySensor *s) {
    if (idx < ALARM_BINARY_SENSOR_COUNT)
      alarm_binary_sensors_[idx] = s;
  }
  void set_alarm_filter_warning_counter_sensor(esphome::sensor::Sensor *s) {
    alarm_filter_warning_counter_sensor_ = s;
  }

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
  float rpm_saf_{0.0f};                 // Supply air fan RPM
  float rpm_eaf_{0.0f};                 // Extract air fan RPM
  float heat_exchanger_utilisation_{0.0f};  // Heat exchanger utilisation (0-100%)
  float pdm_rhs_{0.0f};                     // PDM RHS sensor (0-100)

  // Alarm update interval (ms)
  uint32_t alarm_update_interval_{30000};
  bool update_pending_{false};  // guard against duplicate modbus commands

  // Sensor pointers
  esphome::sensor::Sensor *saf_percent_sensor_{nullptr};
  esphome::sensor::Sensor *saf_volume_sensor_{nullptr};
  esphome::sensor::Sensor *eaf_percent_sensor_{nullptr};
  esphome::sensor::Sensor *eaf_volume_sensor_{nullptr};
  esphome::sensor::Sensor *heat_demand_sensor_{nullptr};
  esphome::sensor::Sensor *outdoor_air_temp_sensor_{nullptr};
  esphome::sensor::Sensor *supply_air_temp_sensor_{nullptr};
  esphome::sensor::Sensor *extract_air_temp_sensor_{nullptr};
  esphome::sensor::Sensor *rpm_saf_sensor_{nullptr};
  esphome::sensor::Sensor *rpm_eaf_sensor_{nullptr};
  esphome::sensor::Sensor *heat_exchanger_utilisation_sensor_{nullptr};
  esphome::sensor::Sensor *pdm_rhs_sensor_{nullptr};

  // Alarm sensor arrays
  esphome::text_sensor::TextSensor *alarm_text_sensors_[ALARM_TEXT_SENSOR_COUNT]{};
  esphome::binary_sensor::BinarySensor *alarm_binary_sensors_[ALARM_BINARY_SENSOR_COUNT]{};
  esphome::sensor::Sensor *alarm_filter_warning_counter_sensor_{nullptr};
};

}  // namespace save_vtr
}  // namespace esphome
