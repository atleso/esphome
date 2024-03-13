import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

CONF_M5STACK420MA_ID = 'm5stack420ma_id'
CONF_ENABLE_12BIT_MODE = 'enable_12bit_mode'

m5stack420ma_ns = cg.esphome_ns.namespace('m5stack420ma')
M5Stack420MASensor = m5stack420ma_ns.class_('M5Stack420MASensor', cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = sensor.sensor_schema(M5Stack420MASensor, cg.PollingComponent).extend({
    cv.GenerateID(): cv.declare_id(M5Stack420MASensor),
    cv.Optional(CONF_ENABLE_12BIT_MODE, default=False): cv.boolean,
}).extend(cv.polling_component_schema('60s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    cg.add(var.set_enable_12bit_mode(config[CONF_ENABLE_12BIT_MODE]))