# BUS_IO_management

总线 / IO 资源管理框架（**精简版**）。

保留原 CH58x 总线管理器的**声明式骨架**——用一个设备模型（`busManage_bus_model_t`）
把若干资源描述（`busManage_resource_desc_t`）打包，每个资源带一个 init 回调；
`board_bus.c` 通过统一的 `manager_init` + 循环 `manager_allocate` **逐个初始化每扇门
所需的 IO**。但砍掉了 8051 上既占 RAM 又用不上的部分。

## 针对 8051 / SDCC 的改动

### 保留（框架骨架）

- `busManage_bus_model_t`：设备模型（device_name / resources / resourcesCnt …），全部 `const`→Flash。
- `busManage_resource_desc_t`：`type` + `union{uart, custom}` + **`init` 回调**。
- `manager_init` / `manager_allocate` / `manager_release` / `dump_status`（`BUSMANAGE_DEBUG` 下）API。
- `board_bus.c` 里 `board_busModel` + 资源表 + 逐个 allocate 触发 init 回调的初始化流程。

### 删除（约 120–140 B XRAM 的运行期状态）

原版 `g_mgr` 含各总线实例跟踪数组（SPI/UART/I2C/USB/Custom）、SPI CS 引脚表、
波特率/版本数组等，用于**运行时总线冲突仲裁**。本工程是单块固定板、资源静态声明且
互不冲突、无热插拔，仲裁永远不会触发，故全部删除。精简后 `g_mgr` 只剩
**一个 model 指针 + initialized 标志（约 5 B）**。

- `manager_allocate` 现仅校验资源属于当前模型（指针区间检查）后调用其 `init` 回调。
- `manager_release` 无状态可释放，为返回 OK 的桩。
- 删除未用到的总线类型（SPI / I2C / CAN / USB），仅留 `UART` / `custom`。

### 8051 适配细节

- **`busManage_io_t`** 压成 **1 字节位域** `{ uint8_t pin : 4; uint8_t port : 4; }`
  （原版是 `port` 位域 + `uint32_t pinMask`）。SDCC 位域从低位排起，故 `pin` 占低 4 位
  （0-7）、`port` 占高 4 位（`BUSIO_PORT_P0..P3`，与 uni-STC 的 `GPIO_PORTx` 一致）。
  保留 `.port`/`.pin` 成员访问；`board_bus.c` 里 `io_to_cfg()` 把它转成 uni-STC
  `GpioConfig` 再读写。io 表初始化须用指定初始化 `{.port = ..., .pin = ...}`
  （成员序是 pin 在前，位置初始化会错位）。
- **`init` 回调经指针调用 → 必须可重入**：`busManage_initFn` 用 `REENTRANT` 修饰
  （`__reentrant` 放在参数列表之后，SDCC 语法），各 init 回调（`door_busInit` 等） likewise。
- **`baudrate` 用 `unsigned long`**：8051 的 `unsigned int` 只有 16 位，装不下 115200。
- **资源表用带索引的内联初始化**：SDCC 不允许用结构体变量拷贝初始化数组（error 69），
  故 `board_all_resources[]` 每项直接内联描述，门资源占连续下标以便门锁注册直接索引。

## 被谁使用

- `src/board_bus.c`：定义 io 表（唯一引脚来源）、资源表、设备模型，`board_bus_init()`
  跑框架完成逐门 IO 初始化；`board_registerDoors()` 把门注册进门锁逻辑。
