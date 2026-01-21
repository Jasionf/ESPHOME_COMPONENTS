import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

DEPENDENCIES = ['i2c']

stm32rgb_ns = cg.esphome_ns.namespace('stm32rgb')
STM32RGBLight = stm32rgb_ns.class_('STM32RGBLight', light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(STM32RGBLight),
    cv.Optional('strip', default=0): cv.uint8_t,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_strip(config['strip']))
    await cg.register_component(var, config)
    await light.register_light(var, config)