# STC12 CMake + Ninja + SDCC 模板工程

Windows 环境下面向 STC12 系列（8051 内核）MCU 的 **CMake + Ninja + SDCC** 模板工程，基于 [uni-STC](https://codeberg.org/20-100/uni-STC) 开源寄存器定义与 HAL 库构建。

STC 官方工具链长期围绕 Keil μVision，存在商业授权风险；本工程演示了一条完全免费的替代路径：SDCC（GPL）+ CMake + Ninja。uni-STC 此前由 Makefile 驱动、以 Linux 环境为主，本工程为其补齐了 Windows + CMake + Ninja 的完整工作流——对 uni-STC 的工程化落地又是一大进步。

开箱即含：系统节拍（Timer0 1 ms）、中断驱动 UART 控制台（printf 重定向）、事件调度框架、环形缓冲、IO 消抖等嵌入式常用基础设施，适合作为新 STC12 项目的起点。

## 特性

- 构建：CMake + Ninja 增量编译，自动生成 `compile_commands.json` 供 clangd 索引
- 控制台：UART1 中断收发 + 环形缓冲 FIFO，SDCC printf 重定向
- 系统节拍：Timer0 1 ms 中断
- 业务框架：节拍广播 + 事件调度器，任务以事件回调方式组织
- UART 两套配置 `#if` 切换，便于与 STC 官方例程对照（见技术要点）

## 硬件基线

| 项目 | 值 |
|---|---|
| MCU | STC12C5A56S2（STC12C5AxxS2 系列，全系列寄存器兼容，Flash 容量随型号变化） |
| 晶振 | 11.0592 MHz（STC12 无内部 RC，必须外接晶振） |
| 控制台 | UART1：P3.0 (RXD) / P3.1 (TXD)，115200 8N1 |

更换型号/封装时只需调整 `CMakeLists.txt` 中的 `MCU_VARIANT` / `MCU_FREQ_HZ` 与容量参数；uni-STC 对 STC12/15/8 各系列均提供对应头文件。

## 目录结构

```
├── src/                  # 应用层：main、UART/定时器初始化、板级总线、业务任务
│   └── userTasks/        # 业务任务示例（事件驱动 + 状态机写法）
├── compoent/             # 通用组件：ringBuffer、事件调度器、总线 IO 管理、消抖、调试宏
├── uni-stc/              # uni-STC 库 vendored 副本（寄存器定义 + HAL，许可见其 LICENSE）
├── tips.txt              # SDCC + STC12 开发笔记
├── CMakeLists.txt
└── build.bat             # 构建入口
```

## 业务流程（模板惯例）

模板演示了一套典型的单循环事件驱动流程，新项目可照此组织：

1. `main()` 只做初始化：时钟防御 → 开总中断 → 控制台 → 系统节拍 → 注册各任务
2. Timer0 产生 1 ms 节拍，经 `tickBroadcast` 统一广播，任务收到 `TICK` 事件
3. 任务在 TICK 处理中分频出秒节拍做周期逻辑；IO 变化经消抖后转为事件投递给调度器
4. 任务内事件回调为纯状态机、不阻塞；诊断输出一律走重定向后的 printf

## 环境搭建（Windows）

CMake 和 Ninja 均可用 winget 一键安装：

```powershell
winget install --id Kitware.CMake -e
winget install --id Ninja-build.Ninja -e
```

SDCC 需要自行到网上下载 Windows 安装程序（本项目基于 SDCC 4.x 验证）：

1. 从 <https://sdcc.sourceforge.net/snap.php> 下载 `sdcc-<version>-x86_64-setup.exe`
2. 安装到**默认目录** `C:\Program Files\SDCC`（`build.bat` 会自动把该目录加入 PATH）

## 构建

```bat
build.bat            :: 首次自动 configure，之后增量构建
build.bat clean      :: 清理构建目录
build.bat rebuild    :: 全量重建（头文件中的调用约定变化时必须全量重编）
```

产物为 `output/` 下的 hex 文件。

## 烧录与调试

- 用 STC-ISP / AiCube-ISP 烧录 hex（冷启动：断电再上电或按复位键）
- STC-ISP 的时钟分频选项建议设为 1 分频（固件启动时也会防御性地清零 `CLKDIV`）
- 串口终端 115200 8N1

## 技术要点（踩坑记录）

1. **SDCC 的中断向量表只在包含 `main()` 的模块里生成**，且 ISR 必须在该模块内以带 `__interrupt(n)` 属性的原型声明；
   普通 `void isr(void);` 原型不会生成向量入口，向量地址会被其他代码占据（见 `src/Main.c` 顶部注释）。
2. **波特率计算必须用精确晶振值**：`MCU_FREQ` 定义为 `11059200ul`；若用 `11059000`（kHz 截断值），
   整数除法会把重载值从 0xFD 截成 0xFE，实际波特率变成 172800——症状是终端收到满屏 `f`（0x66）。
3. UART1 有两套配置，由 `src/userUART_init.c` 顶部的 `UART_CFG_OFFICIAL` 切换：
   - `1`：STC 官方例程配置——Timer1 12T、9600 波特率、不碰 AUXR
   - `0`（默认）：Timer1 1T（AUXR.T1x12）、115200 波特率
4. UART 收发均为中断驱动，FIFO 复用 ringBuffer 组件；`ringBuf_push/pop/count` 已声明为
   `REENTRANT`，可安全地在 ISR 与主循环之间共用。
5. `--model-large` 下所有 auto 局部变量默认落在 XRAM；函数 reentrant 化会额外消耗 IRAM，按需取舍。
6. SDCC + 8051 的其他注意事项（大小端、性能差异等）记录在 `tips.txt`。

## 资源占用（当前固件）

| 资源 | 占用 |
|---|---|
| XRAM | ~891 / 1024 B |
| IRAM 栈 | 112 B |
| Flash | ~20 / 56 KB |

## 许可

`uni-stc/` 目录为 [uni-STC](https://codeberg.org/20-100/uni-STC) 开源库的 vendored 副本，遵循 BSD-3-Clause；
项目其余代码暂未指定许可证。
