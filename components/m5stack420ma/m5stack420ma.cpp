#include "esphome.h"
#include "MODULE_4_20MA.h"

class M5Stack420MASensor : public PollingComponent {
 public:
  // Create a sensor instance as a member, not by inheritance
  Sensor *current_sensor{nullptr};

  // Constructor for initializing the sensor with a polling interval
  M5Stack420MASensor() : PollingComponent(5000) {} // Polling every 5 seconds

  void set_sda_pin(uint8_t sda_pin) {
    this->custom_sda_pin = sda_pin;
  }

  void set_scl_pin(uint8_t scl_pin) {
    this->custom_scl_pin = scl_pin;
  }

  void setup() override {
    // Assuming the `begin` method configures the module correctly
    m5stack420ma.begin(&Wire, MODULE_4_20MA_ADDR, custom_sda_pin, custom_scl_pin);
  }

  void update() override {
    // Here you would read from your sensor and publish the value
    uint16_t current_value = m5stack420ma.getCurrentValue(0);
    if(current_sensor) {
      current_sensor->publish_state(current_value);
    }
  }

 private:
  MODULE_4_20MA m5stack420ma;
  uint8_t custom_sda_pin = 2; // Default SDA pin
  uint8_t custom_scl_pin = 1; // Default SCL pin
};