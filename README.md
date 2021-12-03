# `ESPHome` components

A collection of my ESPHome components.
Please ⭐️ this repo if you find it useful.

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

**Important note:** Currently in order to run BLE Gateway you need to make some changes in ESPHome `esp32_ble_tracker` component and Passive BLE Monitor integration, but I will send PR to these components and hopefully they will be accepted.

#### ESPHome configuration example (with [event](https://esphome.io/components/api.html#homeassistant-event-action), you can use direct ble_monitor.parse_data [service call](https://esphome.io/components/api.html#homeassistant-service-action))
```yaml
ble_gateway:
  devices:
    - mac_address: 01:23:45:67:89:AB
    - mac_address: !secret lywsd03mmc_mac
  on_ble_advertise:
    then:
      - homeassistant.event:
          event: esphome.on_ble_advertise
          data:
            packet: !lambda return packet;
```

#### Home Assistant Passive BLE Monitor configuration example (remove automation if you use direct ble_monitor.parse_data service call)
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
