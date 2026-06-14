# RTOS Platform Layer + LVGL Simulator Design

**Date:** 2026-05-31
**Type:** Learning project — horizontal comparison of RTOS implementations

## Goal

Build a CMake-based project where switching `-DRTOS_TARGET=<rtos_name>` compiles the same demo app against different RTOS implementations (FreeRTOS, RT-Thread, ThreadX, LiteOS, AliOS, Zephyr), running on Linux via POSIX threads simulation, with LVGL v9 UI displayed via SDL2.

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Build system | CMake | LVGL official simulator uses CMake; Zephyr uses CMake; one build system to learn |
| RTOS simulation | POSIX threads (pthread) | One process, multiple threads; simulates real preemption; shared SDL window |
| Directory style | Flat, shared platform/ | Easy to compare "same function, different RTOS"; add new RTOS by adding one folder |
| Abstraction API | task, sync, time, memory | Core primitives; defer queue/timer/software-timer (too divergent) |
| LVGL version | v9 (latest stable) | Official Linux port support; modern API |
| Demo content | Dashboard (gauge + labels + button + slider) | Covers basic widgets and event system |

## Architecture

```
CMakeLists.txt
  └── -DRTOS_TARGET=freertos|rtthread|threadx|liteos|alios|zephyr
        └── selects one platform/<rtos>/ implementation
              └── implements rtos_task.h, rtos_sync.h, rtos_time.h, rtos_mem.h
                    └── linked into apps/demo/
                          └── main.c → dashboard.c → lvgl
                                └── lvgl_port/ → SDL2 display
```

## Components

### 1. `platform/include/` — Public abstraction headers

**`rtos_types.h`** — Unified types: `rtos_handle_t`, `rtos_status_t`, `rtos_prio_t`, `rtos_tick_t`

**`rtos_task.h`** — Task lifecycle:
- `rtos_task_create(name, priority, stack_size, entry_fn, param) → handle`
- `rtos_task_delete(handle)`
- `rtos_task_suspend(handle)`
- `rtos_task_resume(handle)`
- `rtos_task_yield()`

**`rtos_sync.h`** — Synchronization primitives:
- `rtos_sem_create(count) → handle`
- `rtos_sem_take(handle, timeout)`
- `rtos_sem_give(handle)`
- `rtos_mutex_create() → handle`
- `rtos_mutex_lock(handle, timeout)`
- `rtos_mutex_unlock(handle)`
- `rtos_event_create() → handle`
- `rtos_event_wait(handle, flags, timeout)`

**`rtos_time.h`** — Time:
- `rtos_delay_ms(ms)`
- `rtos_delay_ticks(ticks)`
- `rtos_tick_get() → tick_count`

**`rtos_mem.h`** — Memory:
- `rtos_malloc(size)`
- `rtos_free(ptr)`
- `rtos_mem_pool_create(pool_ptr, size, block_count)`
- `rtos_mem_pool_alloc(pool, size)`
- `rtos_mem_pool_free(pool, ptr)`

### 2. `platform/<rtos>/` — RTOS-specific implementations

Each RTOS folder contains `.c` files implementing the headers above:
- `platform/freertos/rtos_task.c` — wraps FreeRTOS xTaskCreate, vTaskDelete, etc.
- `platform/rtthread/rtos_task.c` — wraps rt_thread_create, rt_thread_delete, etc.
- `platform/threadx/rtos_task.c` — wraps tx_thread_create, tx_thread_delete, etc.
- `platform/liteos/rtos_task.c` — wraps LOS_TaskCreate, etc.
- `platform/alios/rtos_task.c` — wraps aos_task_new, etc.
- `platform/zephyr/rtos_task.c` — wraps k_thread_create, etc.

POSIX simulation: each `rtos_<foo>.c` uses pthread primitives underneath (pthread_create, pthread_mutex, sem_t, etc.), mapping the abstract API directly to POSIX. The scheduler logic (priority-based, preemption) is implemented in a lightweight `platform/posix/rtos_scheduler.c`.

### 3. `lvgl_port/` — LVGL adaptation layer

**`lvgl_port_linux.c`** — SDL2 display driver + input driver for Linux
- `lvgl_port_init()` — initialize SDL2 window (800x480), register LVGL display/input drivers
- Uses `lv_linux_sdl_init()` or manual SDL2 setup (v9 compatible)

**`lvgl_port_task.c`** — LVGL tick task
- Runs `lv_timer_handler()` every 5ms in a dedicated RTOS task
- Uses `rtos_task_create()` from the abstraction layer

### 4. `lvgl/` — LVGL v9 source

Fetched via CMake FetchContent from `https://github.com/lvgl/lvgl.git` (v9.x branch).

### 5. `apps/demo/` — Demo application

**`main.c`** — Entry point:
1. `rtos_platform_init()` (if needed)
2. `lvgl_port_init()` — init SDL2 + LVGL
3. `rtos_task_create("lvgl_tick", prio, stack, lvgl_tick_task, NULL)` — LVGL tick driver task
4. `rtos_task_create("dashboard", prio, stack, dashboard_task, NULL)` — demo UI task
5. Block forever (or delete self)

**`dashboard.c`** — UI demo:
- Gauge/arc widget (仪表盘)
- Label showing tick count (实时数据)
- Button with click callback
- Slider with value change callback

## Build

```bash
mkdir build && cd build
cmake .. -DRTOS_TARGET=freertos
make -j$(nproc)
./rtos_demo
```

Switch RTOS: change `-DRTOS_TARGET=rtthread` etc.

## Build Targets

| RTOS_TARGET | Notes |
|-------------|-------|
| `freertos` | Pure POSIX simulation, no hardware deps |
| `rtthread` | POSIX simulation of RT-Thread kernel |
| `threadx` | POSIX simulation of ThreadX kernel |
| `liteos` | POSIX simulation of LiteOS kernel |
| `alios` | POSIX simulation of AliOS kernel |
| `zephyr` | Uses Zephyr's native_posix board (special case) |
| `posix` | Direct POSIX implementation (baseline, no RTOS) |

## Error Handling

- All abstraction functions return `rtos_status_t` (RTOS_OK, RTOS_ERR, RTOS_ERR_TIMEOUT, RTOS_ERR_INVALID_PARAM)
- `rtos_assert(expr)` — debug assertions, prints to stderr in simulation mode
- CMake: if RTOS_TARGET is invalid, `message(FATAL_ERROR)` at configure time

## Testing

- Compile each RTOS_TARGET successfully
- Run demo, verify SDL2 window appears with dashboard UI
- Verify button click and slider interaction work
- Verify LVGL tick runs (label updates every second)

## Future (Out of Scope)

- Message queues, software timers, event flags (deferred to v2)
- Hardware target compilation (STM32, ESP32, etc.)
- Unit test framework integration
- Performance benchmarking between RTOS implementations
