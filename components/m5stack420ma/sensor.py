import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

CONF_M5STACK420MA_ID = 'm5stack420ma_id'
CONF_ENABLE_12BIT_MODE = 'enable_12bit_mode'
CONF_SDA_PIN = 'sda_pin'
CONF_SCL_PIN = 'scl_pin'

m5stack420ma_ns = cg.esphome_ns.namespace('m5stack420ma')
M5Stack420MASensor = m5stack420ma_ns.class_('M5Stack420MASensor', cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(M5Stack420MASensor),
    cv.Optional(CONF_ENABLE_12BIT_MODE, default=False): cv.boolean,
    cv.Optional(CONF_UPDATE_INTERVAL): cv.update_interval,
    cv.Optional(CONF_SDA_PIN): cv.pin,
    cv.Optional(CONF_SCL_PIN): cv.pin,
}).extend(i2c.i2c_device_schema(0x55))  # Assuming 0x55 is the default I2C address

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    
    if CONF_ENABLE_12BIT_MODE in config:
        cg.add(var.set_enable_12bit_mode(config[CONF_ENABLE_12BIT_MODE]))
    
    if CONF_SDA_PIN in config:
        cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    if CONF_SCL_PIN in config:
        cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))