import esphome.codegen as cg
from esphome.components import climate
from esphome.components.modbus_controller import (
    CONF_MODBUS_CONTROLLER_ID,
    ModbusController,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

from . import SaveVTRClimate

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["modbus_controller"]
AUTO_LOAD = ["save_vtr"]

CONF_MODBUS_ID = "modbus_id"
CONF_ALARM_UPDATE_INTERVAL = "alarm_update_interval"
CONF_MAX_AIRFLOW = "max_airflow"


def _validate_modbus_id(config):
    if CONF_MODBUS_ID not in config and CONF_MODBUS_CONTROLLER_ID not in config:
        raise cv.Invalid(
            f"Either '{CONF_MODBUS_ID}' or '{CONF_MODBUS_CONTROLLER_ID}' is required"
        )
    return config


CONFIG_SCHEMA = cv.All(
    climate.climate_schema(SaveVTRClimate).extend(
        {
            cv.Optional(CONF_MODBUS_ID): cv.use_id(ModbusController),
            cv.Optional(CONF_MODBUS_CONTROLLER_ID): cv.use_id(ModbusController),
            cv.Optional(CONF_UPDATE_INTERVAL, default="10s"): cv.update_interval,
            cv.Optional(CONF_ALARM_UPDATE_INTERVAL, default="300s"): cv.update_interval,
            cv.Optional(CONF_MAX_AIRFLOW, default=300.0): cv.positive_float,
        }
    ),
    _validate_modbus_id,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    modbus_id = config.get(CONF_MODBUS_ID) or config[CONF_MODBUS_CONTROLLER_ID]
    modbus = await cg.get_variable(modbus_id)
    cg.add(var.set_modbus(modbus))
    cg.add(var.set_alarm_update_interval(config[CONF_ALARM_UPDATE_INTERVAL]))
    cg.add(var.set_max_airflow(config[CONF_MAX_AIRFLOW]))
