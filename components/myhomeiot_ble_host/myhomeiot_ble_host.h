#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace myhomeiot_ble_host {

class MyHomeIOT_BLEHost;

class MyHomeIOT_BLEClientNode {
 public:
  virtual bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) = 0;
  virtual void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) = 0;
  virtual void loop() = 0;

  MyHomeIOT_BLEHost *host() { return this->ble_host_; }
  void set_ble_host(MyHomeIOT_BLEHost *ble_host) { this->ble_host_ = ble_host; }
  esp32_ble_tracker::ClientState state() const { return this->state_; }
 protected:
  MyHomeIOT_BLEHost *ble_host_;
  esp32_ble_tracker::ClientState state_;
};

class MyHomeIOT_BLEHost : public Component, public esp32_ble_tracker::ESPBTClient {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void connect() override {}

  void register_ble_client(MyHomeIOT_BLEClientNode *client) {
    client->set_ble_host(this);
    this->clients_.push_back(client);
  }

  int gattc_if;
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
  void on_scan_end() override {}

 protected:
  MyHomeIOT_BLEClientNode *current{nullptr};
  std::vector<MyHomeIOT_BLEClientNode *> clients_;
};

}  // namespace myhomeiot_ble_host
}  // namespace esphome

#endif
