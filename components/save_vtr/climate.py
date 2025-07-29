import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.modbus_controller import ModbusController
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

DEPENDENCIES = ["modbus_controller"]

save_vtr_ns = cg.esphome_ns.namespace("save_vtr")
SaveVTRClimate = save_vtr_ns.class_("SaveVTRClimate", climate.Climate, cg.PollingComponent)

CONFIG_SCHEMA = climate.climate_schema(SaveVTRClimate).extend(
    {
        cv.Required("modbus_id"): cv.use_id(ModbusController),
        cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.update_interval,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    modbus = await cg.get_variable(config["modbus_id"])
    cg.add(var.set_modbus(modbus))
