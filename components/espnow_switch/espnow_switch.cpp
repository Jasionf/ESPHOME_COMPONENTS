#include "espnow_switch.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/application.h"

namespace esphome {
namespace espnow_switch {

static const char *const TAG = "espnow_switch";

void ESPNowSwitch::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ESPNow Switch...");
  // 无需在 C++ 层注册广播回调；依赖乐观状态与重试发送
}

void ESPNowSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "ESPNow Switch:");
  LOG_SWITCH("  ", "Switch", this);
  ESP_LOGCONFIG(TAG, "  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
                this->mac_address_[0], this->mac_address_[1], this->mac_address_[2],
                this->mac_address_[3], this->mac_address_[4], this->mac_address_[5]);
  ESP_LOGCONFIG(TAG, "  Response Token: %s", this->response_token_.c_str());
  ESP_LOGCONFIG(TAG, "  Retry Count: %d", this->retry_count_);
  ESP_LOGCONFIG(TAG, "  Retry Interval: %dms", this->retry_interval_);
}

void ESPNowSwitch::write_state(bool state) {
  ESP_LOGD(TAG, "Setting switch to %s", state ? "ON" : "OFF");
  
  // 设置命令：1 = ON, 0 = OFF
  std::string cmd = state ? "1" : "0";
  this->current_command_ = cmd;
  
  // 发送命令
  this->send_command_(cmd);
  
  // 发布状态（乐观模式）
  this->publish_state(state);
}

void ESPNowSwitch::send_command_(const std::string &cmd) {
  this->response_received_ = false;
  
  // 重试发送命令
  for (uint8_t i = 0; i < this->retry_count_; i++) {
    if (this->response_received_) {
      ESP_LOGI(TAG, "Response confirmed, stopping retries");
      break;
    }
    
    // 获取当前 WiFi 信道
    int channel = this->espnow_->get_wifi_channel();
    
    // 构建消息格式: MAC-ADDRESS=CMD;ch=CHANNEL;
    char data[64];
    snprintf(data, sizeof(data), "%02X%02X-%02X%02X-%02X%02X=%s;ch=%d;",
             this->mac_address_[0], this->mac_address_[1],
             this->mac_address_[2], this->mac_address_[3],
             this->mac_address_[4], this->mac_address_[5],
             cmd.c_str(), channel);
    
    // 转换为 vector
    std::vector<uint8_t> payload(data, data + strlen(data));
    
    // 发送 ESPNow 消息
    esp_err_t result = this->espnow_->send(this->mac_address_, payload, nullptr);
    
    if (result == ESP_OK) {
      ESP_LOGV(TAG, "ESPNow message sent (attempt %d/%d): %s", i + 1, this->retry_count_, data);
    } else {
      ESP_LOGW(TAG, "Failed to send ESPNow message (attempt %d/%d)", i + 1, this->retry_count_);
    }
    
    // 等待一段时间后重试
    if (i < this->retry_count_ - 1) {
      delay(this->retry_interval_);
    }
  }
  
  if (!this->response_received_) {
    ESP_LOGW(TAG, "No response received after %d attempts", this->retry_count_);
  }
}

void ESPNowSwitch::on_espnow_broadcast(const uint8_t *data, size_t len) {
  // 将数据转换为字符串
  std::string response((char*)data, len);
  
  // 检查响应中是否包含我们的匹配令牌
  if (response.find(this->response_token_) != std::string::npos) {
    ESP_LOGI(TAG, "Response received (matched token): %s", this->response_token_.c_str());
    this->response_received_ = true;
  }
}

}  // namespace espnow_switch
}  // namespace esphome
