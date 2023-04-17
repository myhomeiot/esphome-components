# `ESPHome` components

A collection of my ESPHome components.

Please ⭐️ this repo if you find it useful.

**If you have questions or problems with this components you can check [this](https://community.home-assistant.io/t/esphome-ble-gateway-and-other-ble-components/367935) thread.**

To use this repository you should confugure it inside your yaml-configuration:
```yaml
external_components:
  - source: github://myhomeiot/esphome-components
```

or download component into `custom_components` folder (you can use another name) and add following lines to your yaml-configuration:
```yaml
external_components:
  - source: custom_components
```

You can take a look at samples of usage of those components in [examples](examples) folder.

## [BLE Client](components/myhomeiot_ble_client)
BLE Client allow to read characteristics from devices.
Difference from build-in [ESPHome BLE Client](https://esphome.io/components/sensor/ble_client.html):
- Always disconnects from device after reading characteristic, this will allow to save device battery. You can specify `update_interval`, defaults to 60min.
- Uses lambda for parsing and extracting data into specific sensors make this component very flexible and useful for prototyping.
- There is no limit to the number of BLE Clients used (build-in BLE Client has limit of 3 instances). This component uses BLE Host component which you should count as one instance of build-in BLE Client. All BLE clients are processed sequentially inside the host component at time when they was detected and update interval reached.

## [BLE Host](components/myhomeiot_ble_host)
Used by BLE Client component.

## [BLE Gateway](components/ble_gateway)
BLE Gateway component will allow you to forward BLE Advertising data packets for external processing to [Home Assistant](https://www.home-assistant.io) or other systems.

If the heart of your Home Automation system is Home Assistant or another similar system and you use [ESPHome](https://esphome.io) devices to extend BLE coverage and process data from BLE sensors, you can dramatically decrease system complexity by remove all BLE data processing from ESPHome devices and forward raw BLE Advertising data to external components like [Passive BLE Monitor](https://github.com/custom-components/ble_monitor).

**Requirements:**
- [Passive BLE Monitor](https://github.com/custom-components/ble_monitor) integration version **6.2** or later. Thanks [@Ernst79](https://github.com/Ernst79)
- [ESPHome](https://esphome.io) version **2022.1** or later

If you use ESPHome **2021.12** version or earlyer you need to make [following changes](https://github.com/esphome/esphome/pull/2854) in ESPHome `esp32_ble_tracker` component.
In order to apply this PR you can use following ESPHome configuration (requires **ESPHome 2021.11** or later):
```yaml
external_components:
  - source: github://myhomeiot/esphome-components
  - source: github://pr#2854
    components: [esp32_ble_tracker]
```

#### ESPHome configuration example
Note: This example use [event](https://esphome.io/components/api.html#homeassistant-event-action), you can use direct `ble_monitor.parse_data` [service call](https://esphome.io/components/api.html#homeassistant-service-action)
```yaml
ble_gateway:
  devices:
    - mac_address: 01:23:45:67:89:AB
    - mac_address: !secret lywsd03mmc_mac
  on_ble_advertise:
    then:
      homeassistant.event:
        event: esphome.on_ble_advertise
        data:
          packet: !lambda return packet;
```

#### Home Assistant Passive BLE Monitor configuration example
Note: Remove automation if you use direct `ble_monitor.parse_data` service call
```yaml
ble_monitor:
  discovery: false
  restore_state: true
  decimals: 1
  period: 300
  devices:
    - name: Living Room Thermo
      mac: 01:23:45:67:89:AB
    - name: Bedroom Thermo
      mac: !secret lywsd03mmc_mac

automation:
  - alias: ESPHome BLE Advertise
    mode: queued
    trigger:
      - platform: event
        event_type: esphome.on_ble_advertise
    action:
      - service: ble_monitor.parse_data
        data:
          packet: "{{ trigger.event.data.packet }}"
```

#### Advanced configuration where ESPHome devices gets MAC addresses from Passive BLE Monitor configuration
Note: Be sure that you turn **on** the `input_boolean.settings_ble_gateway` if you want to receive BLE packets from BLE Gateway's.

**Important note:** New device address will be populated to ESPHome devices only after Passive BLE Monitor receives first BLE packet and creates entities for it. If in your configuration you don't have BLE stick and you have only ESPHome devices you can use one of the following methods:
1. Add this device MAC address manually into the `input_text.settings_ble_gateway_add_device`. After Passive BLE Monitor creates entities for new device, you can remove address.
2. Enable [discovery](https://custom-components.github.io/ble_monitor/configuration_params#discovery) for Passive BLE Monitor and ESPHome BLE Gateway `input_boolean.settings_ble_gateway_discovery`. After required devices will be discovered turn off discovery options (for Passive BLE Monitor and ESPHome BLE Gateway) and clean up unneeded devices that got detected if any.

```yaml
# ESPHome
ble_gateway:
  id: blegateway
  on_ble_advertise:
    then:
      homeassistant.event:
        event: esphome.on_ble_advertise
        data:
          packet: !lambda return packet;

binary_sensor:
  - platform: homeassistant
    id: ble_gateway_discovery
    entity_id: binary_sensor.ble_gateway
    attribute: discovery
    on_state:
      then:
        lambda: id(blegateway).set_discovery(x);

text_sensor:
  - platform: homeassistant
    id: ble_gateway_devices
    entity_id: binary_sensor.ble_gateway
    attribute: devices
    on_value:
      then:
        lambda: id(blegateway).set_devices(x);

switch:
  - platform: template
    id: switch_ble_gateway_discovery
    name: BLE Gateway Discovery
    icon: mdi:bluetooth-connect
    lambda: return id(blegateway).get_discovery();
    turn_on_action: [lambda: id(blegateway).set_discovery(true);]
    turn_off_action: [lambda: id(blegateway).set_discovery(false);]
    disabled_by_default: true
    entity_category: config

# Home Assistant
input_boolean:
  settings_ble_gateway:
    name: BLE Gateway
    icon: mdi:bluetooth
  settings_ble_gateway_discovery:
    name: BLE Gateway Discovery
    icon: mdi:bluetooth-connect

input_text:
  settings_ble_gateway_add_device:
    name: BLE Gateway Add Device
    icon: mdi:bluetooth-connect
    initial: ''

template:
  - binary_sensor:
      - name: BLE Gateway
        icon: mdi:bluetooth
        state: "{{ is_state('input_boolean.settings_ble_gateway', 'on') }}"
        attributes:
          discovery: "{{ is_state('input_boolean.settings_ble_gateway_discovery', 'on') }}"
          # devices: "{{ states | selectattr('entity_id', 'search', '^(device_tracker|sensor).ble_') | selectattr('attributes.mac address', 'defined') | map(attribute='attributes.mac address') | unique | sort | join('') | replace(':', '') ~ (states('input_text.settings_ble_gateway_add_device') | replace(':', '') | trim) if is_state('binary_sensor.ble_gateway', 'on') }}"
          # Important note: In Passive BLE Monitor version 7.8.2 and later 'attributes.mac address' was changed to 'attributes.mac_address', please update your config
          # devices: "{{ states | selectattr('entity_id', 'search', '^(device_tracker|sensor).ble_') | selectattr('attributes.mac_address', 'defined') | map(attribute='attributes.mac_address') | unique | sort | join('') | replace(':', '') ~ (states('input_text.settings_ble_gateway_add_device') | replace(':', '') | trim) if is_state('binary_sensor.ble_gateway', 'on') }}"
          # Note: In Home Assistant 2022.x, Passive BLE Monitor version 8.x and later you can use device attribute identifiers
          devices: >-
            {% set devices = namespace(items = []) %}
            {% for s in states | selectattr('entity_id', 'search', '^(device_tracker|sensor).ble_') | map(attribute='entity_id') %}
              {% set devices.items = devices.items + ([device_id(s)] if device_id(s) else []) %}
            {% endfor %}
            {% set ns = namespace(items = []) %}
            {% for s in devices.items | unique %}
              {% set ns.items = ns.items + [(device_attr(s, 'identifiers') | first)[1]] %}
            {% endfor %}
            {{ ns.items | unique | sort | join('') | replace(':', '') ~ (states('input_text.settings_ble_gateway_add_device') | replace(':', '') | trim) if is_state('binary_sensor.ble_gateway', 'on') }}
```

**More configuration examples you can find in [examples](examples) folder.**
