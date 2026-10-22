import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent, save_vtr_ns

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

SaveVTRSwitch = save_vtr_ns.class_("SaveVTRSwitch", switch.Switch, cg.Component)

# (conf_key, setter_name, register_address, icon)
SWITCHES = [
    ("eco_mode", "set_eco_mode_switch", 2505, "mdi:leaf"),
    ("moisture_transfer", "set_moisture_transfer_switch", 3001, "mdi:water-sync"),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(name): switch.switch_schema(
            SaveVTRSwitch,
            icon=icon,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="DISABLED",
        )
        for name, _, _, icon in SWITCHES
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    for name, setter, reg, _ in SWITCHES:
        if name in config:
            sw = await switch.new_switch(config[name], paren, reg)
            await cg.register_component(sw, config[name])
            cg.add(getattr(paren, setter)(sw))