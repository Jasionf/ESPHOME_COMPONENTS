import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_RESET_PIN

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@Jasionf"]

aw87559_ns = cg.esphome_ns.namespace("aw87559")
AW87559Component = aw87559_ns.class_("AW87559Component", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AW87559Component),
        cv.Optional(CONF_RESET_PIN): cv.All(
            cv.only_on_esp32, cv.only_on_esp8266, cv.invalid("Reset pin is not supported on this platform")
        ),
    }
).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x5B))
