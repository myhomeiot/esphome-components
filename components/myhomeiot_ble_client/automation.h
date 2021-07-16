#pragma once

#include "esphome/core/automation.h"
#include "myhomeiot_ble_client.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace myhomeiot_ble_client {

class MyHomeIOT_BLEClientValueTrigger : public Trigger<std::vector<uint8_t>> {
 public:
  explicit MyHomeIOT_BLEClientValueTrigger(MyHomeIOT_BLEClient *parent) {
    parent->add_on_state_callback([this](std::vector<uint8_t> value) { this->trigger(value); });
  }
};

}  // namespace myhomeiot_ble_client
}  // namespace esphome

#endif
