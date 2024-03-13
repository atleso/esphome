import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY, CONF_UNIT_OF_MEASUREMENT, CONF_ICON, CONF_ACCURACY_DECIMALS, CONF_UPDATE_INTERVAL

DEPENDENCIES = ['i2c']

m5stack420ma_ns = cg.esphome_ns.namespace('m5stack420ma')
M5Stack420MASensor = m5stack420ma_ns.class_('M5Stack420MASensor', cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(M5Stack420MASensor),
    cv.Optional(CONF_UPDATE_INTERVAL): cv.update_interval,
    cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=UNIT_EMPTY): cv.string,
    cv.Optional(CONF_ICON, default=ICON_EMPTY): cv.icon,
    cv.Optional(CONF_ACCURACY_DECIMALS, default=1): cv.int_range(min=0, max=5),
}).extend(i2c.i2c_device_schema(0x55))  # Assuming 0x55 is the default I2C address

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    yield i2c.register_i2c_device(var, config)
