# ringBuffer

定长环形缓冲区（FIFO），支持单元素与批量的 push / pop / peek，可选满时覆盖。
缓冲区内存由调用者提供（`RINGBUFCRTL_INIT`），组件本身不动态分配。

## 针对 8051 / SDCC 的改动

以下为历史编译适配：

- **去掉 `#pragma once`**：SDCC 不支持该 pragma，保留 `#ifndef` 头保护即可。
- **去掉 `RING_DEBUG`**：原版在 `ringBuffer.h` 里 `#define RING_DEBUG`，使
  `ringBuffer.c` 在 `ringBuf_init` 等函数里经 `DBG_macro` 打印调试信息。
  8051 上默认关闭这些打印，故移除该开关及相关打印代码。
- **`ringBuffer.c` 增加 `#include "DBG_macro.h"`**：源文件用到 `__HIGH_CODE`
  宏（CH58x 段属性，8051 下为空），该宏由 `DBG_macro.h` 提供。原版是在
  `RING_DEBUG` 分支里间接包含，移除 `RING_DEBUG` 后需显式包含。

### 低栈改造（当前改动）

`ringBuf_count / ringBuf_push / ringBuf_pop / ringBuf_peek` 原先声明为
`__reentrant`（ISR 与主流程共享）。mcs51 重入函数把**所有参数和局部变量压进
运行时栈**（`ringBuf_push` 帧曾达 22 B+），是栈最坏链的主要贡献者。改为：

- **去重入 + `__critical` 临界区**：这 4 个入口改为非重入 wrapper，真实逻辑在
  `*_core` 静态函数内，由 `__critical { }` 包裹。`__critical` 保存/恢复 `EA`，
  主流程里 1→0→1、ISR 里（EA 本为 0）0→0 不嵌套。因 ISR 无法在临界区内抢占，
  非重入函数用的 PARM / overlay 区不会被踩坏，16 位索引读写也变原子。
- **`_get_item_ptr` 去掉 `ringBuf_ptr_t`（ptrdiff_t）中间量**：改用
  `unsigned char *` + `unsigned int` 偏移，消除了内联展开里的 `__mullong`
  与多个 4 字节栈槽（帧 29 B → 22 B）。
- **`idx % depth` 改为比较-减法**：索引恒 `< 2*depth`（`RINGBUF_UPDATE_IDX` /
  peek 折返保证），故 `idx % depth == (idx >= depth ? idx - depth : idx)`，
  去掉热路径上的 `__moduint` 库调用（4 处）。
- core 之间互调 `ringBuf_count_core`，避免嵌套临界区。

API 签名不变，调用方无需改动；多元素接口（`*_multi`）仍逐个调用上述 wrapper，
每个元素一次临界区。

代价：非重入后各函数局部变量转入 IRAM 固定区（data/overlay，约 +30 B），
因此 `CMakeLists.txt` 的 `STACK_SIZE` 相应从 160 下调到 128；栈分析实测最坏
栈从 138 B 降至 84 B（余量 22 → 44 B）。

## 依赖

- `<stdint.h>` / `<stdbool.h>` / `<stddef.h>` / `<string.h>`
- `DBG_macro.h`（仅为 `__HIGH_CODE`）

## 被谁使用

- `EventScheduling`：事件队列（`EventSchedul_TaskQueue` 环形队列）
- `src/userTasks/userButton.c`：按键原始事件队列
- `src/userUART_init.c`：UART1 中断驱动收发 FIFO（`uart1_isr` 与主流程并发，
  依赖本组件的 `__critical` 临界区保证共享访问安全）
