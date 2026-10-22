import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_EMPTY,
    ENTITY_CATEGORY_CONFIG,
    UNIT_HOUR,
    UNIT_MINUTE,
)

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent, save_vtr_ns

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

SaveVTRNumber = save_vtr_ns.class_("SaveVTRNumber", number.Number, cg.Component)

UNIT_DAYS = "d"
UNIT_MONTHS = "months"

# (conf_key, setter_name, reg, min_val, max_val, step, unit, icon, device_class)
NUMBERS = [
    (
        "filter_period",
        "set_filter_period_number",
        7000,
        1.0,
        12.0,
        1.0,
        UNIT_MONTHS,
        "mdi:air-filter",
        DEVICE_CLASS_EMPTY,
    ),
    (
        "holiday_time",
        "set_holiday_time_number",
        1100,
        1.0,
        365.0,
        1.0,
        UNIT_DAYS,
        "mdi:palm-tree",
        DEVICE_CLASS_DURATION,
    ),
    (
        "away_time",
        "set_away_time_number",
        1101,
        1.0,
        72.0,
        1.0,
        UNIT_HOUR,
        "mdi:home-export-outline",
        DEVICE_CLASS_DURATION,
    ),
    (
        "fireplace_time",
        "set_fireplace_time_number",
        1102,
        1.0,
        60.0,
        1.0,
        UNIT_MINUTE,
        "mdi:fireplace",
        DEVICE_CLASS_DURATION,
    ),
    (
        "refresh_time",
        "set_refresh_time_number",
        1103,
        1.0,
        240.0,
        1.0,
        UNIT_MINUTE,
        "mdi:weather-windy",
        DEVICE_CLASS_DURATION,
    ),
    (
        "crowded_time",
        "set_crowded_time_number",
        1104,
        1.0,
        8.0,
        1.0,
        UNIT_HOUR,
        "mdi:account-group",
        DEVICE_CLASS_DURATION,
    ),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(name): number.number_schema(
            SaveVTRNumber,
            unit_of_measurement=unit,
            icon=icon,
            device_class=device_class,
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
        for name, _, _, _, _, _, unit, icon, device_class in NUMBERS
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    for name, setter, reg, min_val, max_val, step, _, _, _ in NUMBERS:
        if name in config:
            num = await number.new_number(
                config[name],
                paren,
                reg,
                min_value=min_val,
                max_value=max_val,
                step=step,
            )
            await cg.register_component(num, config[name])
            cg.add(getattr(paren, setter)(num))