# STM32F103ZET6 LVGL v9 移植设计

> 日期: 2026-06-13 | 状态: 已批准

## 目标

将 RTOS LVGL Demo 工程从 macOS/SDL2 模拟器移植到正点原子精英版 STM32F103ZET6 开发板裸机运行。

## 硬件规格

| 项目 | 规格 |
|------|------|
| MCU | STM32F103ZET6, Cortex-M3, 72MHz |
| 内部 SRAM | 64KB @ 0x20000000 |
| 外部 SRAM | IS62WV51216, 1MB @ 0x68000000 (FSMC Bank1 NE3) |
| Flash | 512KB @ 0x08000000 |
| LCD | 3.5" TFT, 320×480, FSMC 8080 16-bit 并口 (NE4 + A10 RS) |
| LCD IC | NT35310 / ILI9488 (自动检测) |
| 触摸 | XPT2046, SPI2 |
| 调试 | USART1, PA9/PA10, 115200-8-N-1 |

## 架构

```
apps/stm32/main.c              ← 主循环: lv_tick_inc+lv_timer_handler+lvgl_port_present
platform/stm32f103/lvgl_port.c ← LVGL flush(FSMC→LCD GRAM) + indev(SPI触摸轮询)
platform/stm32f103/lcd.c       ← ILI9341/ILI9488 驱动 (硬复位+初始化序列+画点)
platform/stm32f103/touch.c     ← XPT2046 SPI 读触摸坐标
platform/stm32f103/usart.c     ← USART1 printf 重定向
platform/stm32f103/bsp.c       ← 时钟 72MHz + FSMC + SPI + SysTick + 外部SRAM
platform/stm32f103/startup.s   ← 向量表 + SystemInit
platform/stm32f103/STM32F103ZETX_FLASH.ld ← 内存映射
platform/stm32f103/stm32f1xx.h ← 外设寄存器定义
platform/stm32f103/rtos_*.c    ← RTOS 抽象层裸机空实现
```

## 关键决策

| 决策 | 方案 | 理由 |
|------|------|------|
| 运行时 | 裸机 Super Loop | 内存紧，调试简单，后续可切 FreeRTOS |
| LV_COLOR_DEPTH | 16 (RGB565) | LCD 原生格式，省内存省 CPU |
| 渲染 | PARTIAL 双缓冲 1/10 屏 | 每缓冲 320×48×2=30KB |
| LVGL 堆 | 外部 SRAM, LV_MEM_SIZE=256KB | 内部 SRAM 留给栈和关键数据 |
| BSP 编程 | 裸寄存器 | 4 个外设，300 行搞定，避免 HAL 依赖 |
| 工具链 | arm-none-eabi-gcc + CMake | 已安装，直接可用 |
| 烧录 | st-flash (ST-Link V2) | 已检测到，命令行一条命令 |

## 内存布局

```
内部 SRAM (64KB @ 0x20000000)
├── 系统栈 (4KB)
├── .data + .bss (全局变量)
└── 堆 (内部 malloc, ~40KB 备用)

外部 SRAM (1MB @ 0x68000000)
├── LVGL 内存池 (lv_malloc, LV_MEM_SIZE=256KB)
├── 绘制缓冲 buf1 (1/10屏 ≈ 30KB)
├── 绘制缓冲 buf2 (1/10屏 ≈ 30KB)
└── 余量 (~684KB)
```

## LVGL 配置 (lv_conf.h 变更)

仅当 RTOS_TARGET=stm32f103 时生效：

| 配置项 | 值 |
|--------|-----|
| LV_COLOR_DEPTH | 16 |
| LV_MEM_SIZE | 256 * 1024 |
| LV_USE_OS | LV_OS_NONE |
| LV_DPI_DEF | 130 |
| LV_DRAW_BUF_STRIDE_ALIGN | 1 |
| LV_USE_DEMO_WIDGETS | 1 |
| LV_USE_LOG | 0 |
| LV_FONT_MONTSERRAT_14 | 1 |
| LV_FONT_MONTSERRAT_24 | 0 |

## 不包含

- DMA2D 加速 (STM32F103 不支持)
- SD 卡文件系统
- 外部 Flash 存储
- FreeRTOS (后续升级)
- 低功耗管理
