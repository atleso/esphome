#include "esphome.h"
#include "esphome/core/log.h"
#include "m5stack420ma.h"




namespace esphome {
namespace m5stack420ma {

static const char *TAG = "m5stack420ma.sensor";

void M5Stack420MASensor::setup(){
  ESP_LOGCONFIG(TAG, "Setting up M5Stack 4-20mA Sensor...");
  // Attempt to communicate with the sensor to ensure it's responding
  uint8_t data[2];
  if (!this->read_bytes(0x0, data, 1)) {
    ESP_LOGE(TAG, "Failed to communicate with M5Stack 4-20mA sensor.");
    mark_failed();
  }
  ESP_LOGD(TAG, "Setup Status %u", data);
}

void M5Stack420MASensor::update(){
  // Assuming you want to read from channel 0
  float current_value = this->read_current(0);
  uint16_t adc_12bit_value = this->read_adc_12bit(0);
  ESP_LOGD(TAG, "Read current value: %u", current_value);
  ESP_LOGD(TAG, "Read adc value: %u", adc_12bit_value);

  // Publish the current value
  if (current_sensor_ != nullptr) {
    current_sensor_->publish_state(current_value);
  }
  if (raw_adc_sensor_ != nullptr) {
    raw_adc_sensor_->publish_state(adc_12bit_value);
  }
}

void M5Stack420MASensor::dump_config(){
  ESP_LOGCONFIG(TAG, "M5Stack 4-20mA Sensor:");
  // Additional config values can be logged here
}



float M5Stack420MASensor::read_current(uint8_t channel) {
  uint8_t reg = MODULE_4_20MA_CURRENT_REG;
  uint8_t data[2] = {0};
  if (!this->read_bytes(reg, data, 2)) {
    ESP_LOGW(TAG, "Failed to read current value");
    return 0;
  }
  uint16_t current_raw = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  float current = current_raw / 100.0;
  ESP_LOGD(TAG, "Raw current: %u, Converted current: %.2f mA", current_raw, current);
  return current;
}

uint16_t M5Stack420MASensor::read_adc_12bit(uint8_t channel) {
  uint8_t reg = MODULE_4_20MA_ADC_12BIT_REG;
  uint8_t data[2] = {0};
  if (!this->read_bytes(reg, data, 2)) {
    ESP_LOGW(TAG, "Failed to read raw adc value");
    return 0;
  }
  uint16_t adc = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  return adc;
}

void M5Stack420MASensor::calibrate(uint16_t calibration_value) {
    uint8_t data[2];
    data[0] = calibration_value & 0xFF;        // Low byte
    data[1] = (calibration_value >> 8) & 0xFF; // High byte

    // Write to the calibration registers
    if (!this->write_bytes(0x30, data, 2)) {
        ESP_LOGW(TAG, "Failed to write calibration value");
    } else {
        ESP_LOGD(TAG, "Calibration successful, written value: %u", calibration_value);
    }
}

void M5Stack420MASensor::test() {
    ESP_LOGD(TAG, "Test method called successfully");
}

}  // namespace EmptyI2CSensor
}  // namespace esphome


