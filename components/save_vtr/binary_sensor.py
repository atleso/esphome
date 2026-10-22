import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_NONE,
)

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

# Ordered list of 0-1/Boolean alarm binary sensors (index must match ALARM_BINARY_REGS in save_vtr.cpp)
ALARM_BINARY_SENSORS = [
    ("alarm_filter_was_detected", 0),
    ("alarm_output", 1),
    ("alarm_type_a", 2),
    ("alarm_type_b", 3),
    ("alarm_type_c", 4),
]

# Correlated & operational status binary sensors: (conf_key, setter_name, device_class, icon, entity_category)
STATUS_BINARY_SENSORS = [
    (
        "any_alarm_active",
        "set_any_alarm_active_binary_sensor",
        DEVICE_CLASS_PROBLEM,
        "mdi:alert-octagon-outline",
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    (
        "filter_warning_active",
        "set_filter_warning_active_binary_sensor",
        DEVICE_CLASS_PROBLEM,
        "mdi:air-filter",
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    (
        "eco_function_active",
        "set_eco_function_active_binary_sensor",
        DEVICE_CLASS_RUNNING,
        "mdi:leaf",
        ENTITY_CATEGORY_NONE,
    ),
    (
        "heater_active",
        "set_heater_active_binary_sensor",
        DEVICE_CLASS_HEAT,
        "mdi:radiator",
        ENTITY_CATEGORY_NONE,
    ),
    (
        "heat_exchanger_active",
        "set_heat_exchanger_active_binary_sensor",
        DEVICE_CLASS_RUNNING,
        "mdi:rotate-3d-variant",
        ENTITY_CATEGORY_NONE,
    ),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        **{
            cv.Optional(name): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            )
            for name, _ in ALARM_BINARY_SENSORS
        },
        **{
            cv.Optional(name): binary_sensor.binary_sensor_schema(
                device_class=device_class,
                icon=icon,
                entity_category=entity_category,
            )
            for name, _, device_class, icon, entity_category in STATUS_BINARY_SENSORS
        },
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    for name, idx in ALARM_BINARY_SENSORS:
        if name in config:
            bs = await binary_sensor.new_binary_sensor(config[name])
            cg.add(paren.set_alarm_binary_sensor(idx, bs))

    for name, setter, _, _, _ in STATUS_BINARY_SENSORS:
        if name in config:
            bs = await binary_sensor.new_binary_sensor(config[name])
            cg.add(getattr(paren, setter)(bs))
