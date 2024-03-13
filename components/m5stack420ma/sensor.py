import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

DEPENDENCIES = ['i2c']

CONF_I2C_ADDR = 0x55
CONF_ENABLE_12BIT_MODE = 'enable_12bit_mode'
CONF_UPDATE_INTERVAL = 'update_interval'

m5stack420ma_ns = cg.esphome_ns.namespace('m5stack420ma')
M5Stack420MASensor = m5stack420ma_ns.class_('M5Stack420MASensor', cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 1).extend({
    cv.GenerateID(): cv.declare_id(M5Stack420MASensor),
    cv.Optional(CONF_ENABLE_12BIT_MODE, default=True): cv.boolean,
    cv.Optional(CONF_UPDATE_INTERVAL): cv.update_interval,
}).extend(cv.polling_component_schema('5s')).extend(i2c.i2c_device_schema(CONF_I2C_ADDR))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    yield i2c.register_i2c_device(var, config)