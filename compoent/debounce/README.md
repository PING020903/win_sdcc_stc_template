# debounce

通用按键/输入去抖组件（连续稳定采样法）。无外部依赖。

## 原理

机械开关（干簧管、按键）切换时会抖动 5~20ms，电平在高/低间多次跳变。本组件要求
原始电平**连续 N 次**（`threshold`）稳定在与当前稳定态不同的值，才接受这次状态翻转；
抖动期间电平反复跳变，计数 `cnt` 不断被清零，永远攒不够阈值，因此不会误触发。

```
每次采样：
  raw != stable :  cnt++ ;  cnt >= threshold 时 stable = raw, 返回 1（去抖边沿）
  raw == stable :  cnt = 0
```

## API

```c
typedef struct { uint8_t cnt : 7; uint8_t stable : 1; } debounce_t;  /* 仅 1 字节 */

void    debounce_init(debounce_t *db, uint8_t initialLevel);
uint8_t debounce_sample(debounce_t *db, uint8_t rawLevel);  /* 1=稳定态翻转 */
uint8_t debounce_state(const debounce_t *db);               /* 当前稳定电平 */
```

- 状态结构体压成 **1 字节**（`cnt:7` + `stable:1`）。去抖阈值由编译期宏
  `DEBOUNCE_SAMPLES`（默认 3）统一控制，不占实例空间——RAM 紧张时的取舍
  （所有输入用同一阈值；如需每实例不同阈值可改回结构体字段）。
- `debounce_init`：`initialLevel` 为采样前的假定电平（如空闲高电平=1）。
- `debounce_sample`：每个采样周期喂一次原始电平，返回 1 表示发生了**去抖后的边沿**。
- `debounce_state`：读取当前稳定电平（0/1）。

## 使用要点

- **采样周期由调用者掌握**（本工程为 10ms），去抖时间 = 采样周期 × `DEBOUNCE_SAMPLES`
  （默认 10ms × 3 = 30ms）。
- **配合边沿触发**：只在 `debounce_sample` 返回 1（稳定态翻转）时发事件，而非检测
  电平本身——这同时避免了"抖动重复"和"电平保持重复"两类重复事件。
- 上升/下降沿由翻转后的 `debounce_state` 判断：变 1 = 上升沿（如松开），变 0 = 下降沿
  （如按下）。

## 被谁使用

- `src/board_bus.c`：门磁×4、门按键×4、配置按键×4 共 12 路输入去抖。
