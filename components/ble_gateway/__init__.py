import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome import automation
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_TRIGGER_ID,
    CONF_ON_BLE_ADVERTISE,
)

CODEOWNERS = ["@myhomeiot"]
DEPENDENCIES = ["esp32_ble_tracker"]

CONF_DEVICES = "devices"

ble_gateway_ns = cg.esphome_ns.namespace("ble_gateway")
BLEGateway = ble_gateway_ns.class_(
    "BLEGateway", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)
BLEGatewayDevice = ble_gateway_ns.class_("BLEGatewayDevice")

# Triggers
BLEGatewayBLEAdvertiseTrigger = ble_gateway_ns.class_(
    "BLEGatewayBLEAdvertiseTrigger", automation.Trigger.template(cg.std_string),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BLEGateway),
        cv.Required(CONF_DEVICES): cv.All(
            cv.ensure_list(
                {
                    cv.GenerateID(): cv.declare_id(BLEGatewayDevice),
                    cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
                }
            ),
            cv.Length(min=1),
        ),
        cv.Required(CONF_ON_BLE_ADVERTISE): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEGatewayBLEAdvertiseTrigger),
            }
        ),
    }
).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    for conf in config[CONF_DEVICES]:
        cg.add(var.register_device(cg.new_Pvariable(conf[CONF_ID], conf[CONF_MAC_ADDRESS].as_hex)))

    for conf in config.get(CONF_ON_BLE_ADVERTISE):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "packet")], conf)
