import esphome.codegen as cg
from esphome.components import climate

save_vtr_ns = cg.esphome_ns.namespace("save_vtr")
SaveVTRClimate = save_vtr_ns.class_("SaveVTRClimate", climate.Climate, cg.PollingComponent)

CODEOWNERS = ["@atleso"]


# Allow save_vtr: block in YAML (required for dependency)
import esphome.config_validation as cv

CONFIG_SCHEMA = cv.Schema({})

async def to_code(config):
    pass

AUTO_LOAD = ["climate", "sensor"]


