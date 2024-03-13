import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY, CONF_UNIT_OF_MEASUREMENT, CONF_ICON, CONF_ACCURACY_DECIMALS, CONF_UPDATE_INTERVAL

DEPENDENCIES = ['i2c']

CONF_CURRENT_VALUE = "current_value"
CONF_RAW_ADC = "raw_adc"


m5stack420ma_ns = cg.esphome_ns.namespace('m5stack420ma')
M5Stack420MASensor = m5stack420ma_ns.class_('M5Stack420MASensor', cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(): cv.declare_id(M5Stack420MASensor),
    cv.Optional(CONF_CURRENT_VALUE): sensor.sensor_schema(unit_of_measurement=UNIT_EMPTY, icon=ICON_EMPTY, accuracy_decimals=2).extend({
        cv.GenerateID(): cv.declare_id(sensor.Sensor),
    }),
    cv.Optional(CONF_RAW_ADC): sensor.sensor_schema(unit_of_measurement=UNIT_EMPTY, icon=ICON_EMPTY, accuracy_decimals=2).extend({
        cv.GenerateID(): cv.declare_id(sensor.Sensor),
    }),
}).extend(i2c.i2c_device_schema(0x55))  # Assuming 0x55 is the default I2C address

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)
    yield i2c.register_i2c_device(var, config)

    if CONF_CURRENT_VALUE in config:
        sensor_ = yield sensor.new_sensor(config[CONF_CURRENT_VALUE])
        cg.add(var.set_current_sensor(sensor_))
    if CONF_RAW_ADC in config:
        sensor_ = yield sensor.new_sensor(config[CONF_RAW_ADC])
        cg.add(var.set_raw_adc_sensor(sensor_))