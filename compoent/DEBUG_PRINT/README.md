# DEBUG_PRINT

调试打印宏集合 + 位操作辅助宏。提供 `DEBUG_PRINT` / `ERROR_PRINT` / `VAR_PRINT_*` /
`MACRO_PRINT_*` 以及 `SET_BIT` / `CLEAR_BIT` / `CHECK_BIT` / `SIZE_ARRARY`。

## 针对 8051 / SDCC 的改动

原 CH58x 版本把所有打印先写进一个 **256 字节** 的 `__DBG_string` 缓冲，再经
`snprintf` + `fputs` 送到 `stdout`，并依赖 `CH58x_common.h` 与 ANSI 彩色。这套在
只有 1 KB XRAM 的 STC12 上放不下。改动如下：

- **去掉 256 字节缓冲**：打印改为走极简 `dbg_printf()`（`dbg_printf.c`），内部直接
  调用 `userUART_WriteByte` 写入 UART1 FIFO，不经过 SDCC `printf` / `putchar`。
  支持格式符仅 `%s %c %u %d %x %p %%`（全部 16 位），无宽度/精度/long/float。
- **低栈消耗**：SDCC `printf` 是重入变参函数，栈分析按 68 B 黑盒估算；`dbg_printf`
  只保留最小重入帧（fmt/va_list/循环指针/5 B 数字缓冲，约 16-18 B），且作为用户
  代码被 `tools/stack_usage.py` 精确实测。
- **新增总开关 `DBG_ENABLE`**（默认 1）：定义为 0 时所有打印宏编译为空操作。
  工程内已无 SDCC `printf` 引用（命令树当前禁用），链接器不再拉入 printf_large。
- **新增编译器兼容宏**：
  - `__HIGH_CODE` —— CH58x 用于把热点代码放进专用段，8051 无意义，定义为空。
  - `__INTERRUPT` —— 同理，定义为空。
  - `REENTRANT` —— SDCC 下为 `__reentrant`，其它编译器为空。8051 规定**经函数指针
    调用的函数必须可重入**，各组件的回调指针类型与本宏配合使用。
- **`VAR_PRINT_LLU` / `VAR_PRINT_LL` / `VAR_PRINT_FLOAT` / `VAR_PRINT_ARR_HEX`
  恒为空操作**：避免拉入 64 位整数与浮点的 printf 支持代码（体积大、速度慢）。
- **去掉 CH58x 依赖与 ANSI 彩色**，头文件仅依赖 `<stdint.h>` / `<stddef.h>`，
  启用打印时再包含 `dbg_printf.h`。

## 保留未动

`SET_BIT` / `CLEAR_BIT` / `CHECK_BIT` / `SIZE_ARRARY` 为纯宏，门锁状态机
（`doorLock.h`）依赖它们，原样保留。

## 用法

```c
DEBUG_PRINT("door %u opened", idx);   // 经 UART1 输出
VAR_PRINT_HEX(flags);
```

关闭全部调试：编译时加 `-DDBG_ENABLE=0`。
