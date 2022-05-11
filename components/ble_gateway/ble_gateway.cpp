#ifdef USE_ESP32

#include "ble_gateway.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ble_gateway {

#define HCI_HEADER_LEN 14

static const char *const TAG = "ble_gateway";

// https://stackoverflow.com/questions/25713995/how-to-decode-a-bluetooth-le-package-frame-beacon-of-a-freetec-px-1737-919-b
std::string BLEGateway::scan_result_to_hci_packet_hex(const esp_ble_gap_cb_param_t::ble_scan_result_evt_param &scan_result) {
  const char *hex = "0123456789ABCDEF";
  char buffer[(HCI_HEADER_LEN + ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX + 1) * 2 + 1];
  uint8_t payload_size = scan_result.adv_data_len + scan_result.scan_rsp_len;

  if (payload_size > ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX) {
    ESP_LOGE(TAG, "Payload size (%d) bigger than maximum (%d)", payload_size,
      ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX);
    return "";
  }

  // HCI Packet Type: HCI Event (0x04), Event Code: LE Meta (0x3E)
  // Sub Event: LE Advertising Report (0x02), Num Reports (0x01)
  snprintf(buffer, sizeof(buffer), "043E%02X0201%02X%02X%02X%02X%02X%02X%02X%02X%02X%*s%02X",
    11 + payload_size + 1, // Total Length
    scan_result.ble_evt_type, // Event Type
    scan_result.ble_addr_type, // Address Type
    scan_result.bda[5], scan_result.bda[4], scan_result.bda[3], scan_result.bda[2], scan_result.bda[1], scan_result.bda[0], 
    payload_size,
    payload_size * 2, "", // Payload filler
    (uint8_t) scan_result.rssi
  );

  char *dest = &buffer[HCI_HEADER_LEN * 2];
  for (int i = 0; i < payload_size; i++) {
    *dest++ = hex[scan_result.ble_adv[i] >> 4];
    *dest++ = hex[scan_result.ble_adv[i] & 0x0F];
  }
  return buffer;
}

std::string address_uint64_to_string(uint64_t address) {
  char buffer[17 + 1];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t) (address >> 40), (uint8_t) (address >> 32),
    (uint8_t) (address >> 24), (uint8_t) (address >> 16),
    (uint8_t) (address >> 8), (uint8_t) (address >> 0));
  return std::string(buffer);
}

void BLEGateway::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE Gateway: Discovery %s, %d device(s) configured:", YESNO(this->discovery_), this->devices_.size());
  for (auto device : this->devices_)
    ESP_LOGCONFIG(TAG, "  MAC address: %s", address_uint64_to_string(device).c_str());
}

bool BLEGateway::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (this->discovery_ || std::find(this->devices_.begin(), this->devices_.end(), device.address_uint64()) != this->devices_.end()) {
    auto packet = scan_result_to_hci_packet_hex(device.get_scan_result());
    ESP_LOGD(TAG, "[%s] Packet %s", address_uint64_to_string(device.address_uint64()).c_str(), packet.c_str());
    this->callback_.call(device, packet);
  }
  return false;
}

void BLEGateway::add_device(uint64_t device) {
  if (std::find(this->devices_.begin(), this->devices_.end(), device) == this->devices_.end())
    this->devices_.push_back(device);
  else
    ESP_LOGW(TAG, "Device with MAC address (%s) already exists", address_uint64_to_string(device).c_str());
}

void BLEGateway::set_devices(std::string devices) {
  const char *s = devices.c_str();
  int len = strlen(s);
  ESP_LOGD(TAG, "set_devices: (%s)", s);

  if (len % 12 == 0) {
    this->devices_.clear();
    for (int i = 0; i < len / 12; i++) {
      uint64_t mac_address;
      sscanf(&s[i * 12], "%12llx", &mac_address);
      add_device(mac_address);
    }
  }
  else
    ESP_LOGE(TAG, "set_devices: Devices lengths (%d) must be a multiple of 12", len);
}

}  // namespace ble_gateway
}  // namespace esphome

#endif
