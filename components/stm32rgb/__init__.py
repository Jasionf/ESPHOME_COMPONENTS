import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, light
from esphome.const import CONF_ID, CONF_STRIP

DEPENDENCIES = ['i2c']

IS_PLATFORM_COMPONENT = True

stm32rgb_ns = cg.esphome_ns.namespace('stm32rgb')
STM32RGBLight = stm32rgb_ns.class_(
    'STM32RGBLight',
    light.LightOutput,
    cg.Component,
    i2c.I2CDevice
)

CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(STM32RGBLight),
    cv.Optional(CONF_STRIP, default=0): cv.uint8_t,
    cv.Optional('address', default=0x1A): cv.i2c_address,
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(0x1A))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await light.register_light(var, config)

    # 傳入 strip 參數
    cg.add(var.set_strip(config[CONF_STRIP]))