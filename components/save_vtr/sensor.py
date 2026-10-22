import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLUME_FLOW_RATE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ENTITY_CATEGORY_NONE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_CELSIUS,
    UNIT_CUBIC_METER_PER_HOUR,
    UNIT_EMPTY,
    UNIT_PERCENT,
    UNIT_REVOLUTIONS_PER_MINUTE,
)

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

CONF_SAF_PERCENT = "saf_percent"
CONF_SAF_VOLUME = "saf_volume"
CONF_EAF_PERCENT = "eaf_percent"
CONF_EAF_VOLUME = "eaf_volume"
CONF_HEAT_DEMAND = "heat_demand"
CONF_HEAT_DEMAND_PERCENT = "heat_demand_percent"
CONF_OUTDOOR_AIR_TEMP = "outdoor_air_temp"
CONF_SUPPLY_AIR_TEMP = "supply_air_temp"
CONF_EXTRACT_AIR_TEMP = "extract_air_temp"
CONF_RPM_SAF = "rpm_saf"
CONF_RPM_EAF = "rpm_eaf"
CONF_HEAT_EXCHANGER_UTILISATION = "heat_exchanger_utilisation"
CONF_HEATER_UTILISATION = "heater_utilisation"
CONF_COOLER_UTILISATION = "cooler_utilisation"
CONF_THERMAL_EFFICIENCY = "thermal_efficiency"
CONF_PDM_RHS = "pdm_rhs"
CONF_ALARM_FILTER_WARNING_COUNTER = "alarm_filter_warning_counter"
CONF_ACTIVE_ALARM_COUNT = "active_alarm_count"

# (conf_key, setter_name, unit, icon, decimals, device_class, state_class, entity_category)
SENSORS = [
    (
        CONF_SAF_PERCENT,
        "set_saf_percent_sensor",
        UNIT_PERCENT,
        "mdi:fan",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_SAF_VOLUME,
        "set_saf_volume_sensor",
        UNIT_CUBIC_METER_PER_HOUR,
        "mdi:fan",
        1,
        DEVICE_CLASS_VOLUME_FLOW_RATE,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_EAF_PERCENT,
        "set_eaf_percent_sensor",
        UNIT_PERCENT,
        "mdi:fan",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_EAF_VOLUME,
        "set_eaf_volume_sensor",
        UNIT_CUBIC_METER_PER_HOUR,
        "mdi:fan",
        1,
        DEVICE_CLASS_VOLUME_FLOW_RATE,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_HEAT_DEMAND,
        "set_heat_demand_sensor",
        UNIT_PERCENT,
        "mdi:fire",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_HEAT_DEMAND_PERCENT,
        "set_heat_demand_sensor",
        UNIT_PERCENT,
        "mdi:fire",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_OUTDOOR_AIR_TEMP,
        "set_outdoor_air_temp_sensor",
        UNIT_CELSIUS,
        "mdi:thermometer",
        1,
        DEVICE_CLASS_TEMPERATURE,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_SUPPLY_AIR_TEMP,
        "set_supply_air_temp_sensor",
        UNIT_CELSIUS,
        "mdi:thermometer",
        1,
        DEVICE_CLASS_TEMPERATURE,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_EXTRACT_AIR_TEMP,
        "set_extract_air_temp_sensor",
        UNIT_CELSIUS,
        "mdi:thermometer",
        1,
        DEVICE_CLASS_TEMPERATURE,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_RPM_SAF,
        "set_rpm_saf_sensor",
        UNIT_REVOLUTIONS_PER_MINUTE,
        "mdi:fan",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    (
        CONF_RPM_EAF,
        "set_rpm_eaf_sensor",
        UNIT_REVOLUTIONS_PER_MINUTE,
        "mdi:fan",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    (
        CONF_HEAT_EXCHANGER_UTILISATION,
        "set_heat_exchanger_utilisation_sensor",
        UNIT_PERCENT,
        "mdi:heat-wave",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_HEATER_UTILISATION,
        "set_heater_utilisation_sensor",
        UNIT_PERCENT,
        "mdi:radiator",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_COOLER_UTILISATION,
        "set_cooler_utilisation_sensor",
        UNIT_PERCENT,
        "mdi:snowflake",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_THERMAL_EFFICIENCY,
        "set_thermal_efficiency_sensor",
        UNIT_PERCENT,
        "mdi:percent",
        1,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_PDM_RHS,
        "set_pdm_rhs_sensor",
        UNIT_PERCENT,
        "mdi:water-percent",
        0,
        DEVICE_CLASS_HUMIDITY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_NONE,
    ),
    (
        CONF_ALARM_FILTER_WARNING_COUNTER,
        "set_alarm_filter_warning_counter_sensor",
        UNIT_EMPTY,
        "mdi:counter",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_TOTAL_INCREASING,
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    (
        CONF_ACTIVE_ALARM_COUNT,
        "set_active_alarm_count_sensor",
        UNIT_EMPTY,
        "mdi:bell-alert-outline",
        0,
        DEVICE_CLASS_EMPTY,
        STATE_CLASS_MEASUREMENT,
        ENTITY_CATEGORY_DIAGNOSTIC,
    ),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(name): sensor.sensor_schema(
            unit_of_measurement=unit,
            icon=icon,
            accuracy_decimals=decimals,
            device_class=device_class,
            state_class=state_class,
            entity_category=entity_category,
        )
        for name, _, unit, icon, decimals, device_class, state_class, entity_category in SENSORS
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    for name, setter, _, _, _, _, _, _ in SENSORS:
        if name in config:
            sens = await sensor.new_sensor(config[name])
            cg.add(getattr(paren, setter)(sens))
