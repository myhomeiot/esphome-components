#ifdef USE_ESP32

#include <esp_gap_ble_api.h>
#include "esphome/core/log.h"
#include "myhomeiot_ble_client.h"

namespace esphome {
namespace myhomeiot_ble_client {

static const char *const TAG = "myhomeiot_ble_client";

void MyHomeIOT_BLEClient::setup() {
  this->state_ = MYHOMEIOT_IDLE;
}

void MyHomeIOT_BLEClient::dump_config() {
  ESP_LOGCONFIG(TAG, "MyHomeIOT BLE Client");
  ESP_LOGCONFIG(TAG, "  MAC address: %s", to_string(this->address_).c_str());
  ESP_LOGCONFIG(TAG, "  Service UUID: %s", this->service_uuid_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Characteristic UUID: %s", this->char_uuid_.to_string().c_str());
  LOG_UPDATE_INTERVAL(this);
}

void MyHomeIOT_BLEClient::loop() {
  if (this->state_ == MYHOMEIOT_DISCOVERED)
    this->connect();
  else if (this->state_ == MYHOMEIOT_ESTABLISHED)
    this->disconnect();
}

void MyHomeIOT_BLEClient::update() {
  this->is_update_requested_ = true;
}

void MyHomeIOT_BLEClient::connect() {
  ESP_LOGI(TAG, "[%s] Connecting", to_string(this->address_).c_str());
  this->state_ = MYHOMEIOT_CONNECTING;
  if (auto status = esp_ble_gattc_open(ble_host_->gattc_if, this->remote_bda_, BLE_ADDR_TYPE_PUBLIC, true))
  {
    ESP_LOGW(TAG, "[%s] open error, status (%d)", to_string(this->address_).c_str(), status);
    report_error(MYHOMEIOT_IDLE);
  }
}

void MyHomeIOT_BLEClient::disconnect() {
  ESP_LOGI(TAG, "[%s] Disconnecting", to_string(this->address_).c_str());
  this->state_ = MYHOMEIOT_IDLE;
  if (auto status = esp_ble_gattc_close(ble_host_->gattc_if, this->conn_id_))
    ESP_LOGW(TAG, "[%s] close error, status (%d)", to_string(this->address_).c_str(), status);
}

void MyHomeIOT_BLEClient::report_results(uint8_t *data, uint16_t len) {
  this->status_clear_warning();
  std::vector<uint8_t> value(data, data + len);
  this->callback_.call(value, *this);
  this->is_update_requested_ = false;
}

void MyHomeIOT_BLEClient::report_error(esp32_ble_tracker::ClientState state) {
  this->state_ = state;
  this->status_set_warning();
}

bool MyHomeIOT_BLEClient::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (!this->is_update_requested_ || this->state_ != MYHOMEIOT_IDLE
    || device.address_uint64() != this->address_)
    return false;

  ESP_LOGD(TAG, "[%s] Found device", device.address_str().c_str());
  memcpy(this->remote_bda_, device.address(), sizeof(this->remote_bda_));
  this->state_ = MYHOMEIOT_DISCOVERED;
  return true;
}

bool MyHomeIOT_BLEClient::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if,
  esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (memcmp(param->open.remote_bda, this->remote_bda_, sizeof(this->remote_bda_)) != 0)
        break;
      if (param->open.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "[%s] OPEN_EVT failed, status (%d), app_id (%d)", to_string(this->address_).c_str(),
          param->open.status, ble_host_->app_id);
        report_error(MYHOMEIOT_IDLE);
        break;
      }
      ESP_LOGI(TAG, "[%s] Connected successfully, app_id (%d)", to_string(this->address_).c_str(), ble_host_->app_id);
      this->conn_id_ = param->open.conn_id;
      if (auto status = esp_ble_gattc_send_mtu_req(ble_host_->gattc_if, param->open.conn_id))
      {
        ESP_LOGW(TAG, "[%s] send_mtu_req failed, status (%d)", to_string(this->address_).c_str(), status);
        report_error();
        break;
      }
      this->start_handle_ = this->end_handle_ = this->char_handle_ = ESP_GATT_ILLEGAL_HANDLE;
      this->state_ = MYHOMEIOT_CONNECTED;
      break;
    }
    case ESP_GATTC_CFG_MTU_EVT: {
      if (param->cfg_mtu.conn_id != this->conn_id_)
        break;
      if (param->cfg_mtu.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "[%s] CFG_MTU_EVT failed, status (%d)", to_string(this->address_).c_str(),
          param->cfg_mtu.status);
        report_error();
        break;
      }
      ESP_LOGV(TAG, "[%s] CFG_MTU_EVT, MTU (%d)", to_string(this->address_).c_str(), param->cfg_mtu.mtu);
      if (auto status = esp_ble_gattc_search_service(esp_gattc_if, param->cfg_mtu.conn_id, nullptr)) {
        ESP_LOGW(TAG, "[%s] search_service failed, status (%d)", to_string(this->address_).c_str(), status);
        report_error();
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      if (memcmp(param->disconnect.remote_bda, this->remote_bda_, sizeof(this->remote_bda_)) != 0)
        return false;
      ESP_LOGD(TAG, "[%s] DISCONNECT_EVT", to_string(this->address_).c_str());
      this->state_ = MYHOMEIOT_IDLE;
      break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
      if (param->search_res.conn_id != this->conn_id_)
        break;
      esp32_ble_tracker::ESPBTUUID uuid = param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 ? esp32_ble_tracker::ESPBTUUID::from_uint16(param->search_res.srvc_id.uuid.uuid.uuid16)
        : param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_32 ? esp32_ble_tracker::ESPBTUUID::from_uint32(param->search_res.srvc_id.uuid.uuid.uuid32)
        : esp32_ble_tracker::ESPBTUUID::from_raw(param->search_res.srvc_id.uuid.uuid.uuid128);
      if (uuid == this->service_uuid_) {
        ESP_LOGD(TAG, "[%s] SEARCH_RES_EVT service (%s) found", to_string(this->address_).c_str(),
          this->service_uuid_.to_string().c_str());
        this->start_handle_ = param->search_res.start_handle;
        this->end_handle_ = param->search_res.end_handle;
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      if (param->search_cmpl.conn_id != this->conn_id_)
        break;
      ESP_LOGV(TAG, "[%s] SEARCH_CMPL_EVT", to_string(this->address_).c_str());

      if (this->start_handle_ == ESP_GATT_ILLEGAL_HANDLE)
      {
        ESP_LOGE(TAG, "[%s] SEARCH_CMPL_EVT service (%s) not found", to_string(this->address_).c_str(),
          this->service_uuid_.to_string().c_str());
        report_error();
        break;
      }

      uint16_t offset = 0;
      esp_gattc_char_elem_t result;
      while (true) {
        uint16_t count = 1;
        auto status = esp_ble_gattc_get_all_char(ble_host_->gattc_if, this->conn_id_, 
          this->start_handle_, this->end_handle_, &result, &count, offset);
        if (status != ESP_GATT_OK) {
          if (status == ESP_GATT_INVALID_OFFSET || status == ESP_GATT_NOT_FOUND)
            break;
          ESP_LOGW(TAG, "[%s] get_all_char error, status (%d)", to_string(this->address_).c_str(), status);
          report_error();
          break;
        }
        if (count == 0)
          break;

        if (this->char_uuid_ == esp32_ble_tracker::ESPBTUUID::from_uuid(result.uuid)) {
          ESP_LOGD(TAG, "[%s] SEARCH_CMPL_EVT char (%s) found", to_string(this->address_).c_str(),
            this->char_uuid_.to_string().c_str());
          this->char_handle_ = result.char_handle;

          if (auto status = esp_ble_gattc_read_char(ble_host_->gattc_if, this->conn_id_, 
            this->char_handle_, ESP_GATT_AUTH_REQ_NONE) != ESP_GATT_OK) {
            ESP_LOGW(TAG, "[%s] read_char error sending read request, status (%d)",
              to_string(this->address_).c_str(), status);
            this->char_handle_ = ESP_GATT_ILLEGAL_HANDLE;
          }
          break;
        }
        offset++;
      }
      if (this->char_handle_ == ESP_GATT_ILLEGAL_HANDLE)
      {
        ESP_LOGE(TAG, "[%s] SEARCH_CMPL_EVT char (%s) not found", to_string(this->address_).c_str(),
          this->char_uuid_.to_string().c_str());
        report_error();
      }
      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      if (param->read.conn_id != this->conn_id_ || param->read.handle != this->char_handle_)
        break;
      if (param->read.status != ESP_GATT_OK)
      {
        ESP_LOGW(TAG, "[%s] READ_CHAR_EVT error reading char at handle (%d), status (%d)", to_string(this->address_).c_str(),
          param->read.handle, param->read.status);
        report_error();
        break;
      }

      report_results(param->read.value, param->read.value_len);
      this->state_ = MYHOMEIOT_ESTABLISHED;
      break;
    }
    default:
      break;
  }
  return true;
}

std::string MyHomeIOT_BLEClient::to_string(uint64_t address) const {
  char buffer[20];
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", (uint8_t) (address >> 40), (uint8_t) (address >> 32),
    (uint8_t) (address >> 24), (uint8_t) (address >> 16), (uint8_t) (address >> 8), (uint8_t) (address >> 0));
  return std::string(buffer);
}

}  // namespace myhomeiot_ble_client
}  // namespace esphome

#endif
