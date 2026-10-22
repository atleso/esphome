import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_NONE

from . import SAVE_VTR_CLIENT_SCHEMA, get_save_vtr_parent, save_vtr_ns

CODEOWNERS = ["@atleso"]
DEPENDENCIES = ["climate"]

SaveVTRUserModeSelect = save_vtr_ns.class_(
    "SaveVTRUserModeSelect", select.Select, cg.Component
)

CONF_USER_MODE = "user_mode"

# Same order/value mapping as the climate custom fan modes (register 1161)
USER_MODE_OPTIONS = [
    "AUTO-VENT",
    "MANUAL",
    "CROWDED",
    "REFRESH",
    "FIREPLACE",
    "AWAY",
    "HOLIDAY",
]

CONFIG_SCHEMA = SAVE_VTR_CLIENT_SCHEMA.extend(
    {
        cv.Optional(CONF_USER_MODE): select.select_schema(
            SaveVTRUserModeSelect,
            icon="mdi:fan-auto",
            entity_category=ENTITY_CATEGORY_NONE,
        ),
    }
)


async def to_code(config):
    paren = await get_save_vtr_parent(config)
    if CONF_USER_MODE in config:
        sel = await select.new_select(
            config[CONF_USER_MODE], paren, options=USER_MODE_OPTIONS
        )
        await cg.register_component(sel, config[CONF_USER_MODE])
        cg.add(paren.set_user_mode_select(sel))