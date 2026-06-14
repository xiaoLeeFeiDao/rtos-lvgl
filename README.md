# RTOS Platform Layer + LVGL Simulator

A CMake-based project for learning and comparing multiple RTOS implementations on PC via POSIX thread simulation, with LVGL v9 GUI displayed through SDL2.

Switch RTOS target with one CMake flag: `-DRTOS_TARGET=<name>`

## Supported RTOS Targets

| RTOS_TARGET | Description |
|-------------|-------------|
| `posix` | Direct POSIX implementation (baseline) |
| `freertos` | FreeRTOS API simulation |
| `rtthread` | RT-Thread API simulation |
| `threadx` | ThreadX (Azure RT) API simulation |
| `liteos` | Huawei LiteOS API simulation |
| `alios` | AliOS API simulation |
| `zephyr` | Zephyr API simulation |

## Prerequisites

- CMake >= 3.20
- SDL2 (`brew install sdl2` on macOS)
- GCC or Clang

## Build

```bash
mkdir build && cd build
cmake .. -DRTOS_TARGET=posix
make -j$(nproc)
./rtos_demo
```

Switch RTOS by changing `-DRTOS_TARGET=<name>`.

## Project Structure

```
├── CMakeLists.txt
├── lv_conf.h               # LVGL v9 configuration
├── platform/
│   ├── include/            # RTOS abstraction headers
│   │   ├── rtos_types.h    # Unified types: status, handle, prio, tick
│   │   ├── rtos_task.h     # Task: create, delete, suspend, resume, yield
│   │   ├── rtos_sync.h     # Sync: semaphore, mutex, event group
│   │   ├── rtos_time.h     # Time: delay_ms, delay_ticks, tick_get
│   │   └── rtos_mem.h      # Memory: malloc, free, pool
│   ├── posix/              # POSIX implementation (pthread based)
│   ├── freertos/           # FreeRTOS simulation (POSIX bridge)
│   ├── rtthread/           # RT-Thread simulation
│   ├── threadx/            # ThreadX simulation
│   ├── liteos/             # LiteOS simulation
│   ├── alios/              # AliOS simulation
│   └── zephyr/             # Zephyr simulation
├── lvgl_port/
│   ├── lvgl_port.h         # LVGL port API
│   └── lvgl_port.c         # SDL2 display driver + tick task
├── lvgl/                   # Fetched by CMake (v9.3.0)
└── apps/demo/
    ├── main.c              # Entry point
    ├── dashboard.c         # Dashboard UI: gauge, labels, button, slider
    └── dashboard.h
```

## RTOS Abstraction API

- **Task:** `rtos_task_create`, `rtos_task_delete`, `rtos_task_suspend`, `rtos_task_resume`, `rtos_task_yield`
- **Sync:** `rtos_sem_create/take/give`, `rtos_mutex_create/lock/unlock`, `rtos_event_create/wait`
- **Time:** `rtos_delay_ms`, `rtos_delay_ticks`, `rtos_tick_get`
- **Memory:** `rtos_malloc`, `rtos_free`, `rtos_mem_pool_create/alloc/free`

## Demo

The dashboard demo shows:
- An arc gauge at top center that animates 0→100
- A tick counter label updating every second
- A clickable button (bottom-left) with click counter
- A draggable slider (bottom-right) with value logging
