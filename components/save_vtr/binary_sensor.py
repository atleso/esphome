import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import save_vtr_ns, SaveVTRClimate


DEPENDENCIES = ["save_vtr"]

# Ordered list of 0-1/Boolean alarm binary sensors (index must match ALARM_BINARY_REGS in save_vtr.cpp)
ALARM_BINARY_SENSORS = [
    ("filter_alarm_was_detected", 0),
    ("output_alarm", 1),
    ("alarm_type_a", 2),
    ("alarm_type_b", 3),
    ("alarm_type_c", 4),
]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(SaveVTRClimate),
    **{
        cv.Optional(name): binary_sensor.binary_sensor_schema(
            device_class="problem",
        )
        for name, _ in ALARM_BINARY_SENSORS
    },
})


async def to_code(config):
    paren = await cg.get_variable(config[CONF_ID])
    for name, idx in ALARM_BINARY_SENSORS:
        if name in config:
            bs = await binary_sensor.new_binary_sensor(config[name])
            cg.add(paren.set_alarm_binary_sensor(idx, bs))
