import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_RESET_PIN

CODEOWNERS = ["@your_github_username"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = []

aw87559_ns = cg.esphome_ns.namespace("aw87559")
AW87559Component = aw87559_ns.class_("AW87559Component", cg.Component, i2c.I2CDevice)

CONF_AW87559_ID = "aw87559_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AW87559Component),
            cv.Optional(CONF_RESET_PIN): cv.gpio_pin_schema,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x58))  # Default I2C address 0x58
)


async def to_code(config):
    var = cg.new_Pvariable(config[cv.GenerateID()])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    # Configure reset GPIO if provided
    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_digital_output_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_gpio(reset_pin))
