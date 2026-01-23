# 麦克风故障排查指南

## 问题诊断步骤

### 1. **硬件连接检查**
```
ES7210 (麦克风 ADC) - 引脚检查：
- SDA (GPIO38): 数据线
- SCL (GPIO39): 时钟线
- GND: 接地
- VCC: 3.3V 电源

I2S 音频线：
- I2S_DIN_PIN (GPIO5): 麦克风数据输入
- I2S_BCLK_PIN (GPIO6): 位时钟
- I2S_LRCLK_PIN (GPIO8): 左右通道时钟
```

### 2. **运行 I2C 通信测试**

#### 方法 A: 使用 Python 脚本
```bash
# 如果有 Raspberry Pi 或其他 I2C 硬件
python3 test_microphone.py
```

#### 方法 B: 使用 ESPHome 测试固件
编译并烧录 `microphone_test.yaml`:
```bash
esphome run microphone_test.yaml
```

查看日志输出，应该看到：
```
✓ I2C 通信正常
✓ 芯片 ID: 0x20
✓ 复位命令已发送
✓ 增益设置: 0x18
✓ ADC 已使能
```

### 3. **常见问题及解决方案**

| 问题 | 症状 | 解决方案 |
|------|------|--------|
| I2C 通信失败 | 显示 "无法读取芯片 ID" | 检查 GPIO38/39 连接、上拉电阻 |
| 芯片 ID 错误 | 读到的芯片 ID 不是 0x20 | 检查硬件是否为 ES7210 |
| ADC 无输出 | 麦克风无声音 | 检查 GPIO5 (I2S_DIN) 连接 |
| 增益太低 | 声音很小 | 调整增益值 (0x00-0x1F) |
| 采样率不匹配 | 声音失真 | 检查 sample_rate 是否为 16000 Hz |

### 4. **I2S 音频总线诊断**

检查日志中是否有 I2S 错误：
```
[i2s_audio] I2S 总线初始化成功
[microphone] 麦克风初始化完成
```

### 5. **增益调整**

ES7210 增益寄存器值 (0x20):
```
0x00 = -24 dB (最小)
0x08 = -12 dB
0x10 = 0 dB
0x18 = +12 dB
0x20 = +24 dB (最大)
```

如果声音太小，增加增益；如果失真，降低增益。

### 6. **PDM 模式检查**

你的配置中有 `pdm: true`，确保：
- ES7210 支持 PDM 模式
- I2S 时钟配置正确
- 采样率设置为 16000 Hz

### 7. **详细日志输出**

在配置中添加：
```yaml
logger:
  level: VERY_VERBOSE
  logs:
    es7210_test: INFO
    i2s_audio: DEBUG
    microphone: DEBUG
```

然后运行固件查看详细的初始化日志。

## 测试流程

1. ✓ 验证 I2C 通信
2. ✓ 检查芯片响应
3. ✓ 复位并配置 ADC
4. ✓ 测试 I2S 音频总线
5. ✓ 调整增益参数
6. ✓ 进行语音录制测试

## 如果仍有问题

请检查以下日志信息：
- `[i2c]` 日志：查看 I2C 扫描结果
- `[es7210_test]` 日志：查看芯片状态
- `[i2s_audio]` 日志：查看音频总线状态
- `[microphone]` 日志：查看麦克风初始化

## 参考资源

- ES7210 数据手册寄存器地址
- I2S 音频总线配置
- ESP32-S3 I2C/I2S 引脚配置
