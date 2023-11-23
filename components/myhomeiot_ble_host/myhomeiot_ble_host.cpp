#ifdef USE_ESP32

#include <esp_gap_ble_api.h>
#include "esphome/core/log.h"
#include "myhomeiot_ble_host.h"

namespace esphome {
namespace myhomeiot_ble_host {

static const char *const TAG = "myhomeiot_ble_host";

void MyHomeIOT_BLEHost::init() {
  auto status = esp_ble_gattc_app_register(this->app_id);
  if (status) {
    ESP_LOGE(TAG, "app_register failed, app_id (%d) status (%d)", this->app_id, status);
    this->mark_failed();
  }
  this->set_state(MYHOMEIOT_IDLE);
}

void MyHomeIOT_BLEHost::setup() {
#if ESPHOME_VERSION_CODE < VERSION_CODE(2023, 11, 0)
  init();
#endif
}

void MyHomeIOT_BLEHost::dump_config() {
  ESP_LOGCONFIG(TAG, "MyHomeIOT BLE Host");
}

void MyHomeIOT_BLEHost::loop() {
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2023, 11, 0)
  if (!esp32_ble::global_ble->is_active()) {
    this->set_state(esp32_ble_tracker::ClientState::INIT);
    return;
  }
  if (this->state_ == esp32_ble_tracker::ClientState::INIT)
    init();
#endif

  if (current)
  {
    current->loop();
    if (this->state() != current->state())
      this->set_state(current->state());
  }
  if (this->state() == MYHOMEIOT_IDLE)
    current = nullptr;
}

bool MyHomeIOT_BLEHost::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (current)
    return false;
  for (auto *client : this->clients_)
    if (client->parse_device(device))
    {
      this->current = client;
      this->set_state(current->state());
      return true;
    }
  return false;
}

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2022, 11, 0)
bool MyHomeIOT_BLEHost::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
  esp_ble_gattc_cb_param_t *param) {
  return gattc_event_handler_internal(event, gattc_if, param);
}
#else
void MyHomeIOT_BLEHost::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
  esp_ble_gattc_cb_param_t *param) {
  gattc_event_handler_internal(event, gattc_if, param);
}
#endif

bool MyHomeIOT_BLEHost::gattc_event_handler_internal(esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if,
  esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_REG_EVT && this->app_id != param->reg.app_id)
    return false;
  if (event != ESP_GATTC_REG_EVT && esp_gattc_if != ESP_GATT_IF_NONE && esp_gattc_if != this->gattc_if)
    return false;

  bool result = true;
  switch (event) {
    case ESP_GATTC_REG_EVT: {
      if (param->reg.status == ESP_GATT_OK) {
        ESP_LOGV(TAG, "REG_EVT, app_id (%d)", this->app_id);
        this->gattc_if = esp_gattc_if;
      } else {
        ESP_LOGE(TAG, "REG_EVT failed app_id (%d) status (%d)", param->reg.app_id, param->reg.status);
      }
      break;
    }
    default:
      if (current)
      {
        result = current->gattc_event_handler(event, esp_gattc_if, param);
        this->set_state(current->state());
      }
      break;
  }
  return result;
}

}  // namespace myhomeiot_ble_host
}  // namespace esphome

#endif
