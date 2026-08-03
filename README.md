# STC12 CMake + Ninja + SDCC 模板工程

Windows 环境下面向 STC12 系列（8051 内核）MCU 的 **CMake + Ninja + SDCC** 模板工程，基于 [uni-STC](https://codeberg.org/20-100/uni-STC) 开源寄存器定义与 HAL 库构建。

在国内高校电子相关专业的单片机课程中，STC51 系列是被广泛使用的入门平台，课堂通常配套 Keil μVision。但 Keil 是商业软件，存在授权风险；本工程为入门学习者和业余开发者提供一条完全免费的替代路径：SDCC（GPL 编译器）+ CMake + Ninja，并沉淀为可直接套用的模板。uni-STC 此前由 Makefile 驱动、以 Linux 环境为主，本工程为其补齐了 Windows + CMake + Ninja 的完整工作流——对 uni-STC 的工程化落地又是一大进步。

开箱即含：系统节拍（Timer0 1 ms）、中断驱动 UART 控制台（printf 重定向）、事件调度框架、环形缓冲、IO 消抖等嵌入式常用基础设施，适合作为新 STC12 项目的起点。

**适合谁**：刚学完 C 语言基础、开始学单片机的学生，以及想脱离 Keil 的个人开发者。不需要预先掌握单片机知识——本工程自身就是一个涵盖定时器、串口、中断与事件框架的完整示例，所用工具下文都会从零介绍。

## 特性

- 构建：CMake + Ninja 增量编译，自动生成 `compile_commands.json` 供 clangd 索引
- 控制台：UART1 中断收发 + 环形缓冲 FIFO，SDCC printf 重定向
- 命令树：cmdTree 静态模式命令解析器（自 CH58x 工程移植，节点数/深度/缓冲区可配），内置 `reset` / `help`
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

**适用范围**：SDCC 与 uni-STC 仅覆盖 8051 内核的 STC 系列（STC8/12/15/90 等）；C251（MCS-251）内核的新型号（STC16、STC32 系列）SDCC 没有对应后端，uni-STC 也明确不支持（见 `uni-stc/README`），这类芯片目前只能使用 Keil C251。

> 关注：STC 官方论坛有一个**实验性支持 C251 的 SDCC 分支**（见
> <https://www.stcaimcu.com/thread-25076-1-1.html>），可留意其进展，但属于实验性质、尚未并入上游，建议生产项目仍以 Keil C251 为准。

## 目录结构

```
├── src/                  # 应用层：main、UART/定时器初始化、板级总线、业务任务
│   └── userTasks/        # 业务任务示例（事件驱动 + 状态机写法）
├── compoent/             # 通用组件：ringBuffer、事件调度器、总线 IO 管理、消抖、命令树、调试宏
├── uni-stc/              # uni-STC 库 vendored 副本（寄存器定义 + HAL，许可见其 LICENSE）
├── tools/                # 构建辅助脚本：stack_usage.py（SDCC 静态栈消耗分析）+ stack_config.json
├── tips.txt              # SDCC + STC12 开发笔记
├── CMakeLists.txt
└── build.bat             # 构建入口
```

## 业务流程（模板惯例）

模板演示了一套典型的单循环事件驱动流程，新项目可照此组织：

1. `main()` 只做初始化：时钟防御 → 开总中断 → 控制台 → 系统节拍 → 注册各任务
2. Timer0 产生 1 ms 节拍，经 `tickBroadcast` 统一广播，任务收到 `TICK` 事件
3. 业务任务在 TICK 处理中采样 IO（10 ms 限速 + 30 ms 消抖），消抖边沿转为事件投递给调度器，并分频出秒节拍做周期逻辑
4. 任务内事件回调为纯状态机、不阻塞；诊断输出一律走重定向后的 printf
5. 调度器 idle 钩子只保留节拍广播这类框架行为，不放任何业务逻辑
6. 串口命令处理挂在主任务的 TICK 里（`cmds_poll()`）：每 300 ms 把 RX FIFO 一次性抽出（丢弃 CR/LF）直接交给命令树解析——不攒行、不单开任务，省 XRAM

## 环境搭建（Windows）

先认识工具链里各角色的分工——Keil 里点一下"编译"完成的事，这里由几个免费工具各司其职：

- **SDCC**：编译器，把 C 代码翻译成单片机能运行的机器码（替代 Keil C51 编译器）
- **CMake**：工程描述工具，读取 `CMakeLists.txt` 生成构建脚本（替代 Keil 的工程文件管理）
- **Ninja**：构建执行器，按脚本完成编译、链接（替代 Keil 的 F7）
- **Python 3**：构建后运行的静态栈分析脚本 `tools/stack_usage.py` 的解释器（CMake 配置期强制要求）
- **clangd**（可选）：编辑器里的代码助手，提供补全与跳转定义

CMake、Ninja 和 Python 均可用 winget 一键安装：

```powershell
winget install --id Kitware.CMake -e
winget install --id Ninja-build.Ninja -e
winget install --id Python.Python.3.12 -e
```

`stack_usage.py` 只用到 Python 标准库（argparse/json/os/re/subprocess/sys），**无需任何 pip 第三方库**；装好 Python 即可直接运行。若想进一步分析 SDCC 库函数（printf 链等）的栈消耗，还需要 `s51`（ucsim 反汇编器，随 SDCC 一起安装，构建时自动发现，缺失时该部分分析自动跳过）。

SDCC 需要自行到网上下载 Windows 安装程序（本项目基于 SDCC 4.x 验证）：

1. 从 <https://sdcc.sourceforge.net/snap.php> 下载 `sdcc-<version>-x86_64-setup.exe`
2. 安装到**默认目录** `C:\Program Files\SDCC`（`build.bat` 会自动把该目录加入 PATH）

## 构建

```bat
build.bat            :: 首次自动 configure，之后增量构建
build.bat clean      :: 清理构建目录
build.bat rebuild    :: 清理后从零全量重建
```

产物为 `output/` 下的 hex 文件。

构建系统会跟踪头文件依赖：SDCC 的 `-MMD` 为每个模块生成依赖文件（`build/obj/*.d`），
Ninja 据此精确重编受头文件修改影响的模块；只修改 `.c` 文件时仅重编该文件 + 重新链接。
注意 `-MMD` 必须直接传给 sdcc 驱动，经 `-Wp` 转给预处理器会破坏编译流水线。

## 从 Keil 迁移

课堂上用惯 Keil 的话，对照关系如下：

| Keil μVision | 本工程 |
|---|---|
| 工程文件（.uvprojx） | `CMakeLists.txt`（纯文本，可进版本库） |
| F7 编译 | `build.bat` |
| Output 目录里的 .hex | `output/` 下的 .hex |
| STC-ISP 烧录 | 完全相同 |
| reg52.h / STC 官方 Keil 头文件 | uni-STC 头文件（SDCC 语法，见 `uni-stc/include`） |
| 编辑器代码补全 | clangd（见下文） |

## clangd 代码索引

SDCC 本身不带 IDE 能力；本工程接入 clangd，在编辑器中获得代码补全、跳转定义、查找引用等体验。
先理清分工：**真正干活的是安装在系统里的 clangd 程序**（语言服务器，随 LLVM 提供），
编辑器扩展只是连接它的"客户端"——只装扩展、不装程序是不会生效的。
（clangd 只辅助读写代码——语义检查由 clang 完成，与 SDCC 语义并非完全一致，诊断噪音已在 `.clangd` 中关闭。）

### 1. 安装 clangd 程序

winget 中有两个可选包（二选一）：

```powershell
winget install --id LLVM.clangd -e    # 仅 clangd 语言服务器，轻量（推荐）
winget install --id LLVM.LLVM -e      # 完整 LLVM 工具链（含 clang/clangd/lld 等，体积大）
```

> 注：LLVM 提供的是 clang 而非 gcc，只是 clang 的命令行与 gcc 高度兼容。

新开一个终端验证：

```powershell
clangd --version
```

winget 会把 clangd 的启动链接放在 `%LOCALAPPDATA%\Microsoft\WinGet\Links\`（该目录默认在 PATH 中）；
若提示找不到命令，检查该目录是否在系统 PATH 里。

### 2. VS Code 接入 clangd

1. 扩展市场安装 **clangd** 扩展（`llvm-vs-code-extensions.vscode-clangd`）。
2. **处理冲突**：微软 **C/C++** 扩展（`ms-vscode.cpptools`）的 IntelliSense 与 clangd 冲突。
   首次启用 clangd 扩展时右下角会弹出提示，点击 **"Disable IntelliSense"** 即可；
   若没弹或错过了，在 `settings.json` 里手动关闭（cpptools 扩展可以保留用于调试，只关它的智能提示）：

   ```json
   "C_Cpp.intelliSenseEngine": "disabled"
   ```

3. **让扩展找到 clangd**。clangd 扩展按以下顺序查找语言服务器程序：

   1. `settings.json` 中的 `clangd.path` 设置（最明确，推荐）
   2. 扩展自己下载的 clangd（找不到程序时它会弹窗询问是否从 GitHub 下载，国内网络慢，不推荐）
   3. 系统 PATH

   winget 装的 clangd 启动链接位于 `%LOCALAPPDATA%\Microsoft\WinGet\Links\`，该目录默认在 PATH 中，
   扩展通常能直接找到；若找不到，Ctrl+Shift+P 执行 "Preferences: Open User Settings (JSON)"，
   显式指定（把 `<用户名>` 换成你的 Windows 用户名）：

   ```json
   "clangd.path": "C:\\Users\\<用户名>\\AppData\\Local\\Microsoft\\WinGet\\Links\\clangd.exe"
   ```

   若装的是完整 LLVM 并加入了 PATH，也可指向 `C:\\Program Files\\LLVM\\bin\\clangd.exe`。

4. 重新加载窗口（Ctrl+Shift+P → "Reload Window"）。打开任意 `.c` 文件，底部状态栏出现
   "clangd: idle" 即接入成功，后台索引完成后补全、跳转定义即可用；异常时查看"输出 → clangd"面板的日志。

### 工作方式

- 构建系统在 `build/` 目录生成 clang 友好的 `compile_commands.json`（SDCC 构建走 custom command，
  CMake 原生导出抓取不到，由 `CMakeLists.txt` 末尾手工生成），clangd 自动发现该文件
- 数据库中的包含路径与宏定义是 SDCC 构建的"clang 视角"：引用 `C:\Program Files\SDCC\include\mcs51`
  取得 `__sfr` / `__sbit` 等关键字定义；若 SDCC 安装位置不同，相应修改 `CMakeLists.txt` 末尾 CDB 段落的路径
- `.clangd` 为工程级配置（关闭诊断噪音、开启后台索引）；`.clang-format` 统一代码风格（LLVM 基底、4 空格缩进、120 列）
- 增删源文件后重跑 `build.bat`（或重新 configure）刷新 `compile_commands.json`

## 烧录与调试

- 用 STC-ISP / AiCube-ISP 烧录 hex（冷启动：断电再上电或按复位键）
- STC-ISP 的时钟分频选项建议设为 1 分频（固件启动时也会防御性地清零 `CLKDIV`）
- 串口终端 115200 8N1
- 错过了开机日志？终端里输入 `reset` 回车即可软复位（写 IAP_CONTR，从用户程序区重启），
  重新打印完整启动日志——开发板复位按键不好用时的替代手段；`help` 列出全部命令。
  新增命令：在 `cmds_init()` 里 `cmdTree_Register()` 挂一个 handler 即可（token 必须是字面量）

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
| XRAM | ~938 / 1024 B |
| IRAM 栈 | 112 B |
| Flash | ~26 / 56 KB |

XRAM 余量约 86 B。已按最小可用配置调过的旋钮：调度器任务池
`EVTSCHEDUL_TASKS_MAX`（现为 2，命令处理并入主任务）、命令树
`CMDTREE_STATIC_MAX_NODES`、UART 缓冲 `UART_BUF_DEPTH`；TEMP 心跳删掉还能再省一点。
注意 `__pdata` 在本板不可用：SDCC 的 `movx @Ri` 分页走 P2 口锁存器，而 P2.0–3 接的是继电器。

## 许可

本项目以 [MIT 许可证](LICENSE) 发布；`uni-stc/` 目录为 [uni-STC](https://codeberg.org/20-100/uni-STC)
开源库的 vendored 副本，遵循其自身的 BSD-3-Clause。
