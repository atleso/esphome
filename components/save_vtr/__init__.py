
import esphome.codegen as cg
from esphome.components import climate

save_vtr_ns = cg.esphome_ns.namespace("save_vtr")
SaveVTRClimate = save_vtr_ns.class_("SaveVTRClimate", climate.Climate, cg.PollingComponent)

CODEOWNERS = ["@atleso"]

# Ensure both platforms are auto-loaded
AUTO_LOAD = ["climate", "sensor"]
