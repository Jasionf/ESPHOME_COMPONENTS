import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@Jasionf"]

CONF_PYRAMIDRGB_ID = "pyramidrgb_id"
CONF_STRIP = "strip"
CONF_BRIGHTNESS = "brightness"

pyramidrgb_ns = cg.esphome_ns.namespace("pyramidrgb")
PyramidRGBComponent = pyramidrgb_ns.class_("PyramidRGBComponent", cg.Component, i2c.I2CDevice)

BASE_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_PYRAMIDRGB_ID): cv.use_id(PyramidRGBComponent),
})

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PyramidRGBComponent),
            cv.Optional(CONF_STRIP, default=1): cv.int_range(min=1, max=2),
            cv.Optional(CONF_BRIGHTNESS, default=100): cv.int_range(min=0, max=100),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x1A))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # 初始亮度设置
    cg.add(var.set_strip_brightness(config[CONF_STRIP], config[CONF_BRIGHTNESS]))