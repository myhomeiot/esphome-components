#include "ble_gateway.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include <sstream>
#include <iomanip>

namespace esphome {
namespace ble_gateway {

static const char *const TAG = "ble_gateway";

// https://stackoverflow.com/questions/41633574/stdhex-and-stdsetw-not-working-with-some-characters
// https://stackoverflow.com/questions/25713995/how-to-decode-a-bluetooth-le-package-frame-beacon-of-a-freetec-px-1737-919-b
std::string scan_rst_to_hci_packet_hex(const esp_ble_gap_cb_param_t::ble_scan_result_evt_param &param) {
  std::stringstream ss;
  uint8_t payload_size = param.adv_data_len + param.scan_rsp_len;

  ss << std::uppercase << std::setfill('0') << std::hex;

  ss << "043E"; // HCI Packet Type: HCI Event (0x04), Event Code: LE Meta (0x3E)
  ss << std::setw(2) << (int)(unsigned char) (11 + payload_size + 1); // Total Length
  ss << "0201"; // Sub Event: LE Advertising Report (0x02), Num Reports (0x01)
  ss << std::setw(2) << (int)(unsigned char) param.ble_evt_type; // Event Type
  ss << std::setw(2) << (int)(unsigned char) param.ble_addr_type; // Peer Address Type
  for (int i = ESP_BD_ADDR_LEN - 1; i >= 0; i--)
    ss << std::setw(2) << (int)(unsigned char) param.bda[i];
  ss << std::setw(2) << (int)(unsigned char) payload_size;
  for (int i = 0; i < payload_size; i++)
    ss << std::setw(2) << (int)(unsigned char) param.ble_adv[i];
  ss << std::setw(2) << (int)(unsigned char) param.rssi;

  return ss.str();
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
      this->callback_.call(packet);
      break;
    }
  return false;
}

}  // namespace ble_gateway
}  // namespace esphome

#endif
