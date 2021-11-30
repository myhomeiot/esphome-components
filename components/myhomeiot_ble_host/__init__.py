import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome.core import coroutine
from esphome.const import (
    CONF_ID,
)

CODEOWNERS = ["@myhomeiot"]
DEPENDENCIES = ["esp32_ble_tracker"]

myhomeiot_ble_host_ns = cg.esphome_ns.namespace("myhomeiot_ble_host")
MyHomeIOT_BLEHost = myhomeiot_ble_host_ns.class_(
    "MyHomeIOT_BLEHost", cg.Component, esp32_ble_tracker.ESPBTClient
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MyHomeIOT_BLEHost),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)

CONF_BLE_HOST_ID = "ble_host_id"

BLE_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BLE_HOST_ID): cv.use_id(MyHomeIOT_BLEHost),
    }
)

async def register_ble_client(var, config):
    parent = await cg.get_variable(config[CONF_BLE_HOST_ID])
    cg.add(parent.register_ble_client(var))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_client(var, config)
