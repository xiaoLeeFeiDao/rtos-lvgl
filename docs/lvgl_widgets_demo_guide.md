# LVGL Widgets Demo 实现架构详解

> 基于 `lv_demo_widgets.c` (LVGL v9.3)，文件路径：`demos/widgets/lv_demo_widgets.c`

---

## 整体架构

```
lv_demo_widgets()
  ├── 自适应屏幕尺寸 (DISP_SMALL / MEDIUM / LARGE)
  ├── 选择字体 (按屏幕尺寸)
  ├── 初始化主题 (lv_theme_default_init)
  ├── 创建预设 Style (style_text_muted, style_title, style_icon, style_bullet)
  ├── 创建 TabView → 添加 3 个 Tab：
  │   ├── t1 "Profile"  → profile_create()
  │   ├── t2 "Analytics" → analytics_create()
  │   └── t3 "Shop"     → shop_create()
  └── color_changer_create() → 右下角浮动颜色切换按钮
```

---

## 1. 入口：自适应与初始化

```c
void lv_demo_widgets(void) {
    // ① 根据屏幕宽度决定尺寸档位
    if(LV_HOR_RES <= 320) disp_size = DISP_SMALL;
    else if(LV_HOR_RES < 720) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    // ② 按档位选不同字号 (大屏用 24/16，中屏 20/14，小屏 18/12)
    font_large = &lv_font_montserrat_24;
    font_normal = &lv_font_montserrat_16;

    // ③ 初始化主题：主色=蓝，辅色=红，深色模式
    lv_theme_default_init(NULL,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        LV_THEME_DEFAULT_DARK, font_normal);

    // ④ 预定义 4 个 Style（复用，避免每个控件单独创建）
    lv_style_init(&style_text_muted);  // 50% 透明度文字
    lv_style_init(&style_title);       // 大号字体
    lv_style_init(&style_icon);        // 主题色图标
    lv_style_init(&style_bullet);      // 圆形无边框小圆点
```

### 关键 API

| API | 作用 |
|-----|------|
| `lv_theme_default_init()` | 初始化默认主题，整个应用统一配色 |
| `lv_style_init()` + `lv_style_set_*()` | 创建可复用的 Style 对象 |
| `lv_obj_add_style(obj, &style, PART)` | 把 Style 应用到控件的指定 PART |

---

## 2. 布局系统：Grid 和 Flex

### Grid 布局 (二维表格)

```c
// 定义列宽和行高数组
static int32_t grid_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static int32_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

// 应用到父容器
lv_obj_set_grid_dsc_array(parent, grid_col_dsc, grid_row_dsc);

// 每个子控件指定它在 Grid 中的位置和跨度
lv_obj_set_grid_cell(avatar,
    LV_GRID_ALIGN_CENTER,  // 列对齐
    0, 1,                  // col, col_span
    LV_GRID_ALIGN_CENTER,  // 行对齐
    0, 5);                 // row, row_span
```

### Grid 关键宏

| 宏 | 含义 |
|---|---|
| `LV_GRID_FR(n)` | 按比例分配剩余空间 (类似 CSS `fr`) |
| `LV_GRID_CONTENT` | 按内容自适应 |
| `LV_GRID_TEMPLATE_LAST` | 数组结束标记 |

### Flex 布局 (一维弹性)

```c
// 水平排列 + 换行
lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
lv_obj_set_flex_grow(cont, 1);                          // 弹性增长因子

// 垂直排列
lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
lv_obj_add_flag(list, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);   // 换新行/列
```

---

## 3. Tab 1 "Profile" — 表单控件

### 用到的控件

| 控件 | 创建 API | 关键设置 |
|------|---------|---------|
| Image | `lv_image_create()` | `lv_image_set_src(img, &img_avatar)` |
| Label | `lv_label_create()` | `lv_label_set_text_static()` 静态文本 |
| Button | `lv_button_create()` | `lv_obj_add_state(btn, LV_STATE_DISABLED)` 禁用态 |
| Textarea | `lv_textarea_create()` | `lv_textarea_set_one_line()` / `lv_textarea_set_password_mode()` |
| Dropdown | `lv_dropdown_create()` | `lv_dropdown_set_options_static("Male\nFemale\nOther")` |
| Slider | `lv_slider_create()` | `lv_obj_add_event_cb(slider, cb, LV_EVENT_ALL, NULL)` |
| Switch | `lv_switch_create()` | 二元切换，无需额外设置 |
| Keyboard | `lv_keyboard_create()` | 虚拟键盘，初始隐藏 |

### 键盘弹出/收起机制 (`ta_event_cb`)

```c
static void ta_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = lv_event_get_user_data(e);

    if(code == LV_EVENT_FOCUSED) {
        // 聚焦 → 弹出键盘，缩小 TabView 高度
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_set_height(tv, LV_VER_RES - lv_obj_get_height(kb));
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
    }
    else if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // 失焦 / 确认 / 取消 → 隐藏键盘
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_set_height(tv, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}
```

### 日历弹出机制 (`birthday_event_cb`)

```c
static void birthday_event_cb(lv_event_t *e) {
    if(code == LV_EVENT_FOCUSED) {
        // 在 lv_layer_top() 上弹出 calendar（最高层级）
        calendar = lv_calendar_create(lv_layer_top());
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);  // 半透明遮罩
        lv_calendar_add_header_dropdown(calendar);               // 年月下拉
        lv_obj_add_event_cb(calendar, calendar_event_cb, LV_EVENT_ALL, ta);
    }
}

static void calendar_event_cb(lv_event_t *e) {
    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t d;
        lv_calendar_get_pressed_date(calendar, &d);
        lv_textarea_set_text_fmt(ta, "%04d-%02d-%02d", d.year, d.month, d.day);
        lv_obj_delete(calendar);  // 选完日期后销毁日历
        calendar = NULL;
    }
}
```

---

## 4. Tab 2 "Analytics" — Chart + Scale + 动画

这是三个 tab 中最复杂的页面。

### 4.1 Chart 图表 + 刻度系统

`create_chart_with_scales()` 自定义函数，组装 **标题 + 竖刻度 + 图表 + 横刻度**：

```
┌──────────────────────────┐
│ Title                    │
│ ┌──────┬─────────────────┤
│ │ 竖   │                 │
│ │ 刻   │   Chart 区域    │
│ │ 度   │                 │
│ │      ├─────────────────┤
│ │      │  横刻度         │
│ └──────┴─────────────────┘
```

```c
// 创建带刻度的图表
chart1 = create_chart_with_scales(cont, "Unique visitors", chart1_texts);

// 添加数据系列
ser1 = lv_chart_add_series(chart1,
    lv_theme_get_color_primary(chart1),  // 用主题色
    LV_CHART_AXIS_PRIMARY_Y);           // 绑定左 Y 轴

// 填充 12 个数据点
for(i = 0; i < 12; i++) {
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
}
```

#### Scale 刻度控件 API

```c
lv_obj_t *scale = lv_scale_create(parent);
lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_LEFT);   // 竖向左
lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM); // 横向下
lv_scale_set_total_tick_count(scale, 11);                  // 刻度总数
lv_scale_set_major_tick_every(scale, 2);                   // 每隔几个标主刻度
lv_scale_set_range(scale, 0, 100);                          // 数值范围
lv_scale_set_text_src(scale, texts);                        // 自定义标签数组
```

### 4.2 Scale 多段仪表盘（3个圆盘）

#### Scale1 - 三色圆弧叠加

```c
lv_scale_set_mode(scale1, LV_SCALE_MODE_ROUND_OUTER);

// 三个弧线层层嵌套，每层 margin 不同（20, 40），实现同心圆弧
arc = lv_arc_create(scale1);
lv_obj_set_style_margin_all(arc, 20, 0);   // 内层偏移 20px
lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
```

#### Scale2 - 360° 环形仪表

```c
lv_scale_set_angle_range(scale2, 360);  // 整圆
lv_scale_set_text_src(scale2, scale2_text);  // ["0","10",...,"90"]
```

#### Scale3 - 225° 速度表 + 分段着色

```c
lv_scale_set_range(scale3, 10, 60);
lv_scale_set_angle_range(scale3, 225);   // 225° 圆弧
lv_scale_set_rotation(scale3, 135);       // 从 135° 开始

// Section API：不同区段不同颜色
lv_scale_section_t *section = lv_scale_add_section(scale3);
lv_scale_set_section_range(scale3, section, 0, 20);    // 红色区间
lv_scale_set_section_style_main(scale3, section, &red_style);

// 指针（用旋转的 Image 实现）
scale3_needle = lv_image_create(scale3);
lv_image_set_src(scale3_needle, &img_demo_widgets_needle);
lv_image_set_pivot(scale3_needle, 3, 4);  // 旋转中心点
```

### 4.3 动画系统 `lv_anim_t`

这是 Demo 中最核心的动态效果，驱动所有仪表盘指针和数据更新。

```c
lv_anim_t a;
lv_anim_init(&a);                                    // ① 初始化
lv_anim_set_values(&a, 20, 100);                     // ② 从 20 → 100
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE); // ③ 无限循环
lv_anim_set_exec_cb(&a, scale1_indic1_anim_cb);      // ④ 每帧回调
lv_anim_set_var(&a, arc);                            // ⑤ 回调参数 (void*)
lv_anim_set_duration(&a, 4100);                       // ⑥ 正向时间 ms
lv_anim_set_reverse_duration(&a, 2700);               // ⑦ 反向时间 ms
lv_anim_start(&a);                                    // ⑧ 启动！
```

#### 动画回调示例

```c
// Scale1 弧线值动画
static void scale1_indic1_anim_cb(void *var, int32_t v) {
    lv_arc_set_value((lv_obj_t *)var, v);
    // v 自动在 20→100→20→100... 之间平滑插值
}

// Scale3 复合动画（旋转指针 + 更新标签）
static void scale3_anim_cb(void *var, int32_t v) {
    // 旋转指针
    lv_image_set_rotation(scale3_needle, (v - 10) * 2250 / 50 + 1350);
    // 更新 Mbps 数字
    lv_label_set_text_fmt(scale3_mbps_label, "%d", v * 10);
}
```

### 4.4 Timer 定时器

```c
lv_timer_t *timer = lv_timer_create(scale2_timer_cb, 100, scale2);

// 每 100ms 调用，随机更新 Session 数据
static void scale2_timer_cb(lv_timer_t *timer) {
    session_desktop += lv_rand(-5, 5);
    lv_label_set_text_fmt(label, "Desktop: %" PRIu32, session_desktop);
    lv_arc_set_value(arc, session_desktop);
}
```

### 4.5 Size Changed 事件

```c
// 窗口大小变化时重新对齐
lv_obj_add_event_cb(scale3, scale3_size_changed_event_cb,
                    LV_EVENT_SIZE_CHANGED, NULL);
```

---

## 5. Tab 3 "Shop" — 电商面板

```
┌──────────────────────────────────┐
│ Monthly Summary     [柱状图区域] │
│ $27,123.25                      │
│ ↑ 17% growth this week          │
├──────────┬──────────┬───────────┤
│ Top      │ Notifications        │
│ products │ ☑ Item purchased     │
│ ...      │ ☑ New connection     │
│ (列表)   │ ✗ New subscriber     │
│          │ ☐ New message        │
│          │ ☑ Milestone reached  │
└──────────┴──────────┴───────────┘
```

### Checkbox 状态控制

```c
lv_obj_t *cb = lv_checkbox_create(notifications);
lv_checkbox_set_text(cb, "Item purchased");

lv_obj_add_state(cb, LV_STATE_CHECKED);                     // 选中
lv_obj_add_state(cb, LV_STATE_DISABLED);                    // 禁用
lv_obj_add_state(cb, LV_STATE_CHECKED | LV_STATE_DISABLED); // 选中+禁用
```

### 列表项辅助函数

```c
static lv_obj_t *create_shop_item(lv_obj_t *parent,
    const void *img_src, const char *name,
    const char *category, const char *price)
{
    // 用 Grid 布局：图片 | 名称 / 分类 | 价格
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(img, ..., 0, 1, ..., 0, 2);   // 跨 2 行
    lv_obj_set_grid_cell(name_label, ..., 2, 1, ..., 0, 1);
    lv_obj_set_grid_cell(cat_label, ..., 2, 1, ..., 1, 1);
    lv_obj_set_grid_cell(price_label, ..., 3, 1, ..., 0, 1);
}
```

---

## 6. 主题切换器 `color_changer_create()`

右下角浮动按钮，点击展开颜色选择器。

```c
// 浮动容器
lv_obj_t *color_cont = lv_obj_create(parent);
lv_obj_add_flag(color_cont, LV_OBJ_FLAG_FLOATING);  // 脱离布局流
lv_obj_set_flex_flow(color_cont, LV_FLEX_FLOW_ROW);
lv_obj_set_style_radius(color_cont, LV_RADIUS_CIRCLE, 0);
lv_obj_align(color_cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

// 每个颜色按钮绑定事件
lv_obj_t *c = lv_button_create(color_cont);
lv_obj_set_style_bg_color(c, lv_palette_main(palette[i]), 0);
lv_obj_add_event_cb(c, color_event_cb, LV_EVENT_ALL, &palette[i]);
```

### 点击颜色 → 全局主题切换

```c
static void color_event_cb(lv_event_t *e) {
    lv_palette_t *primary = lv_event_get_user_data(e);

    // 重新初始化主题 → 所有控件颜色实时变化
    lv_theme_default_init(NULL,
        lv_palette_main(*primary),
        lv_palette_main(*primary + 3),  // 辅色偏移 3 个色系
        LV_THEME_DEFAULT_DARK, font_normal);

    // 同步更新图表系列颜色
    lv_chart_set_series_color(chart1, ser1, lv_palette_main(*primary));
}
```

### 展开/收缩动画

```c
static void color_changer_anim_cb(void *var, int32_t v) {
    lv_obj_t *obj = var;
    // v: 0→256，映射为宽度变化
    int32_t w = lv_map(v, 0, 256, LV_DPX(60), max_w);
    lv_obj_set_width(obj, w);
    // 子按钮逐渐显示/隐藏
    for(i = 0; i < lv_obj_get_child_count(obj); i++)
        lv_obj_set_style_opa(lv_obj_get_child(obj, i), v, 0);
}
```

---

## 7. 幻灯片自动轮播

```c
void lv_demo_widgets_start_slideshow(void) {
    lv_obj_t *cont = lv_tabview_get_content(tv);
    lv_obj_t *tab = lv_obj_get_child(cont, 0);
    int32_t v = lv_obj_get_scroll_bottom(tab);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, scroll_anim_y_cb);              // 控制 scroll_y
    lv_anim_set_values(&a, 0, v);                            // 从顶滚到底
    lv_anim_set_completed_cb(&a, slideshow_anim_completed_cb); // 完成后切 tab
    lv_anim_start(&a);
}
```

---

## 8. 核心 API 分层总结

| 层级 | API | 作用 |
|------|-----|------|
| **生命周期** | `lv_init()` → `lv_tick_inc()` → `lv_timer_handler()` | 驱动 LVGL 心跳和渲染 |
| **主题** | `lv_theme_default_init()` | 全局统一配色，切换主题 |
| **容器** | `lv_obj_create()` | 所有控件的基类，也是布局容器 |
| **布局** | `lv_obj_set_grid_dsc_array()` / `lv_obj_set_flex_flow()` | Grid 二维 / Flex 一维布局 |
| **控件** | `lv_*_create()` 系列 | arc, bar, btn, calendar, chart, checkbox, dropdown, image, keyboard, label, scale, slider, switch, tabview, textarea 等 |
| **Style** | `lv_style_init()` → `lv_obj_add_style()` | 外观复用，支持多 PART |
| **事件** | `lv_obj_add_event_cb(obj, cb, EVENT, user_data)` | 交互处理，user_data 传递上下文 |
| **动画** | `lv_anim_init()` → `lv_anim_set_*()` → `lv_anim_start()` | 属性平滑过渡，支持正/反向、循环 |
| **定时器** | `lv_timer_create(cb, period_ms, user_data)` | 周期性任务 |
| **顶层** | `lv_layer_top()` | 最高层级，弹窗/遮罩用 |
| **状态** | `lv_obj_add_state(obj, STATE)` | CHECKED, DISABLED, FOCUSED 等 |
| **标志** | `lv_obj_add_flag(obj, FLAG)` | HIDDEN, FLOATING, CLICKABLE, FLEX_IN_NEW_TRACK 等 |

---

## 9. 事件驱动模式

```
用户操作
  → SDL 事件（鼠标按下/移动/松开）
    → lvgl_port_update_mouse(state, x, y)
      → lv_indev_read() 读取输入设备
        → lv_timer_handler() 处理事件分发
          → lv_obj_add_event_cb 注册的回调被触发
            → 控件状态更新 / 动画启动 / 值变更
```

### 常用事件码

| 事件 | 含义 |
|------|------|
| `LV_EVENT_CLICKED` | 点击 |
| `LV_EVENT_PRESSED` | 按下 |
| `LV_EVENT_VALUE_CHANGED` | 值改变（slider/arc/chart/calendar） |
| `LV_EVENT_FOCUSED` / `LV_EVENT_DEFOCUSED` | 聚焦/失焦 |
| `LV_EVENT_READY` / `LV_EVENT_CANCEL` | 键盘确认/取消 |
| `LV_EVENT_SIZE_CHANGED` | 尺寸变化 |
| `LV_EVENT_DELETE` | 控件被删除（清理资源） |
| `LV_EVENT_ALL` | 接收所有事件 |
