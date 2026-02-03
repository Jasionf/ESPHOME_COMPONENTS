import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch, espnow
from esphome.const import CONF_ID
from . import (
    espnow_switch_ns,
    CONF_ESPNOW_ID,
    CONF_MAC_ADDRESS,
    CONF_RESPONSE_TOKEN,
    CONF_RETRY_COUNT,
    CONF_RETRY_INTERVAL,
)

DEPENDENCIES = ["espnow"]
CODEOWNERS = ["@jason"]

ESPNowSwitch = espnow_switch_ns.class_("ESPNowSwitch", switch.Switch, cg.Component)


CONFIG_SCHEMA = (
    switch.switch_schema(ESPNowSwitch)
    .extend(
        {
            cv.GenerateID(CONF_ESPNOW_ID): cv.use_id(espnow.ESPNowComponent),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_RESPONSE_TOKEN): cv.string,
            cv.Optional(CONF_RETRY_COUNT, default=20): cv.int_range(min=1, max=100),
            cv.Optional(CONF_RETRY_INTERVAL, default=250): cv.int_range(min=10, max=5000),
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
    
    # 解析并设置 MAC 地址（cv.mac_address 返回 core.MACAddress）
    mac = config[CONF_MAC_ADDRESS]
    cg.add(var.set_mac_address(mac.parts[0], mac.parts[1], mac.parts[2], mac.parts[3], mac.parts[4], mac.parts[5]))
    
    # 设置响应匹配令牌
    cg.add(var.set_response_token(config[CONF_RESPONSE_TOKEN]))
    
    # 设置重试参数
    cg.add(var.set_retry_count(config[CONF_RETRY_COUNT]))
    cg.add(var.set_retry_interval(config[CONF_RETRY_INTERVAL]))



