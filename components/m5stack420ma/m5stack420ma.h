#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"


#define MODULE_4_20MA_ADDR          0x55
#define MODULE_4_20MA_ADC_12BIT_REG 0x00
#define MODULE_4_20MA_ADC_8BIT_REG  0x10
#define MODULE_4_20MA_CURRENT_REG   0x20
#define MODULE_4_20MA_CAL_REG       0x30
#define JUMP_TO_BOOTLOADER_REG      0xFD
#define FIRMWARE_VERSION_REG        0xFE
#define I2C_ADDRESS_REG             0xFF


namespace esphome {
namespace m5stack420ma {

class M5Stack420MASensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
  public:
    M5Stack420MASensor() = default;
    
    void set_current_sensor(sensor::Sensor *current_sensor) { this->current_sensor_ = current_sensor; }
    void set_raw_adc_sensor(sensor::Sensor *raw_adc_sensor) { this->raw_adc_sensor_ = raw_adc_sensor; }

    void calibrate(uint16_t calibration_value);

    void test();

    void setup() override;
    void update() override;
    void dump_config() override;
  
    float read_current(uint8_t channel);
    uint16_t read_adc_12bit(uint8_t channel);

  protected:
    sensor::Sensor *current_sensor_{nullptr};
    sensor::Sensor *raw_adc_sensor_{nullptr};


  private:
    static constexpr uint8_t kAddress = 0x55; // Default I2C address of the MODULE_4_20MA

    static constexpr uint8_t kCurrentRegBase = 0x20; // Register addresses based on MODULE_4_20MA.h
};

}  // namespace EmptyI2CSensor
}  // namespace esphome
