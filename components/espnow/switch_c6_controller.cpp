#ifdef USE_ESP32

#include "switch_c6_controller.h"

#include "espnow_packet.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace esphome::espnow {

void SwitchC6Controller::register_handlers() {
  if (!this->parent_) return;
  this->parent_->register_received_handler(this);
  this->parent_->register_broadcasted_handler(this);
}

void SwitchC6Controller::set_channel(uint8_t ch) {
  if (!this->parent_) return;
  this->parent_->set_wifi_channel(ch);
  this->parent_->apply_wifi_channel();
}

uint8_t SwitchC6Controller::get_channel() const {
  if (!this->parent_) return 0;
  return this->parent_->get_wifi_channel();
}

bool SwitchC6Controller::ensure_broadcast_peer_added() {
  if (this->broadcast_peer_added_) return true;
  if (!this->parent_) return false;
  auto err = this->parent_->add_peer(ESPNOW_BROADCAST_ADDR);
  if (err == ESP_OK) {
    this->broadcast_peer_added_ = true;
    return true;
  }
  return false;
}

bool SwitchC6Controller::send_custom_message(const std::string &message) {
  if (message.empty() || !this->parent_) return false;
  if (!this->ensure_broadcast_peer_added()) return false;
  std::vector<uint8_t> payload(message.begin(), message.end());
  return this->parent_->send(ESPNOW_BROADCAST_ADDR, payload) == ESP_OK;
}

bool SwitchC6Controller::send_switch_command(const std::string &device_mac, bool on, bool need_response) {
  auto mac_norm = normalize_mac(device_mac);
  if (!is_valid_mac_format(mac_norm)) return false;
  uint8_t ch = this->get_channel();
  if (ch < 1 || ch > 14) ch = 1;

  char buf[64];
  int n = std::snprintf(buf, sizeof(buf), "%s=%s;ch=%u%s", mac_norm.c_str(), on ? "1" : "0", ch,
                        need_response ? ";" : "");
  if (n <= 0) return false;
  return this->send_custom_message(std::string(buf, static_cast<size_t>(n)));
}

bool SwitchC6Controller::send_status_query(const std::string &device_mac) {
  auto mac_norm = normalize_mac(device_mac);
  if (!is_valid_mac_format(mac_norm)) return false;
  uint8_t ch = this->get_channel();
  if (ch < 1 || ch > 14) ch = 1;
  char buf[64];
  int n = std::snprintf(buf, sizeof(buf), "%s=?;ch=%u;", mac_norm.c_str(), ch);
  if (n <= 0) return false;
  return this->send_custom_message(std::string(buf, static_cast<size_t>(n)));
}

bool SwitchC6Controller::send_version_query(const std::string &device_mac) {
  auto mac_norm = normalize_mac(device_mac);
  if (!is_valid_mac_format(mac_norm)) return false;
  uint8_t ch = this->get_channel();
  if (ch < 1 || ch > 14) ch = 1;
  char buf[64];
  int n = std::snprintf(buf, sizeof(buf), "%s=V;ch=%u;", mac_norm.c_str(), ch);
  if (n <= 0) return false;
  return this->send_custom_message(std::string(buf, static_cast<size_t>(n)));
}

bool SwitchC6Controller::on_received(const ESPNowRecvInfo &info, const uint8_t *data, uint8_t size) {
  (void)info;
  auto parsed = parse_data(data, size);
  // In this minimal controller, we simply return whether it looked valid
  return parsed.is_valid;
}

bool SwitchC6Controller::on_broadcasted(const ESPNowRecvInfo &info, const uint8_t *data, uint8_t size) {
  (void)info;
  auto parsed = parse_data(data, size);
  return parsed.is_valid;
}

SwitchC6ParsedData SwitchC6Controller::parse_data(const uint8_t *data, int len) {
  SwitchC6ParsedData res;
  if (data == nullptr || len <= 0) {
    res.is_valid = false;
    res.error_message = "Empty data";
    return res;
  }
  std::string s(reinterpret_cast<const char *>(data), static_cast<size_t>(len));
  // Trim
  auto not_space = [](int ch) { return !std::isspace(ch); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  if (s.empty()) {
    res.is_valid = false;
    res.error_message = "Blank string";
    return res;
  }

  // Expect formats like:
  // MAC;status;voltage
  // MAC;version
  auto semi1 = s.find(';');
  if (semi1 == std::string::npos || semi1 == 0) {
    res.is_valid = false;
    res.error_message = "No semicolon separator";
    return res;
  }
  res.mac_address = normalize_mac(s.substr(0, semi1));
  std::string rest = s.substr(semi1 + 1);

  auto semi2 = rest.find(';');
  if (semi2 == std::string::npos) {
    // Version response: MAC;version
    res.version = rest;
    res.command_type = COMMAND_VERSION_RETURN;
    res.is_valid = true;
    return res;
  }

  // Status/broadcast: status;voltage
  std::string status = rest.substr(0, semi2);
  std::string voltage = rest.substr(semi2 + 1);

  // Broadcast vs status: MAC may be FFFF-FFFF-FFFF
  if (is_broadcast_mac(res.mac_address)) {
    res.command_type = COMMAND_BROADCAST;
  } else {
    res.command_type = COMMAND_STATUS_RETURN;
  }
  res.switch_state = (status == "1" || status == "ON" || status == "on");
  res.voltage = parse_voltage(voltage);
  res.is_valid = true;
  return res;
}

std::string SwitchC6Controller::normalize_mac(const std::string &mac) {
  // Extract hex chars only, upper-case, then format XXXX-XXXX-XXXX
  std::string hex;
  hex.reserve(12);
  for (char c : mac) {
    if (std::isxdigit(static_cast<unsigned char>(c))) hex.push_back(static_cast<char>(std::toupper(c)));
  }
  if (hex.size() != 12) {
    // Fallback: uppercase and replace ':' with '-'
    std::string up = mac;
    std::transform(up.begin(), up.end(), up.begin(), [](unsigned char c) { return std::toupper(c); });
    for (char &c : up) {
      if (c == ':') c = '-';
    }
    return up;
  }
  char out[15];
  std::snprintf(out, sizeof(out), "%.*s-%.*s-%.*s", 4, hex.c_str(), 4, hex.c_str() + 4, 4, hex.c_str() + 8);
  return std::string(out);
}

bool SwitchC6Controller::is_valid_mac_format(const std::string &mac) {
  auto m = mac;
  // Accept XXXX-XXXX-XXXX or XX:XX:XX:XX:XX:XX
  if (m.size() == 14 && m[4] == '-' && m[9] == '-') return true;
  if (m.size() == 17 && m[2] == ':' && m[5] == ':' && m[8] == ':' && m[11] == ':' && m[14] == ':') return true;
  // Also accept normalized fallback to 12 hex
  size_t hex_count = 0;
  for (char c : m) {
    if (std::isxdigit(static_cast<unsigned char>(c))) hex_count++;
  }
  return hex_count == 12;
}

bool SwitchC6Controller::is_broadcast_mac(const std::string &mac) { return normalize_mac(mac) == "FFFF-FFFF-FFFF"; }

float SwitchC6Controller::parse_voltage(const std::string &voltage) {
  std::string v = voltage;
  // Remove unit
  v.erase(std::remove(v.begin(), v.end(), 'V'), v.end());
  v.erase(std::remove(v.begin(), v.end(), 'v'), v.end());
  // Trim spaces
  auto not_space = [](int ch) { return !std::isspace(ch); };
  v.erase(v.begin(), std::find_if(v.begin(), v.end(), not_space));
  v.erase(std::find_if(v.rbegin(), v.rend(), not_space).base(), v.end());
  // Convert
  return std::strtof(v.c_str(), nullptr);
}

}  // namespace esphome::espnow

#endif  // USE_ESP32
