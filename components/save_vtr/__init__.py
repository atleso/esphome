import esphome.codegen as cg
from esphome.components import climate, modbus_controller
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@atleso"]

save_vtr_ns = cg.esphome_ns.namespace("save_vtr")
SaveVTRClimate = save_vtr_ns.class_(
    "SaveVTRClimate",
    climate.Climate,
    cg.PollingComponent,
    modbus_controller.modbus_controller_ns.class_("ControllerDevice"),
)

CONF_SAVE_VTR_ID = "save_vtr_id"

# Shared schema for the satellite platforms (sensor, binary_sensor, text_sensor,
# switch, number, select, button). The parent climate is referenced either
# through an explicit `save_vtr_id:` or, as in the existing configs for this
# component, implicitly through the platform block's `id:` key.
SAVE_VTR_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(SaveVTRClimate),
        cv.Optional(CONF_SAVE_VTR_ID): cv.use_id(SaveVTRClimate),
    }
)


async def get_save_vtr_parent(config):
    parent_id = config.get(CONF_SAVE_VTR_ID, config[CONF_ID])
    return await cg.get_variable(parent_id)


# Optional top-level `save_vtr:` block (backwards compatibility)
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass