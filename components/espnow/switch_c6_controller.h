#pragma once

#ifdef USE_ESP32

#include "espnow_component.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace esphome::espnow {

// Parsed result similar to Arduino version
enum SwitchC6CommandType : uint8_t {
  COMMAND_UNKNOWN = 0,
  COMMAND_STATUS_RETURN = 1,
  COMMAND_BROADCAST = 2,
  COMMAND_VERSION_RETURN = 3,
};

struct SwitchC6ParsedData {
  SwitchC6CommandType command_type{COMMAND_UNKNOWN};
  std::string mac_address;  // "XXXX-XXXX-XXXX" or "FFFF-FFFF-FFFF" (broadcast)
  bool switch_state{false};
  float voltage{0.0f};
  std::string version;
  bool is_valid{false};
  std::string error_message;
};

// Lightweight controller that builds Switch‑C6 messages and uses ESPNowComponent to send/receive
class SwitchC6Controller : public ESPNowReceivedPacketHandler, public ESPNowBroadcastedHandler {
 public:
  explicit SwitchC6Controller(ESPNowComponent *parent) : parent_(parent) {}

  void register_handlers();

  // Channel helpers
  void set_channel(uint8_t ch);
  uint8_t get_channel() const;

  // Command helpers
  bool send_switch_command(const std::string &device_mac, bool on, bool need_response = false);
  bool send_status_query(const std::string &device_mac);
  bool send_version_query(const std::string &device_mac);
  bool send_custom_message(const std::string &message);

  // Receive hooks
  bool on_received(const ESPNowRecvInfo &info, const uint8_t *data, uint8_t size) override;
  bool on_broadcasted(const ESPNowRecvInfo &info, const uint8_t *data, uint8_t size) override;

  // Parsing
  static SwitchC6ParsedData parse_data(const uint8_t *data, int len);

 private:
  static std::string normalize_mac(const std::string &mac);
  static bool is_valid_mac_format(const std::string &mac);
  static bool is_broadcast_mac(const std::string &mac);
  static float parse_voltage(const std::string &voltage);

  bool ensure_broadcast_peer_added();

  ESPNowComponent *parent_{nullptr};
  bool broadcast_peer_added_{false};
};

}  // namespace esphome::espnow

#endif  // USE_ESP32
