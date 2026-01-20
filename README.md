# AW87559 ESPHome Component

AW87559 是一个高功率音频放大器，这是其 ESPHome 组件实现。

## 文件结构

```
custom_components/
└── aw87559/
    ├── __init__.py          # ESPHome 配置和代码生成
    ├── aw87559.h            # C++ 头文件
    ├── aw87559.cpp          # C++ 实现文件
    ├── manifest.json        # 组件元数据
    └── README.md            # 本文件
```

## 安装

1. 在 ESPHome 配置目录创建 custom components 文件夹：
```bash
mkdir -p esphome/custom_components/aw87559
```

2. 将以下文件复制到该目录：
   - `__init__.py`
   - `aw87559.h`
   - `aw87559.cpp`
   - `manifest.json`

## 配置示例

在 ESPHome YAML 配置中添加：

```yaml
# I2C 总线配置
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz

# AW87559 放大器配置
aw87559:
  id: my_amplifier
  address: 0x58              # 或 0x59，取决于硬件地址设置
  reset_pin: GPIO12          # 可选，GPIO 复位引脚
```

## 功能

| 函数 | 说明 |
|------|------|
| `write_reg(reg, value)` | 写入单个寄存器 |
| `read_reg(reg, out_value)` | 读取单个寄存器 |
| `apply_table(pairs, length)` | 批量写入寄存器序列 |

## 寄存器定义

- `0x00` - CHIPID（应为 0x5A）
- `0x01` - SYSCTRL（系统控制，包括 PA 使能位）
- `0x02` - BATSAFE（电池保护）
- `0x03` - BSTOVR（Boost 过压）
- `0x04` - BSTVPR（Boost 电压）
- `0x05` - PAGR（PA 增益）
- `0x06-0x0A` - 其他 PA 控制寄存器

## 初始化流程

1. GPIO 复位（如果配置）
2. 读取 ID 寄存器验证设备
3. 写入 0x78 到 SYSCTRL 寄存器启用 PA

## 故障排除

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| AW87559 not responding | I2C 通讯失败 | 检查 SDA/SCL 连接、地址设置 |
| ID 寄存器不是 0x5A | 芯片不匹配或通讯问题 | 验证是否为 AW87559 芯片 |
| PA 无法启用 | I2C 写入失败 | 检查 I2C 总线状态 |

## 与 IDF 版本的主要区别

- 使用 ESPHome 的 `I2CDevice` 基类替代直接的 I2C 驱动调用
- 使用 `Component` 虚函数替代回调系统
- 使用 Python 配置模式替代 C 结构体初始化
- 自动内存管理，无需手动分配/释放

## 许可证

MIT License
