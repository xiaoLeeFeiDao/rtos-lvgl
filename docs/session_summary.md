# RTOS + LVGL v9 项目实战总结

> 涵盖：工程搭建、显示调试、事件处理、Demo 架构分析、Bug 修复全过程

---

## 一、工程搭建

### 1.1 项目结构

```
rtos/
├── CMakeLists.txt          # 顶层构建，通过 -DRTOS_TARGET=<name> 切换平台
├── lv_conf.h               # LVGL v9 配置 (LV_COLOR_DEPTH=32)
├── platform/
│   ├── include/            # RTOS 抽象头文件 (rtos_task/sync/time/mem)
│   ├── posix/              # POSIX pthread 模拟实现
│   └── {freertos,rtthread,threadx,liteos,alios,zephyr}/  # 24个桩文件
├── lvgl_port/
│   ├── lvgl_port.h         # LVGL 与 SDL2 的桥接接口
│   └── lvgl_port.c         # SDL2 渲染、鼠标输入、LVGL 初始化
├── apps/
│   ├── demo/               # 简单仪表盘 Demo (gauge + button + slider)
│   └── complex/            # LVGL 官方 Widgets 展示 Demo
├── docs/                   # 文档
└── build/                  # CMake 构建输出
```

### 1.2 CMake 核心配置

```cmake
# RTOS 目标选择
set(RTOS_TARGET "posix" CACHE STRING "...")
set(SUPPORTED_RTOS posix freertos rtthread threadx liteos alios zephyr)

# 获取 LVGL v9.3.0
FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git GIT_TAG v9.3.0)

# 编译定义
target_compile_definitions(rtos_demo PRIVATE
    RTOS_TARGET_${RTOS_TARGET}
    LV_CONF_INCLUDE_SIMPLE      # 使用 #include "lv_conf.h"
    LV_LVGL_H_INCLUDE_SIMPLE    # 使用 #include "lvgl.h"
)

# 链接
target_link_libraries(rtos_demo PRIVATE lvgl lvgl::examples SDL2::SDL2)
```

### 1.3 关键编译宏

| 宏 | 作用 |
|---|------|
| `LV_CONF_INCLUDE_SIMPLE` | LVGL 头文件用 `#include "lv_conf.h"` 而非相对路径 |
| `LV_KCONFIG_IGNORE` | LVGL CMake 自动添加，忽略 Kconfig，使用 lv_conf.h |
| `LV_COLOR_DEPTH 32` | 显示色深 = XRGB8888 (4字节/像素) |

---

## 二、显示问题排查与修复

### 2.1 核心问题：像素格式不匹配

**现象**：界面显示多条圆弧并排、横条纹、颜色错乱

**根因**：`lv_color_t` 在 LVGL v9 中是 3 字节结构体（RGB888），但当 `LV_COLOR_DEPTH=32` 时，display 格式是 `LV_COLOR_FORMAT_XRGB8888` = 4 字节/像素。我们的代码使用 `sizeof(lv_color_t)` (3) 来：
1. 分配绘制缓冲区 — 少分配了 25% 空间
2. flush 拷贝每行数据 — 每行少拷贝 1/4 字节

```c
// LVGL v9 的 lv_color_t 定义 (src/misc/lv_color.h:109)
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;  // 始终 3 字节！

// LV_COLOR_DEPTH 决定显示格式 (src/misc/lv_color.h:214)
#if LV_COLOR_DEPTH == 32
    LV_COLOR_FORMAT_NATIVE = LV_COLOR_FORMAT_XRGB8888,  // 4 字节/像素
#endif
```

**修复**：`lvgl_port.c` 中全部用硬编码 4 字节/像素：

```c
// lvgl_port_flush — 必须用 4，不能用 sizeof(lv_color_t)
int px_size = 4;
for (int y = 0; y < h; y++) {
    memcpy(dst, src, w * px_size);
    dst += pitch;
    src += w * px_size;
}

// lvgl_port_init — 缓冲区用 uint8_t* 分配，大小 = 像素数 × 4
int px_size = 4;
uint32_t buf_sz = g_tw * g_th / 10 * px_size;
uint8_t *buf1 = malloc(buf_sz);
lv_display_set_buffers(disp, buf1, buf2, buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
```

### 2.2 线程安全

**现象**：SDL LockTexture 从后台线程调用，Metal/GPU 产生未定义行为

**修复**：改用单线程模型，`lv_tick_inc()` + `lv_timer_handler()` + SDL 渲染全部在主循环中：

```c
while (g_running) {
    SDL_PollEvent(&ev);       // 事件
    lv_tick_inc(16);          // LVGL 心跳
    lv_timer_handler();       // LVGL 渲染(主线程)
    dashboard_update(...);    // UI 更新(主线程)
    lvgl_port_present();      // SDL 呈现
    SDL_Delay(16);
}
```

### 2.3 其他显示要点

| 要点 | 细节 |
|------|------|
| Retina/HiDPI | `SDL_WINDOW_ALLOW_HIGHDPI` + `SDL_GetRendererOutputSize` |
| 像素格式 | Apple Silicon LE: `SDL_PIXELFORMAT_XRGB8888` ↔ LVGL XRGB8888 |
| 纹理访问 | `SDL_TEXTUREACCESS_STREAMING` + `SDL_LockTexture` |
| 渲染模式 | `LV_DISPLAY_RENDER_MODE_PARTIAL` (FULL 模式在预编译库会卡死) |

---

## 三、鼠标事件处理

### 3.1 坐标缩放

```c
float scale = (float)lvgl_port_get_width() / 960;
// SDL 返回逻辑坐标，LVGL 需要物理坐标
ev.button.x * scale, ev.button.y * scale
```

### 3.2 拖动与松手修复

**Bug 1** — `SDL_MOUSEMOTION` 一直传 `LV_INDEV_STATE_PRESSED`，导致悬浮就触发拖动：

```c
// ❌ 错误
if (ev.type == SDL_MOUSEMOTION)
    lvgl_port_update_mouse(LV_INDEV_STATE_PRESSED, x, y);

// ✅ 正确 — 追踪按键状态
static int mouse_down = 0;
if (ev.type == SDL_MOUSEMOTION) {
    lv_indev_state_t st = mouse_down ? LV_INDEV_STATE_PRESSED
                                     : LV_INDEV_STATE_RELEASED;
    lvgl_port_update_mouse(st, x, y);
}
```

**Bug 2** — `MOUSEBUTTONUP` 把坐标设成 (0,0)，滑块弹回起点：

```c
// ❌ 错误 — 坐标变成 (0,0)，slider 以为是拖到最左
lvgl_port_update_mouse(LV_INDEV_STATE_RELEASED, 0, 0);

// ✅ 正确 — 保持松手时的坐标
lvgl_port_update_mouse(LV_INDEV_STATE_RELEASED, x, y);
```

---

## 四、LVGL Widgets Demo 架构

### 4.1 整体结构

```
lv_demo_widgets()
├── 自适应屏幕尺寸 (DISP_SMALL/MEDIUM/LARGE)
├── 选择字体 (24/16, 20/14, 18/12)
├── lv_theme_default_init() 初始化主题
├── 预定义 Style (muted, title, icon, bullet)
├── TabView → 3 个 Tab
│   ├── "Profile"  → profile_create()    表单+键盘+日历
│   ├── "Analytics" → analytics_create() 图表+仪表盘+动画
│   └── "Shop"     → shop_create()       电商面板+Checkbox
└── color_changer_create() → 浮动主题切换
```

### 4.2 布局系统

**Grid 布局 (二维表格)**：
```c
static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static int32_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
lv_obj_set_grid_cell(child, LV_GRID_ALIGN_STRETCH, col, span, LV_GRID_ALIGN_START, row, span);
```

**Grid 宏**：
| 宏 | 含义 |
|---|---|
| `LV_GRID_FR(n)` | 按比例分配剩余空间 |
| `LV_GRID_CONTENT` | 按内容自适应 |
| `LV_GRID_TEMPLATE_LAST` | 数组结束标记 |

**Flex 布局 (一维弹性)**：
```c
lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
lv_obj_set_flex_grow(child, 1);
lv_obj_add_flag(child, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);  // 强制换行
```

### 4.3 控件速查

| 控件 | 创建 | 关键设置 |
|------|------|---------|
| Label | `lv_label_create()` | `lv_label_set_text_static()` |
| Button | `lv_button_create()` | `lv_obj_add_state(btn, LV_STATE_DISABLED)` |
| Textarea | `lv_textarea_create()` | `lv_textarea_set_one_line()` / `_password_mode()` |
| Dropdown | `lv_dropdown_create()` | `lv_dropdown_set_options_static("A\nB\nC")` |
| Slider | `lv_slider_create()` | `lv_slider_set_range()` / `_set_value()` |
| Switch | `lv_switch_create()` | 二元切换 |
| Keyboard | `lv_keyboard_create()` | `lv_keyboard_set_textarea(kb, ta)` |
| Calendar | `lv_calendar_create()` | `lv_calendar_add_header_dropdown()` |
| Chart | `lv_chart_create()` | `lv_chart_add_series()` / `_set_next_value()` |
| Scale | `lv_scale_create()` | `lv_scale_set_mode()` / `_set_range()` / `_add_section()` |
| Arc | `lv_arc_create()` | `lv_arc_set_range()` / `_set_value()` |
| Checkbox | `lv_checkbox_create()` | `lv_obj_add_state(cb, LV_STATE_CHECKED)` |
| TabView | `lv_tabview_create()` | `lv_tabview_add_tab()` |

### 4.4 动画系统

```c
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, obj);                       // 目标对象
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_height);  // 每帧回调
lv_anim_set_values(&a, from_val, to_val);       // 起止值
lv_anim_set_duration(&a, 250);                   // 持续时间 ms
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_start(&a);
```

### 4.5 事件系统

```c
lv_obj_add_event_cb(obj, callback, LV_EVENT_ALL, user_data);

// 常用事件码
LV_EVENT_CLICKED        // 点击
LV_EVENT_VALUE_CHANGED  // 值改变
LV_EVENT_FOCUSED        // 获得焦点
LV_EVENT_DEFOCUSED      // 失去焦点
LV_EVENT_READY          // 键盘确认
LV_EVENT_CANCEL         // 键盘取消
LV_EVENT_DELETE         // 控件销毁
LV_EVENT_SIZE_CHANGED   // 尺寸变化
```

---

## 五、键盘弹出/页面缩放 Bug 修复

### 5.1 官方 Demo 的 3 个 Bug

`ta_event_cb` 函数中键盘弹出时的操作顺序错误：

```c
// ❌ 官方原版 (Bug)
if(code == LV_EVENT_FOCUSED) {
    // 1. 取键盘高度 → kb 隐藏中，返回 0
    // 2. lv_obj_update_layout(tv) → kb 不在 tv 子树，白更新
    // 3. tv = LV_VER_RES - 0 = 全高 → 没缩小
    // 4. 显示键盘 → 键盘盖在页面上
}

// ✅ 修复后
if(code == LV_EVENT_FOCUSED) {
    lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);   // ① 先显示
    lv_obj_update_layout(kb);                      // ② 更新键盘布局
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);  // ③ 对齐底部
    int32_t kb_h = lv_obj_get_height(kb);          // ④ 拿到真实高度
    // ⑤ 动画缩小 tv
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, tv);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_height);
    lv_anim_set_values(&a, lv_obj_get_height(tv), LV_VER_RES - kb_h);
    lv_anim_set_duration(&a, 250);
    lv_anim_start(&a);
}
```

**修复要点**：
| # | Bug | 修复 |
|---|-----|------|
| 1 | kb 隐藏时 `lv_obj_get_height` = 0 | 先 `remove_flag(HIDDEN)` 再取高度 |
| 2 | `lv_obj_update_layout(tv)` — kb 不在 tv 子树 | 改成 `lv_obj_update_layout(kb)` |
| 3 | `lv_obj_set_height` 不生效 | 用 `lv_anim_t` 动画驱动高度变化 |
| 4 | kb 默认位置在左上角 | `lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0)` |

### 5.2 Profile 页面布局优化

**问题**：Profile 面板用 `LV_GRID_CONTENT` 行，内容挤在顶部，下面大片空白，显得像键盘已预留空间。

**修复**：改用 `LV_GRID_FR` 让面板按比例填满高度：

```c
// ❌ 之前 — 内容挤在顶部
static int32_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, ...};

// ✅ 之后 — 按 3:2 比例填充
static int32_t grid_main_row_dsc[] = {LV_GRID_FR(3), LV_GRID_FR(2), ...};
```

同时 `panel2` 从 `LV_GRID_ALIGN_START` 改为 `LV_GRID_ALIGN_STRETCH`，与 `panel3` 等高。

---

## 六、LVGL 调试技巧

### 6.1 日志系统

```c
// lv_conf.h
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

// 代码中使用
LV_LOG_WARN("kb_h=%d tv_h=%d", (int)lv_obj_get_height(kb), (int)lv_obj_get_height(tv));
```

### 6.2 常见调试手段

| 手段 | 说明 |
|------|------|
| `lv_obj_get_height/width` | 获取控件布局后的实际尺寸 |
| `lv_obj_get_child_count` | 遍历子控件 |
| `lv_obj_get_parent` | 获取父容器 |
| `lv_obj_check_type(obj, &lv_textarea_class)` | 类型检查 |
| `lv_obj_send_event(obj, EVENT, param)` | 手动发送事件 |
| `lv_obj_update_layout(obj)` | 强制立即更新布局 |

---

## 七、API 分层速查

| 层级 | API | 作用 |
|------|-----|------|
| 生命周期 | `lv_init()` → `lv_tick_inc(n)` → `lv_timer_handler()` | 驱动 LVGL |
| 主题 | `lv_theme_default_init()` | 全局配色 |
| 容器 | `lv_obj_create()` + Grid/Flex | 布局 |
| 控件 | `lv_*_create()` 系列 | UI 元素 |
| Style | `lv_style_init()` + `lv_obj_add_style()` | 外观复用 |
| 事件 | `lv_obj_add_event_cb(obj, cb, EVENT, data)` | 交互 |
| 动画 | `lv_anim_init()` → `_set_*()` → `_start()` | 属性过渡 |
| 定时器 | `lv_timer_create(cb, ms, data)` | 周期任务 |
| Flag | `lv_obj_add/remove_flag()` | HIDDEN, FLOATING, CLICKABLE 等 |
| State | `lv_obj_add/remove_state()` | CHECKED, DISABLED, FOCUSED 等 |
| 对齐 | `lv_obj_align(obj, ALIGN, x, y)` | 相对父容器定位 |
| 层级 | `lv_layer_top()` | 最高层弹窗 |

---

## 八、构建与运行

```bash
# 配置
cd build && cmake .. -DRTOS_TARGET=posix -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(sysctl -n hw.ncpu)

# 运行
./build/rtos_demo      # 简单仪表盘
./build/complex_demo   # LVGL Widgets 展示
```

---

## 九、知识点覆盖情况

Widgets Demo 覆盖了 ~60% 的 LVGL API，未覆盖的包括：Canvas、Span、Table、Tileview、Lottie、QRCode、Observer、Fragment、XML UI、多显示器、GPU 加速、自定义控件等。详见 `docs/lvgl_widgets_demo_guide.md`。
