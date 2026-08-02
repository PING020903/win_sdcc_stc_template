# EventScheduling

事件调度器：任务注册、按事件区间投递事件、环形队列缓存、主循环分发。
支持一个休眠回调（`sleepMethod`）在轮询间隙让出 CPU。

## 针对 8051 / SDCC 的改动

原版同时支持**动态**（经注入的 `malloc`/`free` 分配任务节点与上下文，活动任务用
`c-linked-list` 链表组织）和**静态**（任务节点池）两种模式。8051 上做了如下改造：

### 1. 仅保留静态模式，去除动态分配

- 删除动态模式及其中断链表相关代码；`EventSchedul_TaskNode` 里**移除 `ll_t node`
  成员**（静态模式按数组下标扫描空闲槽位，不需要链表），每个节点省下若干字节。
- **上下文 `EventSchedul_Context` 改为单一静态实例**（`static _evtCtx`），
  `EventSchedul_Create()` 返回它的地址并**忽略 allocator 参数**；`EventSchedul_Destroy()`
  变为空操作。彻底去掉 `malloc`/`free`，避免在 1 KB XRAM 上做动态内存管理。
- 任务上限由 `EVTSCHEDUL_TASKS_MAX`（默认 8）固定，事件队列深度
  `EVTSCHEDUL_TASKS_QUEUE_MAX`（= 任务数 × 2）。

### 2. 函数指针回调改为可重入（8051 硬性要求）

8051 的非可重入函数使用固定 overlay 区，**经指针间接调用时 SDCC 要求目标函数
可重入**（否则报 error 92）。因此：

- 任务回调指针类型改为
  `typedef void (*EventSchedul_pTaskFunc)(EventSchedul_EventId, void*) REENTRANT;`
- `sleepMethod` 成员改为 `void (*sleepMethod)(void) REENTRANT;`
- 所有实际回调（任务函数、休眠函数）须以 `REENTRANT` 声明，例如
  `static void doorLockTask(EventSchedul_EventId, void*) REENTRANT { ... }`。

> **SDCC 语法注意**：`__reentrant` 必须放在**参数列表之后**
> （`void (*fn)(int) __reentrant`），放在 `*` 前会语法报错；且 SDCC **无法**把
> 普通指针强转成 reentrant 函数指针。故 `EventSchedul_RegSleepMethod()` 的形参
> 直接声明为 reentrant 函数指针 `void (*pFunc)(void) REENTRANT`，调用方传入
> 可重入函数即可，不再做 `void*` 中转强转。

`REENTRANT` 宏由 `DEBUG_PRINT/DBG_macro.h` 提供（SDCC 下展开为 `__reentrant`）。

## 被谁使用

- `src/tickBroadcast.c`：把周期 tick 广播给已注册任务。
- `src/userTasks/userTask_doorLock.c`：注册门锁任务并消费事件。
- `src/Main.c`：创建调度器、注册休眠回调、进入 `EventSchedul_MainLoop`。
