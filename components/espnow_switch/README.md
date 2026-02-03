# ESPNow Switch Component

这是一个用于 ESPHome 的 ESPNow 开关组件，可以轻松地通过 ESP-NOW 协议控制多个远程继电器设备。

## 功能特点

- 🔄 自动重试机制，确保命令可靠传输
- 📡 支持响应确认，知道命令是否成功送达
- 🎛️ 支持多个开关设备，配置简单
- ⚙️ 可自定义重试次数和间隔
- 📝 详细的日志输出，便于调试

## 安装

将此组件复制到你的 ESPHome 配置目录下的 `components` 文件夹中。

## 配置示例

### 基础配置

```yaml
esphome:
  name: my-espnow-controller
  
esp32:
  board: esp32dev
  framework:
    type: esp-idf

# 配置 ESPNow 组件
espnow:
  id: espnow1
  auto_add_peer: true

# 添加 ESPNow 开关
switch:
  - platform: espnow_switch
    name: "客厅灯开关"
    espnow_id: espnow1
    mac_address: "B4:3A:45:81:EC:70"
    response_token: "142B-2F9F-8704"
```

### 多设备配置

```yaml
espnow:
  id: espnow1
  auto_add_peer: true

switch:
  # 设备 1
  - platform: espnow_switch
    name: "客厅灯"
    espnow_id: espnow1
    mac_address: "B4:3A:45:81:EC:70"
    response_token: "142B-2F9F-8704"
    retry_count: 12
    retry_interval: 300
  
  # 设备 2
  - platform: espnow_switch
    name: "卧室灯"
    espnow_id: espnow1
    mac_address: "AA:BB:CC:DD:EE:FF"
    response_token: "1234-5678-9ABC"
    retry_count: 30
    retry_interval: 150
  
  # 设备 3
  - platform: espnow_switch
    name: "厨房插座"
    espnow_id: espnow1
    mac_address: "11:22:33:44:55:66"
    response_token: "ABCD-EFGH-IJKL"
```

## 配置选项

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `name` | string | 是 | - | 开关的名称 |
| `espnow_id` | id | 是 | - | ESPNow 组件的 ID |
| `mac_address` | string | 是 | - | 目标设备的 MAC 地址（格式：AA:BB:CC:DD:EE:FF） |
| `response_token` | string | 是 | - | 与远端设备响应匹配的令牌（用于确认响应） |
| `retry_count` | int | 否 | 12 | 发送命令的最大重试次数（1-100） |
| `retry_interval` | int | 否 | 300 | 每次重试之间的间隔时间（毫秒，10-5000） |

## 工作原理

1. **发送命令**: 当开关状态改变时，组件会构造一个包含 MAC 地址、命令和 WiFi 信道信息的消息
2. **自动重试**: 如果没有收到响应确认，组件会自动重试发送命令
3. **响应确认**: 远程设备收到命令后，会发送包含 `response_token` 的广播消息作为确认
4. **停止重试**: 一旦收到确认响应，组件会停止重试

## 消息格式

### 发送格式
```
B43A-4581-EC70=1;ch=6;
```
- 前 12 位：MAC 地址（去掉冒号，每 4 位用 `-` 分隔）
- `=` 后面：命令（1=开，0=关）
- `ch=`：当前 WiFi 信道

### 响应格式
远程设备应在响应消息中包含 `response_token`，例如：
```
142B-2F9F-8704
```

## 与原始实现的对比

### 原始实现（复杂）
```yaml
globals:
  - id: current_cmd
    type: std::string
    restore_value: no
    initial_value: '""'
  - id: response_received
    type: bool
    restore_value: no
    initial_value: 'false'

espnow:
  id: espnow1
  auto_add_peer: true
  peers:
    - B4:3A:45:81:EC:70
  on_receive:
    - logger.log: ...
  on_broadcast:
    - lambda: |-
        std::string response((char*)data, size);
        if(response.find("142B-2F9F-8704") != std::string::npos) {
          ESP_LOGI("main", "Response received from device");
          id(response_received) = true;
        }
    - logger.log: ...

script:
  - id: send_switch_command
    mode: restart
    then:
      - globals.set: ...
      - repeat: ...

switch:
  - platform: template
    name: "SwitchC6"
    optimistic: true
    turn_on_action:
      - globals.set: ...
      - script.execute: send_switch_command
    turn_off_action:
      - globals.set: ...
      - script.execute: send_switch_command
```

### 新组件（简单）
```yaml
espnow:
  id: espnow1
  auto_add_peer: true
  on_broadcast:
    - lambda: |-
        // 将广播的数据传递给组件用于停止重试
        id(sw1).handle_broadcast(data, size);

switch:
  - platform: espnow_switch
    id: sw1
    name: "SwitchC6"
    espnow_id: espnow1
    mac_address: "B4:3A:45:81:EC:70"
    response_token: "142B-2F9F-8704"
    retry_count: 12
    retry_interval: 300
```

## 调试

将日志级别设置为 `VERBOSE` 以查看详细的调试信息：

```yaml
logger:
  level: VERBOSE
```

这会显示：
- 每次发送尝试的详细信息
- 接收到的响应
- 重试状态

## 注意事项

- 确保 `mac_address` 格式正确（使用冒号或短横线分隔）
- `response_token` 必须与远程设备返回的标识符匹配
- ESP-NOW 依赖 WiFi，但不需要连接到路由器
- 如果设备距离太远，可能需要增加 `retry_count`

## 许可

MIT License
