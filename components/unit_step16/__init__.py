import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@m5stack"]

unit_step16_ns = cg.esphome_ns.namespace("unit_step16")
UnitStep16Component = unit_step16_ns.class_("UnitStep16Component", cg.Component, i2c.I2CDevice)
OutputChannel = unit_step16_ns.enum("OutputChannel", is_class=True)

CONF_UNIT_STEP16_ID = "unit_step16_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UnitStep16Component),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x48))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
