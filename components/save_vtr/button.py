import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_NONE

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent, save_vtr_ns

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

SaveVTRButton = save_vtr_ns.class_("SaveVTRButton", button.Button, cg.Component)

# Button action identifiers (must match SaveVTRButtonAction in save_vtr.h)
ACTION_ACTIVATE_FIREPLACE = 1
ACTION_ACTIVATE_REFRESH = 2
ACTION_ACTIVATE_CROWDED = 3

BUTTONS = [
    ("activate_fireplace", ACTION_ACTIVATE_FIREPLACE, "mdi:fireplace"),
    ("activate_refresh", ACTION_ACTIVATE_REFRESH, "mdi:fan-plus"),
    ("activate_crowded", ACTION_ACTIVATE_CROWDED, "mdi:account-group"),
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(name): button.button_schema(
            SaveVTRButton,
            icon=icon,
            entity_category=ENTITY_CATEGORY_NONE,
        )
        for name, _, icon in BUTTONS
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    for name, action_id, _ in BUTTONS:
        if name in config:
            btn = await button.new_button(config[name], paren, action_id)
            await cg.register_component(btn, config[name])
            cg.add(paren.register_button(btn))