#include "esphome.h"
#include "esphome/core/log.h"
#include "m5stack420ma.h"




namespace esphome {
namespace m5stack420ma {

static const char *TAG = "m5stack420ma.sensor";

void M5Stack420MASensor::setup(){
  ESP_LOGI(TAG, "M5Stack 4-20mA sensor setup");
  // Initialize the I2C device
  this->set_i2c_address(kAddress);    
}

void M5Stack420MASensor::update(){
  // Assuming you want to read from channel 0
  uint16_t current_value = this->read_current(0);
  ESP_LOGD(TAG, "Read current value: %u", current_value);

  // Publish the current value
  if (current_sensor != nullptr) {
    current_sensor->publish_state(current_value);
  }
}

void M5Stack420MASensor::dump_config(){
  ESP_LOGCONFIG(TAG, "M5Stack 4-20mA Sensor:");
  // Additional config values can be logged here
}

uint16_t M5Stack420MASensor::read_current(uint8_t channel) {
  // Adjust according to how the MODULE_4_20MA sensor encodes its current values
  // Here's an example reading a 16-bit current value from the specified channel
  uint8_t reg = MODULE_4_20MA_CURRENT_REG;
  uint8_t data[2] = {0};
  if (!this->read_bytes(reg, data, 2)) {
    ESP_LOGW(TAG, "Failed to read current value");
    return 0;
  }
  uint16_t current = (uint16_t(data[0]) << 8) | uint16_t(data[1]);
  return current;
}

}  // namespace EmptyI2CSensor
}  // namespace esphome


