import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_UNIT_OF_MEASUREMENT, CONF_ICON, CONF_ACCURACY_DECIMALS
from . import save_vtr_ns, SaveVTRClimate
import esphome.components.sensor as sensor_core


DEPENDENCIES = ["save_vtr"]

CONF_SAF_PERCENT = "saf_percent"
CONF_SAF_VOLUME = "saf_volume"
CONF_EAF_PERCENT = "eaf_percent"
CONF_EAF_VOLUME = "eaf_volume"
CONF_HEAT_DEMAND = "heat_demand"
CONF_OUTDOOR_AIR_TEMP = "outdoor_air_temp"
CONF_SUPPLY_AIR_TEMP = "supply_air_temp"
CONF_EXTRACT_AIR_TEMP = "extract_air_temp"

SENSORS = [
    (CONF_SAF_PERCENT, "%", "mdi:fan", 0),
    (CONF_SAF_VOLUME, "m³/h", "mdi:fan", 1),
    (CONF_EAF_PERCENT, "%", "mdi:fan", 0),
    (CONF_EAF_VOLUME, "m³/h", "mdi:fan", 1),
    (CONF_HEAT_DEMAND, "%", "mdi:fire", 0),
    (CONF_OUTDOOR_AIR_TEMP, "°C", "mdi:thermometer", 1),
    (CONF_SUPPLY_AIR_TEMP, "°C", "mdi:thermometer", 1),
    (CONF_EXTRACT_AIR_TEMP, "°C", "mdi:thermometer", 1),
]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(SaveVTRClimate),
    **{
        cv.Optional(name): sensor.sensor_schema(
            unit_of_measurement=unit,
            icon=icon,
            accuracy_decimals=decimals,
        )
        for name, unit, icon, decimals in SENSORS
    },
})

async def to_code(config):
    paren = await cg.get_variable(config[CONF_ID])
    for name, _, _, _ in SENSORS:
        if name in config:
            sens = await sensor.new_sensor(config[name])
            cg.add(getattr(paren, f"set_{name}_sensor")(sens))

