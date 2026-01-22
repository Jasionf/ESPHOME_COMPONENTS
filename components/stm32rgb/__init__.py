# __init__.py
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, light
from esphome.const import (
    CONF_ID,
    CONF_STRIP,
    CONF_ADDRESS,
)

# 声明这个组件依赖 i2c
DEPENDENCIES = ['i2c']

# 标记这是一个平台组件（light 平台）
IS_PLATFORM_COMPONENT = True

# 定义 C++ 命名空间
stm32rgb_ns = cg.esphome_ns.namespace('stm32rgb')

# 声明 C++ 类（必须和 .h 文件中的类名一致）
STM32RGBLight = stm32rgb_ns.class_(
    'STM32RGBLight',
    light.LightOutput,
    cg.Component,
    i2c.I2CDevice
)

# 配置验证 schema
CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend({
    # 必须的 ID
    cv.GenerateID(): cv.declare_id(STM32RGBLight),

    # 可选：strip 参数，默认 0
    cv.Optional(CONF_STRIP, default=0): cv.uint8_t,

    # 可选：I2C 地址，默认 0x1A（和你的硬件匹配）
    cv.Optional(CONF_ADDRESS, default=0x1A): cv.i2c_address,
}).extend(cv.COMPONENT_SCHEMA).extend(i2c.i2c_device_schema(default_address=0x1A))


# 代码生成函数（最核心部分）
async def to_code(config):
    # 创建 C++ 变量实例
    var = cg.new_Pvariable(config[CONF_ID])

    # 注册为组件（setup、loop 等生命周期）
    await cg.register_component(var, config)

    # 注册为 I2C 设备（会自动使用 yaml 中的 i2c 配置）
    await i2c.register_i2c_device(var, config)

    # 注册为 light 输出（会自动处理 light: 的各种属性）
    await light.register_light(var, config)

    # 把 yaml 中的 strip 值传给 C++ 类的 set_strip() 方法
    cg.add(var.set_strip(config[CONF_STRIP]))

    # 可选：如果你在 C++ 类中还有其他 setter，也可以在此添加
    # 例如：如果以后加了亮度限制或其他参数
    # cg.add(var.set_max_brightness(config.get(CONF_MAX_BRIGHTNESS, 255)))