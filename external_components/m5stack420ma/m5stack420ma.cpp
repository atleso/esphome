#include "esphome.h"
#include "MODULE_4_20MA.h"

class M5Stack420MASensor : public PollingComponent, public Sensor {
 public:
  MODULE_4_20MA m5stack420ma;
  void setup() override {
    // Initialization with custom SDA, SCL pins if specified in YAML
    m5stack420ma.begin(&Wire, MODULE_4_20MA_ADDR, custom_sda_pin, custom_scl_pin);
  }

  void update() override {
    // Example: Read current value from channel 0
    uint16_t current_value = m5stack420ma.getCurrentValue(0);
    publish_state(current_value);
  }

  void set_pins(uint8_t sda_pin, uint8_t scl_pin) {
    this->custom_sda_pin = sda_pin;
    this->custom_scl_pin = scl_pin;
  }

 private:
  uint8_t custom_sda_pin = 1; // Default SDA pin
  uint8_t custom_scl_pin = 2; // Default SCL pin
};