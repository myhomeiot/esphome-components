import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome import automation
from esphome.const import (
    CONF_ID,
    CONF_DISCOVERY,
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

# Triggers
BLEGatewayBLEAdvertiseTrigger = ble_gateway_ns.class_(
    "BLEGatewayBLEAdvertiseTrigger", automation.Trigger.template(cg.std_string),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BLEGateway),
        cv.Optional(CONF_DISCOVERY, default=False): cv.boolean,
        cv.Optional(CONF_DEVICES): cv.All(
            cv.ensure_list(
                {
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

    cg.add(var.set_discovery(config[CONF_DISCOVERY]))
    if config.get(CONF_DEVICES):
      cg.add(var.set_devices("".join(f"{str(conf[CONF_MAC_ADDRESS]).replace(':', '')}" for conf in config[CONF_DEVICES])))

    for conf in config.get(CONF_ON_BLE_ADVERTISE):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(esp32_ble_tracker.ESPBTDeviceConstRef, "x"), (cg.std_string, "packet")], conf)
