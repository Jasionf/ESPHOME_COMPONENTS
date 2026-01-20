# Si5351 ESPHome Component

Si5351 是一个可编程时钟生成器，这是其 ESPHome 组件实现。

## 文件结构

```
custom_components/
└── si5351/
    ├── __init__.py          # ESPHome 配置和代码生成
    ├── si5351.h             # C++ 头文件
    ├── si5351.cpp           # C++ 实现文件
    └── manifest.json        # 组件元数据
```

## 安装

1. 在 ESPHome 配置目录创建 custom components 文件夹：
```bash
mkdir -p esphome/custom_components/si5351
```

2. 将以下文件复制到该目录：
   - `__init__.py`
   - `si5351.h`
   - `si5351.cpp`
   - `manifest.json`

## 配置示例

在 ESPHome YAML 配置中添加：

```yaml
# I2C 总线配置
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz

# Si5351 时钟生成器配置
si5351:
  id: clock_gen
  address: 0x60  # Si5351 默认 I2C 地址
```

## 功能

| 函数 | 说明 |
|------|------|
| `write_reg(reg, value)` | 写入单个寄存器 |
| `write_regs(start_reg, data, length)` | 写入多个连续寄存器 |
| `read_reg(reg, out_value)` | 读取单个寄存器 |

## 初始化流程

初始化序列执行以下步骤：

1. **禁用所有输出** - 写入 0xFF 到寄存器 3
2. **关闭输出驱动** - 写入 0x80 到寄存器 16-18
3. **设置晶体负载** - 写入 0xC0 到寄存器 183（内部 10pF 电容）
4. **配置 Multisynth NA** - 8 字节参数写入寄存器 26
5. **配置 Multisynth1** - 8 字节参数写入寄存器 50
6. **CLK1 控制设置** - 写入 0x4C 到寄存器 17（整数模式，MultiSynth 1 作为源）
7. **PLL 复位** - 写入 0xA0 到寄存器 177
8. **启用所有输出** - 写入 0x00 到寄存器 3

## 寄存器映射

| 地址 | 名称 | 功能 |
|------|------|------|
| 3 | OUTPUT_ENABLE_CONTROL | 输出使能控制 |
| 9 | OEB_PIN_ENABLE_CONTROL | OEB 引脚使能 |
| 16-18 | OUTPUT_DRIVERS | 输出驱动配置 |
| 17 | CLK1_CONTROL | CLK1 控制 |
| 26-33 | MULTISYNTH_NA | MultiSynth NA 参数 |
| 50-57 | MULTISYNTH1 | MultiSynth 1 参数 |
| 177 | PLL_RESET | PLL 复位 |
| 183 | XTAL_LOAD_CAP | 晶体负载电容 |

## 时钟输出配置

当前配置输出以下时钟频率：
- **CLK0** - 禁用
- **CLK1** - 启用（通过 Multisynth1）
- **CLK2** - 禁用

可通过修改 Multisynth 参数调整输出频率。

## 故障排除

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| Si5351 not responding | I2C 通讯失败 | 检查 SDA/SCL 连接、地址设置 |
| 时钟输出不稳定 | PLL 配置问题 | 检查 Multisynth 参数、晶体配置 |
| 输出频率不对 | Multisynth 参数错误 | 使用 Si5351 时钟计算器验证参数 |

## 与 IDF 版本的主要区别

- 使用 ESPHome 的 `I2CDevice` 基类替代直接的 I2C 驱动调用
- 使用 `Component` 虚函数替代回调系统
- 使用 Python 配置模式替代 C 结构体初始化
- 寄存器常数定义为类常量

## 参考资源

- Si5351 数据手册: Silicon Labs Si5351A/B/C 时钟生成器
- ESPHome I2C 组件: https://esphome.io/components/i2c.html

## 许可证

MIT License
