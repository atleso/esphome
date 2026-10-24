#pragma once

#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/version.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/modbus_controller/modbus_controller.h"

#if ESPHOME_VERSION_CODE < VERSION_CODE(2026, 9, 0)
#error "save_vtr requires ESPHome 2026.9.0 or newer (modbus_controller::ControllerDevice API)"
#endif

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

namespace esphome {
namespace save_vtr {

/// Number of 0-3 "text" alarm registers polled in a detail scan.
static constexpr size_t ALARM_TEXT_SENSOR_COUNT = 32;
/// Number of 0-1 summary alarm flags polled every update cycle.
static constexpr size_t ALARM_BINARY_SENSOR_COUNT = 5;

/// Button action identifiers (must match button.py)
enum SaveVTRButtonAction : uint8_t {
  ACTION_ACTIVATE_FIREPLACE = 1,
  ACTION_ACTIVATE_REFRESH = 2,
  ACTION_ACTIVATE_CROWDED = 3,
};

class SaveVTRClimate;

#ifdef USE_SWITCH
/// Boolean setting exposed as a switch; writes register `reg` (0/1) and
/// publishes optimistically until the next poll confirms the state.
class SaveVTRSwitch : public switch_::Switch, public Component {
 public:
  SaveVTRSwitch(SaveVTRClimate *parent, uint16_t reg) : parent_(parent), reg_(reg) {}

 protected:
  void write_state(bool state) override;
  SaveVTRClimate *parent_{nullptr};
  uint16_t reg_{0};
};
#endif

#ifdef USE_NUMBER
/// Numeric setting (raw register value == user value, 1:1).
class SaveVTRNumber : public number::Number, public Component {
 public:
  SaveVTRNumber(SaveVTRClimate *parent, uint16_t reg) : parent_(parent), reg_(reg) {}

 protected:
  void control(float value) override;
  SaveVTRClimate *parent_{nullptr};
  uint16_t reg_{0};
};
#endif

#ifdef USE_SELECT
/// User mode select; same value mapping as the climate custom fan modes.
class SaveVTRUserModeSelect : public select::Select, public Component {
 public:
  explicit SaveVTRUserModeSelect(SaveVTRClimate *parent) : parent_(parent) {}

 protected:
  void control(const std::string &value) override;
  SaveVTRClimate *parent_{nullptr};
};
#endif

#ifdef USE_BUTTON
/// One-shot button that requests a user mode change.
class SaveVTRButton : public button::Button, public Component {
 public:
  SaveVTRButton(SaveVTRClimate *parent, uint8_t action_id) : parent_(parent), action_id_(action_id) {}

 protected:
  void press_action() override;
  SaveVTRClimate *parent_{nullptr};
  uint8_t action_id_{0};
};
#endif

/// Climate entity for the Systemair SAVE VTR ventilation unit over Modbus.
///
/// Reads are batched per contiguous register range and dispatched through
/// modbus_controller::ControllerDevice; every read completes asynchronously
/// (response, exception, timeout or drop) so the pending-request counter
/// always drains and values are published as they arrive - never from a
/// timer snapshot.
class SaveVTRClimate : public climate::Climate,
                       public PollingComponent,
                       public modbus_controller::ControllerDevice {
 public:
  void setup() override;
  /// UI cadence: temperatures, setpoint, user mode, heat demand, airflow,
  /// RPM, output utilisation and the 5 summary alarm flags.
  void update() override;
  /// Alarm cadence (own interval): the 32 detail alarm registers + filter
  /// warning counter. Also triggered immediately when a summary alarm flag
  /// changes, so a new alarm is described within one UI cycle.
  void update_alarms();

  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  void dump_config() override;

  void set_modbus(modbus_controller::ModbusController *modbus);
  void set_alarm_update_interval(uint32_t interval_ms) { this->alarm_update_interval_ = interval_ms; }
  void set_max_airflow(float max_airflow) { this->max_airflow_ = max_airflow; }

  // Write helpers used by the child entities
  bool write_holding_register(uint16_t address, uint16_t value);
  bool request_user_mode(uint16_t mode_value);
  void trigger_button_action(uint8_t action_id);

#ifdef USE_SENSOR
  void set_saf_percent_sensor(sensor::Sensor *s) { this->saf_percent_sensor_ = s; }
  void set_saf_volume_sensor(sensor::Sensor *s) { this->saf_volume_sensor_ = s; }
  void set_eaf_percent_sensor(sensor::Sensor *s) { this->eaf_percent_sensor_ = s; }
  void set_eaf_volume_sensor(sensor::Sensor *s) { this->eaf_volume_sensor_ = s; }
  void set_heat_demand_sensor(sensor::Sensor *s) { this->heat_demand_sensor_ = s; }
  void set_outdoor_air_temp_sensor(sensor::Sensor *s) { this->outdoor_air_temp_sensor_ = s; }
  void set_supply_air_temp_sensor(sensor::Sensor *s) { this->supply_air_temp_sensor_ = s; }
  void set_extract_air_temp_sensor(sensor::Sensor *s) { this->extract_air_temp_sensor_ = s; }
  void set_rpm_saf_sensor(sensor::Sensor *s) { this->rpm_saf_sensor_ = s; }
  void set_rpm_eaf_sensor(sensor::Sensor *s) { this->rpm_eaf_sensor_ = s; }
  void set_heat_exchanger_utilisation_sensor(sensor::Sensor *s) {
    this->heat_exchanger_utilisation_sensor_ = s;
  }
  void set_heater_utilisation_sensor(sensor::Sensor *s) { this->heater_utilisation_sensor_ = s; }
  void set_cooler_utilisation_sensor(sensor::Sensor *s) { this->cooler_utilisation_sensor_ = s; }
  void set_thermal_efficiency_sensor(sensor::Sensor *s) { this->thermal_efficiency_sensor_ = s; }
  void set_pdm_rhs_sensor(sensor::Sensor *s) { this->pdm_rhs_sensor_ = s; }
  void set_alarm_filter_warning_counter_sensor(sensor::Sensor *s) {
    this->alarm_filter_warning_counter_sensor_ = s;
  }
  void set_active_alarm_count_sensor(sensor::Sensor *s) { this->active_alarm_count_sensor_ = s; }
#endif

#ifdef USE_BINARY_SENSOR
  void set_alarm_binary_sensor(size_t idx, binary_sensor::BinarySensor *s) {
    if (idx < ALARM_BINARY_SENSOR_COUNT)
      this->alarm_binary_sensors_[idx] = s;
  }
  void set_any_alarm_active_binary_sensor(binary_sensor::BinarySensor *s) {
    this->any_alarm_active_binary_sensor_ = s;
  }
  void set_filter_warning_active_binary_sensor(binary_sensor::BinarySensor *s) {
    this->filter_warning_active_binary_sensor_ = s;
  }
  void set_eco_function_active_binary_sensor(binary_sensor::BinarySensor *s) {
    this->eco_function_active_binary_sensor_ = s;
  }
  void set_heater_active_binary_sensor(binary_sensor::BinarySensor *s) { this->heater_active_binary_sensor_ = s; }
  void set_heat_exchanger_active_binary_sensor(binary_sensor::BinarySensor *s) {
    this->heat_exchanger_active_binary_sensor_ = s;
  }
#endif

#ifdef USE_TEXT_SENSOR
  void set_user_mode_text_sensor(text_sensor::TextSensor *s) { this->user_mode_text_sensor_ = s; }
  void set_system_status_text_sensor(text_sensor::TextSensor *s) { this->system_status_text_sensor_ = s; }
  void set_alarm_summary_text_sensor(text_sensor::TextSensor *s) { this->alarm_summary_text_sensor_ = s; }
  void set_alarm_text_sensor(size_t idx, text_sensor::TextSensor *s) {
    if (idx < ALARM_TEXT_SENSOR_COUNT)
      this->alarm_text_sensors_[idx] = s;
  }
#endif

#ifdef USE_SWITCH
  void set_eco_mode_switch(SaveVTRSwitch *s) { this->eco_mode_switch_ = s; }
  void set_moisture_transfer_switch(SaveVTRSwitch *s) { this->moisture_transfer_switch_ = s; }
#endif

#ifdef USE_NUMBER
  void set_filter_period_number(SaveVTRNumber *n) { this->filter_period_number_ = n; }
  void set_holiday_time_number(SaveVTRNumber *n) { this->holiday_time_number_ = n; }
  void set_away_time_number(SaveVTRNumber *n) { this->away_time_number_ = n; }
  void set_fireplace_time_number(SaveVTRNumber *n) { this->fireplace_time_number_ = n; }
  void set_refresh_time_number(SaveVTRNumber *n) { this->refresh_time_number_ = n; }
  void set_crowded_time_number(SaveVTRNumber *n) { this->crowded_time_number_ = n; }
#endif

#ifdef USE_SELECT
  void set_user_mode_select(SaveVTRUserModeSelect *s) { this->user_mode_select_ = s; }
#endif

#ifdef USE_BUTTON
  void register_button(SaveVTRButton *b) { this->buttons_.push_back(b); }
#endif

 protected:
  // ControllerDevice::on_response()/on_error() only track online state; they
  // do not forward to the typed callbacks below. Forward explicitly (as
  // modbus_controller's WriterDevice does), or no reply ever reaches us.
  void on_response(std::span<const uint8_t> request_pdu, std::span<const uint8_t> response_pdu) override;
  void on_error(std::span<const uint8_t> request_pdu, modbus::ExceptionCode exception_code) override;

  // ControllerDevice terminal hooks: exactly one fires per accepted read.
  void on_read_registers(modbus::EntityType entity_type, uint16_t start_address,
                         std::span<const uint16_t> registers, modbus::ResponseStatus status) override;
  bool on_no_response(std::span<const uint8_t> request_pdu) override;
  void on_not_sent(std::span<const uint8_t> request_pdu) override;
  void on_write_single_register(uint16_t address, uint16_t value, modbus::ResponseStatus status) override;

  bool queue_read_registers_(modbus::EntityType entity_type, uint16_t start_address, uint16_t count);
  void handle_registers_read_(modbus::EntityType entity_type, uint16_t start_address,
                              std::span<const uint16_t> registers);
  void complete_read_request_();
  void finalize_cycle_();
  void publish_user_mode_(uint16_t raw);
  void update_climate_action_();
  void update_thermal_efficiency_();
  void update_alarm_binary_state_(size_t idx, uint16_t raw);
  void update_alarm_text_state_(size_t idx, uint16_t raw);
  void correlate_alarms_();
  bool has_any_alarm_entity_() const;
  bool needs_alarm_detail_scan_() const;

  modbus_controller::ModbusController *modbus_{nullptr};
  uint32_t alarm_update_interval_{300000};
  float max_airflow_{300.0f};

  /// Number of reads queued in the current cycle that have not reached a
  /// terminal callback yet. A new cycle is not queued until it is 0.
  uint16_t pending_reads_{0};
  bool climate_dirty_{false};
  bool alarm_rescan_requested_{false};
  bool alarm_scan_done_{false};
  uint32_t last_alarm_scan_ms_{0};;

  // Cached state values
  float heat_demand_percent_{0.0f};
  float saf_percent_{0.0f};
  float saf_volume_{0.0f};
  float eaf_percent_{0.0f};
  float eaf_volume_{0.0f};
  float outdoor_air_temp_{NAN};
  float supply_air_temp_{NAN};
  float extract_air_temp_{NAN};
  float rpm_saf_{0.0f};
  float rpm_eaf_{0.0f};
  float heater_utilisation_{0.0f};
  float cooler_utilisation_{0.0f};
  float heat_exchanger_utilisation_{0.0f};
  float thermal_efficiency_{NAN};
  float pdm_rhs_{NAN};

  // Alarm correlation state (raw register values cached; 0xFF = unknown)
  uint8_t alarm_text_states_[ALARM_TEXT_SENSOR_COUNT]{};
  uint8_t alarm_text_published_[ALARM_TEXT_SENSOR_COUNT]{};
  uint8_t alarm_binary_states_[ALARM_BINARY_SENSOR_COUNT]{};
  int last_active_alarm_count_{-1};
  std::string last_system_status_{};
  std::string last_alarm_summary_{};

#ifdef USE_SENSOR
  sensor::Sensor *saf_percent_sensor_{nullptr};
  sensor::Sensor *saf_volume_sensor_{nullptr};
  sensor::Sensor *eaf_percent_sensor_{nullptr};
  sensor::Sensor *eaf_volume_sensor_{nullptr};
  sensor::Sensor *heat_demand_sensor_{nullptr};
  sensor::Sensor *outdoor_air_temp_sensor_{nullptr};
  sensor::Sensor *supply_air_temp_sensor_{nullptr};
  sensor::Sensor *extract_air_temp_sensor_{nullptr};
  sensor::Sensor *rpm_saf_sensor_{nullptr};
  sensor::Sensor *rpm_eaf_sensor_{nullptr};
  sensor::Sensor *heat_exchanger_utilisation_sensor_{nullptr};
  sensor::Sensor *heater_utilisation_sensor_{nullptr};
  sensor::Sensor *cooler_utilisation_sensor_{nullptr};
  sensor::Sensor *thermal_efficiency_sensor_{nullptr};
  sensor::Sensor *pdm_rhs_sensor_{nullptr};
  sensor::Sensor *alarm_filter_warning_counter_sensor_{nullptr};
  sensor::Sensor *active_alarm_count_sensor_{nullptr};
#endif

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *alarm_binary_sensors_[ALARM_BINARY_SENSOR_COUNT]{};
  binary_sensor::BinarySensor *any_alarm_active_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *filter_warning_active_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *eco_function_active_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *heater_active_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *heat_exchanger_active_binary_sensor_{nullptr};
#endif

#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *user_mode_text_sensor_{nullptr};
  text_sensor::TextSensor *system_status_text_sensor_{nullptr};
  text_sensor::TextSensor *alarm_summary_text_sensor_{nullptr};
  text_sensor::TextSensor *alarm_text_sensors_[ALARM_TEXT_SENSOR_COUNT]{};
#endif

#ifdef USE_SWITCH
  SaveVTRSwitch *eco_mode_switch_{nullptr};
  SaveVTRSwitch *moisture_transfer_switch_{nullptr};
#endif

#ifdef USE_NUMBER
  SaveVTRNumber *filter_period_number_{nullptr};
  SaveVTRNumber *holiday_time_number_{nullptr};
  SaveVTRNumber *away_time_number_{nullptr};
  SaveVTRNumber *fireplace_time_number_{nullptr};
  SaveVTRNumber *refresh_time_number_{nullptr};
  SaveVTRNumber *crowded_time_number_{nullptr};
#endif

#ifdef USE_SELECT
  SaveVTRUserModeSelect *user_mode_select_{nullptr};
#endif

#ifdef USE_BUTTON
  std::vector<SaveVTRButton *> buttons_;
#endif
};

}  // namespace save_vtr
}  // namespace esphome