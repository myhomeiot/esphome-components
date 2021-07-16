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

## [BLE Client](custom_components/myhomeiot_ble_client)
BLE Client allow to read characteristics from devices.
Difference from build-in [ESPHome BLE Client](https://esphome.io/components/sensor/ble_client.html):
- Always disconnects from device after reading characteristic, this will allow to save device battery. You can specify `update_interval`, defaults to 60min.
- Uses lambda for parsing and extracting data into specific sensors make this component very flexible and useful for prototyping.
- There is no limit to the number of BLE Clients used (build-in BLE Client has limit of 3 instances). This component uses BLE Host component which you should count as one instance of build-in BLE Client. All BLE clients are processed sequentially inside the host component at time when they was detected and update interval reached.

## [BLE Host](custom_components/myhomeiot_ble_host)
Used by BLE Client component.
