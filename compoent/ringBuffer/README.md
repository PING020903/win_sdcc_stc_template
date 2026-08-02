# ringBuffer

定长环形缓冲区（FIFO），支持单元素与批量的 push / pop / peek，可选满时覆盖。
缓冲区内存由调用者提供（`RINGBUFCRTL_INIT`），组件本身不动态分配。

## 针对 8051 / SDCC 的改动

缓冲 logic 本身**未改动**，仅做编译适配：

- **去掉 `#pragma once`**：SDCC 不支持该 pragma，保留 `#ifndef` 头保护即可。
- **去掉 `RING_DEBUG`**：原版在 `ringBuffer.h` 里 `#define RING_DEBUG`，使
  `ringBuffer.c` 在 `ringBuf_init` 等函数里经 `DBG_macro` 打印调试信息。
  8051 上默认关闭这些打印，故移除该开关及相关打印代码。
- **`ringBuffer.c` 增加 `#include "DBG_macro.h"`**：源文件用到 `__HIGH_CODE`
  宏（CH58x 段属性，8051 下为空），该宏由 `DBG_macro.h` 提供。原版是在
  `RING_DEBUG` 分支里间接包含，移除 `RING_DEBUG` 后需显式包含。

## 依赖

- `<stdint.h>` / `<stdbool.h>` / `<stddef.h>` / `<string.h>`
- `DBG_macro.h`（仅为 `__HIGH_CODE`）

## 被谁使用

- `EventScheduling`：事件队列（`EventSchedul_TaskQueue` 环形队列）
- `src/userTasks/userButton.c`：按键原始事件队列
