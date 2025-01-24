#pragma once

#ifdef USE_ESP32

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/automation.h"
#include "esphome/components/myhomeiot_ble_host/myhomeiot_ble_host.h"

namespace esphome {
namespace myhomeiot_ble_client {

class MyHomeIOT_BLEClient : public PollingComponent, public myhomeiot_ble_host::MyHomeIOT_BLEClientNode {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

  void add_on_state_callback(std::function<void(std::vector<uint8_t>, const MyHomeIOT_BLEClient &)> &&callback) { this->callback_.add(std::move(callback)); }
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device);
  bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

  void set_address(uint64_t address) { this->address_ = address; }

  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_service_uuid16(uint16_t uuid) { this->service_uuid_ = esp32_ble_tracker::ESPBTUUID::from_uint16(uuid); }
  void set_service_uuid32(uint32_t uuid) { this->service_uuid_ = esp32_ble_tracker::ESPBTUUID::from_uint32(uuid); }
  void set_service_uuid128(uint8_t *uuid) { this->service_uuid_ = esp32_ble_tracker::ESPBTUUID::from_raw(uuid); }
  void set_char_uuid16(uint16_t uuid) { this->char_uuid_ = esp32_ble_tracker::ESPBTUUID::from_uint16(uuid); }
  void set_char_uuid32(uint32_t uuid) { this->char_uuid_ = esp32_ble_tracker::ESPBTUUID::from_uint32(uuid); }
  void set_char_uuid128(uint8_t *uuid) { this->char_uuid_ = esp32_ble_tracker::ESPBTUUID::from_raw(uuid); }

  const uint8_t *remote_bda() const { return remote_bda_; }
 protected:
  std::string to_string(uint64_t address) const;

  CallbackManager<void(std::vector<uint8_t>, const MyHomeIOT_BLEClient &)> callback_{};
  bool is_update_requested_;
  uint64_t address_;
  esp32_ble_tracker::ESPBTUUID service_uuid_;
  esp32_ble_tracker::ESPBTUUID char_uuid_;
  esp_bd_addr_t remote_bda_;
  uint16_t conn_id_;
  uint16_t start_handle_;
  uint16_t end_handle_;
  uint16_t char_handle_;

  void connect();
  void disconnect();
  void report_results(uint8_t *data, uint16_t len);
  void report_error(esp32_ble_tracker::ClientState state = MYHOMEIOT_ESTABLISHED);
};

class MyHomeIOT_BLEClientValueTrigger : public Trigger<std::vector<uint8_t>, const MyHomeIOT_BLEClient &> {
 public:
  explicit MyHomeIOT_BLEClientValueTrigger(MyHomeIOT_BLEClient *parent) {
    parent->add_on_state_callback([this](std::vector<uint8_t> value, const MyHomeIOT_BLEClient &xthis) { this->trigger(value, xthis); });
  }
};

}  // namespace myhomeiot_ble_client
}  // namespace esphome

#endif
