import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID
from . import (
    espnow_switch_ns,
    CONF_ESPNOW_ID,
    CONF_MAC_ADDRESS,
    CONF_DEVICE_ID,
    CONF_RETRY_COUNT,
    CONF_RETRY_INTERVAL,
)

DEPENDENCIES = ["espnow"]
CODEOWNERS = ["@jason"]

ESPNowSwitch = espnow_switch_ns.class_("ESPNowSwitch", switch.Switch, cg.Component)


def validate_mac_address(value):
    """验证 MAC 地址格式"""
    value = cv.string_strict(value)
    parts = value.replace("-", ":").split(":")
    if len(parts) != 6:
        raise cv.Invalid("MAC address must have 6 pairs of hex digits")
    
    for part in parts:
        if len(part) != 2:
            raise cv.Invalid("Each part of MAC address must be 2 hex digits")
        try:
            int(part, 16)
        except ValueError:
            raise cv.Invalid(f"Invalid hex value: {part}")
    
    return value.upper().replace("-", ":")


CONFIG_SCHEMA = (
    switch.switch_schema(ESPNowSwitch)
    .extend(
        {
            cv.GenerateID(CONF_ESPNOW_ID): cv.use_id(cg.Component),
            cv.Required(CONF_MAC_ADDRESS): validate_mac_address,
            cv.Required(CONF_DEVICE_ID): cv.string,
            cv.Optional(CONF_RETRY_COUNT, default=40): cv.int_range(min=1, max=100),
            cv.Optional(CONF_RETRY_INTERVAL, default=100): cv.int_range(min=10, max=5000),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)
    
    # 获取 ESPNow 组件引用
    espnow_component = await cg.get_variable(config[CONF_ESPNOW_ID])
    cg.add(var.set_espnow_component(espnow_component))
    
    # 解析并设置 MAC 地址
    mac_str = config[CONF_MAC_ADDRESS]
    mac_parts = mac_str.split(":")
    mac_array = [int(part, 16) for part in mac_parts]
    cg.add(var.set_mac_address(mac_array[0], mac_array[1], mac_array[2], 
                                mac_array[3], mac_array[4], mac_array[5]))
    
    # 设置设备 ID
    cg.add(var.set_device_id(config[CONF_DEVICE_ID]))
    
    # 设置重试参数
    cg.add(var.set_retry_count(config[CONF_RETRY_COUNT]))
    cg.add(var.set_retry_interval(config[CONF_RETRY_INTERVAL]))
