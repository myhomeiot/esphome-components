import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import myhomeiot_ble_host, esp32_ble_tracker
from esphome import const, automation
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_SERVICE_UUID,
    CONF_TRIGGER_ID,
    CONF_ON_VALUE,
)

CODEOWNERS = ["@myhomeiot"]
DEPENDENCIES = ["myhomeiot_ble_host"]
MULTI_CONF = True

CONF_BLE_HOST = "ble_host"
CONF_CHARACTERISTIC_UUID = "characteristic_uuid"

myhomeiot_ble_client_ns = cg.esphome_ns.namespace("myhomeiot_ble_client")
MyHomeIOT_BLEClient = myhomeiot_ble_client_ns.class_(
    "MyHomeIOT_BLEClient", cg.Component
)
MyHomeIOT_BLEClientConstRef = MyHomeIOT_BLEClient.operator("ref").operator("const")

# Triggers
MyHomeIOT_BLEClientValueTrigger = myhomeiot_ble_client_ns.class_(
    "MyHomeIOT_BLEClientValueTrigger",
    automation.Trigger.template(cg.std_vector.template(cg.uint8)),
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MyHomeIOT_BLEClient),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_SERVICE_UUID): esp32_ble_tracker.bt_uuid,
            cv.Required(CONF_CHARACTERISTIC_UUID): esp32_ble_tracker.bt_uuid,
            cv.Optional(CONF_ON_VALUE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MyHomeIOT_BLEClientValueTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(cv.polling_component_schema("60min"))
    .extend(myhomeiot_ble_host.BLE_CLIENT_SCHEMA)
)

def versiontuple(v):
    return tuple(map(int, (v.split("."))))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    reversed = versiontuple(const.__version__.split("-")[0]) >= versiontuple("2021.9.0")
    if len(config[CONF_SERVICE_UUID]) == len(esp32_ble_tracker.bt_uuid16_format):
        cg.add(var.set_service_uuid16(esp32_ble_tracker.as_hex(config[CONF_SERVICE_UUID])))
    elif len(config[CONF_SERVICE_UUID]) == len(esp32_ble_tracker.bt_uuid32_format):
        cg.add(var.set_service_uuid32(esp32_ble_tracker.as_hex(config[CONF_SERVICE_UUID])))
    elif len(config[CONF_SERVICE_UUID]) == len(esp32_ble_tracker.bt_uuid128_format):
        uuid128 = esp32_ble_tracker.as_reversed_hex_array(config[CONF_SERVICE_UUID]) if reversed else esp32_ble_tracker.as_hex_array(config[CONF_SERVICE_UUID])
        cg.add(var.set_service_uuid128(uuid128))

    if len(config[CONF_CHARACTERISTIC_UUID]) == len(esp32_ble_tracker.bt_uuid16_format):
        cg.add(var.set_char_uuid16(esp32_ble_tracker.as_hex(config[CONF_CHARACTERISTIC_UUID])))
    elif len(config[CONF_CHARACTERISTIC_UUID]) == len(esp32_ble_tracker.bt_uuid32_format):
        cg.add(var.set_char_uuid32(esp32_ble_tracker.as_hex(config[CONF_CHARACTERISTIC_UUID])))
    elif len(config[CONF_CHARACTERISTIC_UUID]) == len(esp32_ble_tracker.bt_uuid128_format):
        uuid128 = esp32_ble_tracker.as_reversed_hex_array(config[CONF_CHARACTERISTIC_UUID]) if reversed else esp32_ble_tracker.as_hex_array(config[CONF_CHARACTERISTIC_UUID])
        cg.add(var.set_char_uuid128(uuid128))

    await cg.register_component(var, config)
    await myhomeiot_ble_host.register_ble_client(var, config)
    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))

    for conf in config.get(CONF_ON_VALUE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_vector.template(cg.uint8), "x"), (MyHomeIOT_BLEClientConstRef, "xthis")], conf)
