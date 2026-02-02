import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    ICON_ROTATE_RIGHT,
    STATE_CLASS_MEASUREMENT,
)
from . import Step16Component, CONF_STEP16_ID

DEPENDENCIES = ["step16"]

step16_ns = cg.esphome_ns.namespace("step16")
Step16Sensor = step16_ns.class_("Step16Sensor", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        Step16Sensor,
        icon=ICON_ROTATE_RIGHT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_STEP16_ID): cv.use_id(Step16Component),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    
    parent = await cg.get_variable(config[CONF_STEP16_ID])
    cg.add(var.set_parent(parent))
