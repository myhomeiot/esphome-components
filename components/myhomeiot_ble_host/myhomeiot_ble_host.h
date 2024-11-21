#pragma once

#ifdef USE_ESP32

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "esphome/core/version.h"
#if __has_include("esphome/core/macros.h")
#include "esphome/core/macros.h" // VERSION_CODE
#else
#define VERSION_CODE(major, minor, patch) ((major) << 16 | (minor) << 8 | (patch))
#endif
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2021, 10, 0)
#define MYHOMEIOT_IDLE esp32_ble_tracker::ClientState::IDLE
#define MYHOMEIOT_DISCOVERED esp32_ble_tracker::ClientState::DISCOVERED
#define MYHOMEIOT_CONNECTING esp32_ble_tracker::ClientState::CONNECTING
#define MYHOMEIOT_CONNECTED esp32_ble_tracker::ClientState::CONNECTED
#define MYHOMEIOT_ESTABLISHED esp32_ble_tracker::ClientState::ESTABLISHED
#else
#define MYHOMEIOT_IDLE esp32_ble_tracker::ClientState::Idle
#define MYHOMEIOT_DISCOVERED esp32_ble_tracker::ClientState::Discovered
#define MYHOMEIOT_CONNECTING esp32_ble_tracker::ClientState::Connecting
#define MYHOMEIOT_CONNECTED esp32_ble_tracker::ClientState::Connected
#define MYHOMEIOT_ESTABLISHED esp32_ble_tracker::ClientState::Established
#endif

namespace esphome {
namespace myhomeiot_ble_host {

class MyHomeIOT_BLEHost;

class MyHomeIOT_BLEClientNode {
 public:
  virtual bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) = 0;
  virtual bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) = 0;
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
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }
  void setup() override;
  void dump_config() override;
  void loop() override;
  void connect() override {}
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2024, 11, 0)
  void disconnect() override {}
#endif

  void register_ble_client(MyHomeIOT_BLEClientNode *client) {
    client->set_ble_host(this);
    this->clients_.push_back(client);
  }

  int gattc_if;
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2022, 11, 0)
  bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
#else
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
#endif
  bool gattc_event_handler_internal(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                    esp_ble_gattc_cb_param_t *param);
  virtual void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {}
  void on_scan_end() override {}

 protected:
  MyHomeIOT_BLEClientNode *current{nullptr};
  std::vector<MyHomeIOT_BLEClientNode *> clients_;
  void init();
};

}  // namespace myhomeiot_ble_host
}  // namespace esphome

#endif
