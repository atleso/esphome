import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC, ENTITY_CATEGORY_NONE

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

CONF_USER_MODE = "user_mode"
CONF_SYSTEM_STATUS = "system_status"
CONF_ALARM_SUMMARY = "alarm_summary"

# Ordered list of 0-3 alarm text sensors (index must match ALARM_TEXT_REGS in save_vtr.cpp)
ALARM_TEXT_SENSORS = [
    ("alarm_saf_ctrl", 0),
    ("alarm_eaf_ctrl", 1),
    ("alarm_frost_prot", 2),
    ("alarm_defrosting", 3),
    ("alarm_saf_rpm", 4),
    ("alarm_eaf_rpm", 5),
    ("alarm_fpt", 6),
    ("alarm_oat", 7),
    ("alarm_sat", 8),
    ("alarm_rat", 9),
    ("alarm_eat", 10),
    ("alarm_ect", 11),
    ("alarm_eft", 12),
    ("alarm_oht", 13),
    ("alarm_emt", 14),
    ("alarm_rgs", 15),
    ("alarm_bys", 16),
    ("alarm_secondary_air", 17),
    ("alarm_filter", 18),
    ("alarm_extra_controller", 19),
    ("alarm_external_stop", 20),
    ("alarm_rh", 21),
    ("alarm_co2", 22),
    ("alarm_low_sat", 23),
    ("alarm_byf", 24),
    ("alarm_manual_override_outputs", 25),
    ("alarm_pdm_rhs", 26),
    ("alarm_pdm_eat", 27),
    ("alarm_manual_fan_stop", 28),
    ("alarm_overheat_temperature", 29),
    ("alarm_fire", 30),
    ("alarm_filter_warning", 31),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(CONF_USER_MODE): text_sensor.text_sensor_schema(
            icon="mdi:fan-auto",
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_SYSTEM_STATUS): text_sensor.text_sensor_schema(
            icon="mdi:shield-check-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_ALARM_SUMMARY): text_sensor.text_sensor_schema(
            icon="mdi:bell-alert-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        **{
            cv.Optional(name): text_sensor.text_sensor_schema(
                icon="mdi:alert-circle-outline",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            )
            for name, _ in ALARM_TEXT_SENSORS
        },
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    if CONF_USER_MODE in config:
        ts = await text_sensor.new_text_sensor(config[CONF_USER_MODE])
        cg.add(paren.set_user_mode_text_sensor(ts))

    if CONF_SYSTEM_STATUS in config:
        ts = await text_sensor.new_text_sensor(config[CONF_SYSTEM_STATUS])
        cg.add(paren.set_system_status_text_sensor(ts))

    if CONF_ALARM_SUMMARY in config:
        ts = await text_sensor.new_text_sensor(config[CONF_ALARM_SUMMARY])
        cg.add(paren.set_alarm_summary_text_sensor(ts))

    for name, idx in ALARM_TEXT_SENSORS:
        if name in config:
            ts = await text_sensor.new_text_sensor(config[name])
            cg.add(paren.set_alarm_text_sensor(idx, ts))
