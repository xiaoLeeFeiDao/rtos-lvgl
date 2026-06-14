# 嵌入式 RTOS + LVGL 从零学习指南

> 基于本项目 (rtos-lvgl) 的实战知识点，按学习顺序编排

---

## 第一课：ARM Cortex-M3 裸机启动

### 1.1 上电后 CPU 做了什么？

```
上电 → 读 0x08000000 的第一个字 → 设为 SP（栈指针）
     → 读 0x08000004 的第二个字 → 设为 PC（程序计数器，即 Reset_Handler 地址）
     → 执行 Reset_Handler
```

**关键文件：** `platform/stm32f103/startup.s`

```asm
.section .vectors, "a"
_vectors:
    .word _stack_top           /* SP 初始值 */
    .word Reset_Handler        /* 复位向量 */
    .word Default_Handler      /* NMI */
    ...（其他中断向量）
```

**你学到的：** 
- 向量表是上电后 CPU 读的第一段数据
- SP 决定了栈在哪（内部 SRAM 顶部 0x20010000）
- PC 决定了从哪开始执行代码

### 1.2 链接脚本决定内存布局

**关键文件：** `platform/stm32f103/STM32F103ZETX_FLASH.ld`

```
内存分布：
┌─────────────────┐ 0x08000000  512KB Flash
│ .vectors        │  ← 向量表
│ .text           │  ← 程序代码（只读）
│ .rodata         │  ← 常量数据（字体、图片）
│ .data (LMA)     │  ← .data 的初始值存这里
├─────────────────┤ 0x20000000  64KB SRAM
│ .data (VMA)     │  ← 已初始化的全局变量
│ .bss            │  ← 未初始化的全局变量（启动时清零）
│ 堆 / 栈         │  ← 动态分配 + 函数调用栈
└─────────────────┘
```

**关键概念：**
- **LMA (Load Memory Address):** 数据在 Flash 中的存储位置
- **VMA (Virtual Memory Address):** 数据运行时的地址
- **`.data` 搬运：** 启动代码把 `.data` 从 Flash 复制到 SRAM
- **`.bss` 清零：** 启动代码把 `.bss` 全部清零

### 1.3 启动代码详解

```asm
Reset_Handler:
    /* ① 把 .data 从 FLASH 复制到 SRAM */
    ldr r0, =_etext        // r0 = Flash 中数据存储地址
    ldr r1, =_sdata        // r1 = SRAM 中数据目标地址
    ldr r2, =_edata        // r2 = SRAM 中数据结束地址
1:  cmp r1, r2
    bge 2f
    ldr r3, [r0], #4       // 从 Flash 读 4 字节
    str r3, [r1], #4       // 写到 SRAM
    b 1b

    /* ② 把 .bss 清零 */
2:  ldr r1, =_sbss
    ldr r2, =_ebss
    mov r3, #0
3:  cmp r1, r2
    bge 4f
    str r3, [r1], #4
    b 3b

    /* ③ 跳转到 C 语言 main() */
4:  bl main
```

**你学到的：**
- 为什么全局变量在 main() 之前就有初始值
- 为什么未初始化全局变量是 0
- C 语言的运行环境是怎么建立起来的

---

## 第二课：时钟树 — CPU 怎么跑到 64MHz

### 2.1 STM32F103 时钟系统

```
HSI 8MHz (内部RC)
  └→ /2 = 4MHz → PLL (×16) → 64MHz
                                 ├→ AHB (CPU, DMA, FSMC) = 64MHz
                                 ├→ APB2 (GPIO, USART1, SPI1) = 64MHz
                                 └→ APB1 (SPI2, USART2) = 32MHz (/2)
```

**关键代码：** `platform/stm32f103/bsp.c:bsp_clock_init()`

```c
// ① 等内部 8MHz 振荡器稳定
RCC->CR |= (1 << 0);           // HSION
while (!(RCC->CR & (1 << 1))); // 等 HSIRDY

// ② 设 Flash 等待周期（高速时必须）
FLASH->ACR = FLASH_ACR_LATENCY_2;  // 2 个等待周期

// ③ 配置 PLL：HSI/2 × 16 = 64MHz
RCC->CFGR = (15 << 18)          // PLLMUL = ×16
          | RCC_CFGR_PPRE1_2;   // APB1 = /2

// ④ 启动 PLL
RCC->CR |= RCC_CR_PLLON;
while (!(RCC->CR & RCC_CR_PLLRDY));

// ⑤ 切换到 PLL
RCC->CFGR = (RCC->CFGR & ~0x3) | RCC_CFGR_SW_PLL;
while ((RCC->CFGR & 0xC) != (2<<2));  // 等切换完成
```

**关键问题：** 为什么需要 Flash 等待周期？
CPU 跑 64MHz 时，Flash 读取速度跟不上。不加等待周期，CPU 会读到错误指令 → HardFault。

---

## 第三课：GPIO — 怎么控制引脚

### 3.1 寄存器控制

每个 GPIO 端口有 7 个寄存器：

| 寄存器 | 作用 | 示例 |
|--------|------|------|
| CRL/CRH | 配置模式（输入/输出/复用） | `GPIOB->CRL = 0x...` |
| IDR | 读输入电平 | `if (GPIOE->IDR & (1<<4))` |
| ODR | 写输出电平 | `GPIOB->ODR |= (1<<0)` |
| BSRR | 原子位置位 | `GPIOB->BSRR = (1<<5)` |
| BRR | 原子位清零 | `GPIOB->BRR = (1<<5)` |

### 3.2 CRL/CRH 配置详解

每个引脚占 4 位（CNF[1:0] + MODE[1:0]）：

```
MODE[1:0]:  00=输入  01=10MHz  10=2MHz  11=50MHz
CNF[1:0]:   00=通用推挽  01=通用开漏
            10=复用推挽  11=复用开漏
```

```c
// 示例：PB5 设为 50MHz 通用推挽输出
// PB5 在 CRL 的 bits[23:20]
GPIOB->CRL = (GPIOB->CRL & 0xFF0FFFFF) | 0x00300000;
//           清除 bits[23:20]   MODE=11 CNF=00
```

---

## 第四课：FSMC — 怎么驱动 LCD 和外部 SRAM

### 4.1 什么是 FSMC？

FSMC (Flexible Static Memory Controller) 把外部设备（LCD、SRAM）映射到 CPU 的地址空间，让你像访问内存一样访问它们。

```
STM32 地址空间：
0x60000000-0x6FFFFFFF → FSMC Bank1 (可接 LCD/SRAM/NOR Flash)
  Region1 (NE1): 0x60000000
  Region2 (NE2): 0x64000000
  Region3 (NE3): 0x68000000  ← 外部 SRAM (IS62WV51216)
  Region4 (NE4): 0x6C000000  ← LCD (ILI9488)
```

### 4.2 8080 并口 LCD 原理

```
FSMC 写操作：
  ① CPU 写 0x6C000000（命令）→ FSMC 自动：NE4 拉低, A10=0, D[15:0]=命令字, NWE 拉低再拉高
  ② CPU 写 0x6C000800（数据）→ FSMC 自动：NE4 拉低, A10=1, D[15:0]=数据字, NWE 拉低再拉高
```

**A10 (RS 引脚):** 
- RS=0 → 写入的是命令
- RS=1 → 写入的是数据

`0x6C000800` 中的 `800` = `1<<11`（因为 16 位总线，A10 对应 HADDR[11]）

### 4.3 FSMC 时序配置

```
一次写操作的时序：
  ┌──── ADDSET ────┬── DATAST ────┐
  │  地址建立时间   │  数据建立时间  │
NE4:  ‾‾‾‾‾‾\______________________/‾‾‾‾‾‾
NWE:  ‾‾‾‾‾‾‾‾‾‾\______________/‾‾‾‾‾‾‾‾‾
D[15:0]: -------<  有效数据  >----------
```

```c
// FSMC BTR 寄存器配置时序
FSMC_BTR4 = FSMC_BTR_ADDSET_3   // ADDSET = 3 个 HCLK
          | (5 << 8);            // DATAST = 5 个 HCLK
```

**你学到的：** 为什么 FSMC 写错了时序会花屏/撕裂

### 4.4 为什么外部 SRAM 32-bit 写失败？

```
CPU 执行: STR R0, [0x68000000]  (32-bit 写)
FSMC 拆成: STRH R0_lo, [0x68000000]  (16-bit 写低半)
          STRH R0_hi, [0x68000002]  (16-bit 写高半)
```

**失败原因：** NBL0/NBL1（字节选择信号）时序问题，导致 32-bit 拆分后的第二个 16-bit 写数据丢失。

---

## 第五课：LVGL 移植核心

### 5.1 LVGL 需要什么？

```
┌─────────────────────────────────────┐
│            LVGL 核心                 │
│  lv_init() → lv_timer_handler()     │
│  渲染引擎 → 写入绘制缓冲区           │
├─────────────────────────────────────┤
│  你需要提供的：                      │
│  ① lvgl_port_flush() — 把缓冲写到 LCD │
│  ② lv_tick_inc()     — 心跳时钟     │
│  ③ lv_indev_read()   — 输入设备     │
│  ④ 绘制缓冲区         — 内存         │
└─────────────────────────────────────┘
```

### 5.2 PARTIAL 渲染模式原理

```
屏幕 480 行 ÷ 缓冲区 10 行 = 48 次刷新

第 1 次: 渲染行 0-9   → flush → 写 LCD
第 2 次: 渲染行 10-19 → flush → 写 LCD
...
第 48 次: 渲染行 470-479 → flush → 写 LCD
```

**缓冲区越大 → 刷新次数越少 → 画面越快**

### 5.3 内存分配策略（本项目的关键问题）

```
64KB 内部 SRAM 分配：
  LVGL 内存池: 48KB  ← 控件、样式、字体数据
  绘制缓冲:    6KB  ← 320×10 像素 × 2字节
  栈 + 全局变量: ~10KB
  ─────────────────
  合计:        64KB  (92% 使用率)
```

**为什么不能把 LVGL 池放外部 SRAM？**

LVGL 池使用 TLSF 算法，初始化时用 32-bit 写来建立链表。32-bit 写到外部 SRAM 失败 → LVGL 崩溃。

**为什么绘制缓冲放外部也失败？**

LVGL 渲染引擎写入缓冲时可能用 memcpy/memset（32-bit 优化），导致同样问题。

---

## 第六课：RTOS 抽象层设计

### 6.1 统一接口

```c
// rtos_task.h — 所有平台统一接口
rtos_handle_t rtos_task_create(name, priority, stack, func, param);
rtos_status_t rtos_task_delete(handle);

// rtos_sync.h
rtos_handle_t rtos_sem_create(count);
rtos_status_t rtos_sem_take(handle, timeout);
rtos_status_t rtos_sem_give(handle);
```

### 6.2 POSIX 实现（macOS 模拟）

```c
// platform/posix/rtos_task.c
rtos_handle_t rtos_task_create(...) {
    pthread_t thread;
    pthread_create(&thread, NULL, func, param);
    return (rtos_handle_t)thread;
}
```

### 6.3 STM32 裸机实现

```c
// platform/stm32f103/rtos_task.c
rtos_handle_t rtos_task_create(...) {
    return RTOS_INVALID_HANDLE;  // 裸机：不支持多任务
}
```

### 6.4 切换到 FreeRTOS 只需改一个 CMake 变量

```bash
cmake -DRTOS_TARGET=freertos ..
# 自动链接 platform/freertos/rtos_task.c（里面用 xTaskCreate）
```

**你学到的：** 抽象层的价值 — 上层代码不改，底层随便换。

---

## 第七课：LVGL Widgets Demo 架构

### 7.1 控件树

```
lv_scr_act() (屏幕)
  └── lv_tabview (标签页)
        ├── Tab "Profile" (表单)
        │     ├── lv_image (头像)
        │     ├── lv_textarea (输入框)
        │     ├── lv_dropdown (下拉)
        │     ├── lv_slider (滑块)
        │     └── lv_switch (开关)
        ├── Tab "Analytics" (数据)
        │     ├── lv_chart (图表)
        │     ├── lv_scale (刻度)
        │     └── lv_arc (圆弧)
        └── Tab "Shop" (电商)
              ├── lv_chart (柱状图)
              └── lv_checkbox (复选框)
```

### 7.2 Grid 布局

```c
// 定义 2 列 3 行的网格
static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
static int32_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

// 控件放在第 0 列、第 1 行，跨 1 列 2 行
lv_obj_set_grid_cell(widget, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 2);
```

| 宏 | 含义 |
|---|------|
| LV_GRID_FR(1) | 占 1 份弹性空间 |
| LV_GRID_CONTENT | 按内容大小 |
| LV_GRID_TEMPLATE_LAST | 数组结束标记 |

### 7.3 事件系统

```c
// 按钮点击
lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

static void btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // 处理点击
    }
}
```

---

## 下一步学习建议

1. **先用 CubeMX + HAL 重做一遍** — 把裸寄存器版本对照 HAL 版本理解
2. **加 FreeRTOS** — 体验多任务调度
3. **修复外部 SRAM** — 用逻辑分析仪排查 32-bit 写时序
4. **修触摸** — 用示波器看 SPI 波形

---

## 文件索引

| 想学什么 | 看哪个文件 |
|---------|-----------|
| CPU 怎么启动 | `platform/stm32f103/startup.s` |
| 内存怎么布局 | `platform/stm32f103/STM32F103ZETX_FLASH.ld` |
| 时钟怎么配置 | `platform/stm32f103/bsp.c:bsp_clock_init()` |
| LCD 怎么驱动 | `platform/stm32f103/lcd.c` |
| FSMC 怎么工作 | `platform/stm32f103/stm32f1xx.h` (FSMC 寄存器) |
| LVGL 怎么移植 | `platform/stm32f103/lvgl_port.c` |
| RTOS 抽象层 | `platform/include/rtos_*.h` |
| LVGL Demo 怎么写的 | `build/_deps/lvgl-src/demos/widgets/lv_demo_widgets.c` |
