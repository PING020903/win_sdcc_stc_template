# c-linked-list

内核风格的侵入式双向链表。

## 针对 8051 / SDCC 的改动

原版是一个约 367 行的完整链表库，包含 `container_of`、`list_add_tail`、
`list_del`、`list_for_each_entry` 等一系列增删遍历宏（部分依赖 GNU 扩展
`__typeof__` / 语句表达式）。

本工程只用到其中的 **`ll_t` 节点类型**，原因：

- `EventScheduling` 组件已改为**静态模式**（任务节点放在固定数组池中），
  不再用链表来组织活动任务，因此所有链表操作宏都用不到。
- 保留完整库会引入 `container_of` 等 GNU 扩展，clang/SDCC 解析与 8051 上均无收益。

因此这里**只保留 `ll_t` 结构体定义**（`struct ll_head { next; prev; }`），
删除了 `container_of` 及全部链表操作宏。

> 注：`EventSchedul.h` 当前版本连 `ll_t` 节点成员也已移除（静态模式彻底不用链表）。
> 本头文件保留仅为兼容可能的外部引用；如确认无引用可整体删除。
