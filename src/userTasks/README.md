# userTasks — 门锁逻辑

门锁状态机、按键处理与事件任务。从 CH58x 工程移植到 STC12（8051 / SDCC），
逻辑基本保留，针对 8051 的紧张资源做了结构体瘦身。

## 模块构成

| 文件 | 职责 |
|---|---|
| `doorLock.h/.c` | 门锁核心：管理器 / 门上下文 / 注册 / 检测 / 锁控制 / 请求开门 |
| `userButton.h/.c` | 按键消抖 + 环形事件队列（缓冲由调用者提供，统一 8 通道） |
| `btnEventMap.h/.c` | 按键事件映射表：channel+trigger → 业务事件（改表即改按键行为） |
| `userTask_doorLock.h/.c` | 门锁事件任务：把硬件事件接入事件调度器，含配置状态机 |
| `userTask_cmds.h` | 桩：仅提供共享调度器句柄 `evtSchedul_ctx` 的 extern 声明 |

## 数据结构（8051 紧凑化）

```c
doorLock_io_t       { detect; doorButton; lock; }   // 3 个 busManage_io_t（各 1 字节）
doorLock_time_t     { uint8_t lockDelaySec; }       // 延时单位=秒，uint8_t 足够
doorLock_ops_t      { detect; lock; doorInit; }     // 方法表（回调集合）
doorLock_hwConfig_t { io; time; const doorLock_ops_t *ops; }
doorLock_context    { hw; flags:4; index:4; }       // flags/index 各 4 位 → 共 1 字节
doorLock_manager_t  { doors[4]; doorCnt:4; openDoor:4(signed); }  // 各 4 位 → 共 1 字节
```

关键优化点：

- **方法表（vtable）**：4 扇门共用同一组回调，`board_bus.c` 定义一个
  `static const doorLock_ops_t door_ops`（进 Flash），每门 `hwConfig` 只存 1 个 ops 指针。
  核心逻辑经 `ctx->hw.ops->detect/lock/doorInit` 调用。
- **延时改秒**：精度到秒即可，`lockDelaySec` 用 `uint8_t`（原 `unsigned int` 毫秒）。
- **位域打包**：`flags`（WIRE/LOCK/INIT 仅 3 位）+ `index`（门编号 0-3）合 1 字节；
  `doorCnt`（0-4）+ `openDoor`（-1 无 / 0-3，有符号 4 位可至 7）合 1 字节。
  头文件有编译期保险 `DOORLOCK_DOOR_MAX > 7` 报错。
- **IO 1 字节**：`busManage_io_t` 位域 `{pin:4, port:4}`（详见 BUS_IO_management）。

## 硬件 IO（每门 3 个，占位引脚见 board_bus.c）

| 信号 | 方向 | 作用 |
|---|---|---|
| 门磁 `detect` | 输入 | 读门状态（更新 wire 标志，不直接开门） |
| 门按键 `doorButton` | 输入 | 按下 → 触发开该门 |
| 继电器 `lock` | 输出 | 1=锁 / 0=开 |

另有 **4 个全局配置按键**（通道 `BTN_CH_CFG_INC/DEC/SELECT/ENTER`）配置门锁延时。

**输入电气与检测方式**：所有输入（门磁 / 门按键 / 配置按键）均为**有源低电平**——
外部上拉使引脚空闲为高，门就位（0Ω）或按键按下时拉低；IO 配置为**浮空输入**
（`GPIO_HIGH_IMPEDANCE_MODE`）。检测采用**轮询**而非中断：STC12C5A60S2 只有
INT0(P3.2)/INT1(P3.3) 两个通用外部中断，**无逐脚 GPIO 边沿中断**（那是 STC8/STC15
的能力），无法给 8~12 个输入各配中断，故由 `board_bus_poll()` 每 ms 扫描做 software
下降沿检测。门锁均为人手级慢事件，轮询响应足够。

## 事件与任务流程

`userTask_doorLock` 注册到事件调度器，消费以下事件：

- `EVT_DOORLOCK_TICK`（1ms）：`button_tick` 每 ms 跑；内部 1000 分频出**秒节拍**，
  每秒递减开门超时计数 `lockTimeoutCnt` 与配置空闲计数 `configIdleCnt`。
- `EVT_DOORLOCK_DETECT`：门磁变化 → 仅更新门状态标志。
- `EVT_DOOR_OPEN_0..3`：门按键释放沿（经 `btnEventMap` 映射）→ `doorLock_requestOpen`
  + 启动 lockDelay 超时。
- `EVT_CFG_INC/DEC/SELECT/ENTER`：配置按键（经 `btnEventMap` 映射）→ 配置态动作。
- `EVT_DOORLOCK_LOCK_TIMEOUT`：超时 → 重新上锁。
- `EVT_DOORLOCK_BTN_SCAN`：按键事件出队 → 查 `btnEventMap` → 投递映射事件。
- `EVT_DOORLOCK_DISPLAY_REFRESH`：显示刷新（数码管驱动待接入）。

### 按键事件映射（`btnEventMap`）

门按键（通道 0-3）与配置键（通道 4-7）统一进 `userButton` 8 通道框架；`board_bus_poll`
只做消抖 + `button_push`。任务在 `EVT_DOORLOCK_BTN_SCAN` 里 `button_poll` 取出
`(channel, press/release)`，查 `btnEventMap` 表得到业务事件再投递给本任务：

| 通道 | 触发 | 事件 |
|---|---|---|
| `BTN_CH_DOOR_n` (0-3) | release | `EVT_DOOR_OPEN_n` |
| `BTN_CH_CFG_INC` (4) | press | `EVT_CFG_INC` |
| `BTN_CH_CFG_DEC` (5) | press | `EVT_CFG_DEC` |
| `BTN_CH_CFG_SELECT` (6) | press | `EVT_CFG_SELECT` |
| `BTN_CH_CFG_ENTER` (7) | press | `EVT_CFG_ENTER` |

改某键的触发行为只需编辑 `btnEventMap.c` 的 `btnEvtMap[]` 表。

### 配置状态机（4 个全局按键）

- **普通态**：`SELECT` 循环切换门号；`ENTER` 进入配置态。
- **配置态**：`INC`/`DEC` 以 `CFG_DELAY_STEP_SEC`（1s）调该门延时；任意键重置空闲计时；
  `SELECT`/`ENTER` 确认退出；`CONFIG_IDLE_TIMEOUT_SEC`（5s）无操作自动确认。

可调宏（`userTask_doorLock.c` 顶部）：`CFG_DELAY_STEP_SEC`、`CONFIG_IDLE_TIMEOUT_SEC`。
