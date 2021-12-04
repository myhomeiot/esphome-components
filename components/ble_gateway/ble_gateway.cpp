#include "ble_gateway.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace ble_gateway {

#define HCI_HEADER_LEN 14

static const char *const TAG = "ble_gateway";

std::string scan_rst_to_hci_packet_hex(const esp_ble_gap_cb_param_t::ble_scan_result_evt_param &param) {
  const char *hex = "0123456789ABCDEF";
  char buffer[(HCI_HEADER_LEN + ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX + 1) * 2 + 1];
  uint8_t payload_size = param.adv_data_len + param.scan_rsp_len;

  if (payload_size > ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX) {
    ESP_LOGE(TAG, "Payload size (%d) bigger than maximum (%d)", payload_size,
      ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX);
    return "";
  }

  // HCI Packet Type: HCI Event (0x04), Event Code: LE Meta (0x3E)
  // Sub Event: LE Advertising Report (0x02), Num Reports (0x01)
  snprintf(buffer, sizeof(buffer), "043E%02X0201%02X%02X%02X%02X%02X%02X%02X%02X%02X%*s%02X",
    11 + payload_size + 1, // Total Length
    param.ble_evt_type, // Event Type
    param.ble_addr_type, // Address Type
    param.bda[5], param.bda[4], param.bda[3], param.bda[2], param.bda[1], param.bda[0], 
    payload_size,
    payload_size * 2, "", // Payload filler
    (uint8_t) param.rssi
  );

  char *dest = &buffer[HCI_HEADER_LEN * 2];
  for (int i = 0; i < payload_size; i++) {
    *dest++ = hex[param.ble_adv[i] >> 4];
    *dest++ = hex[param.ble_adv[i] & 0x0F];
  }
  return buffer;
}

std::string mac_address_to_string(uint64_t address) {
  char buffer[20];
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", (uint8_t) (address >> 40), (uint8_t) (address >> 32),
    (uint8_t) (address >> 24), (uint8_t) (address >> 16), (uint8_t) (address >> 8), (uint8_t) (address >> 0));
  return std::string(buffer);
}

void BLEGateway::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE Gateway Devices:");
  for (auto *device : this->devices_)
    ESP_LOGCONFIG(TAG, "  MAC address: %s", mac_address_to_string(device->address_).c_str());
}

bool BLEGateway::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  for (auto *x : this->devices_)
    if (device.address_uint64() == x->address_) {
      auto packet = scan_rst_to_hci_packet_hex(device.get_scan_rst());
      ESP_LOGD(TAG, "[%s] Packet %s", mac_address_to_string(x->address_).c_str(), packet.c_str());
      this->callback_.call(device, packet);
      break;
    }
  return false;
}

}  // namespace ble_gateway
}  // namespace esphome

#endif
