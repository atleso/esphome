
import esphome.codegen as cg
from esphome.components import climate
from . import sensor as sensor_platform

save_vtr_ns = cg.esphome_ns.namespace("save_vtr")
SaveVTRClimate = save_vtr_ns.class_("SaveVTRClimate", climate.Climate, cg.PollingComponent)

CODEOWNERS = ["@atleso"]

# Ensure both platforms are auto-loaded
AUTO_LOAD = ["climate", "sensor"]


