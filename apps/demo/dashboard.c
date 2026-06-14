#include "dashboard.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include <stdio.h>

static lv_obj_t *g_gauge = NULL;
static lv_obj_t *g_tick_lbl = NULL;
static lv_obj_t *g_btn_lbl = NULL;
static int g_click_count = 0;

static void btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        g_click_count++;
        char buf[32];
        snprintf(buf, sizeof(buf), "Clicks: %d", g_click_count);
        lv_label_set_text(g_btn_lbl, buf);
    }
}

static int g_created = 0;

void dashboard_create(void)
{
    if (g_created) return;
    g_created = 1;

    float s = (float)lvgl_port_get_width() / 960;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Gauge */
    g_gauge = lv_arc_create(lv_scr_act());
    lv_obj_set_size(g_gauge, 170 * s, 170 * s);
    lv_obj_align(g_gauge, LV_ALIGN_TOP_MID, 0, 20 * s);
    lv_arc_set_rotation(g_gauge, 135);
    lv_arc_set_bg_angles(g_gauge, 0, 270);
    lv_arc_set_range(g_gauge, 0, 100);
    lv_arc_set_value(g_gauge, 0);
    lv_obj_set_style_arc_color(g_gauge, lv_color_hex(0x00d4ff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g_gauge, 14 * s, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g_gauge, lv_color_hex(0x2a2a3e), LV_PART_MAIN);

    /* Tick label */
    g_tick_lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(g_tick_lbl, "Tick: 0");
    lv_obj_align(g_tick_lbl, LV_ALIGN_CENTER, 0, 80 * s);
    lv_obj_set_style_text_color(g_tick_lbl, lv_color_hex(0xc8c8ff), LV_PART_MAIN);

    /* Button */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 130 * s, 50 * s);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 20 * s, -20 * s);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00d4ff), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    g_btn_lbl = lv_label_create(btn);
    lv_label_set_text(g_btn_lbl, "Clicks: 0");
    lv_obj_center(g_btn_lbl);
    lv_obj_set_style_text_color(g_btn_lbl, lv_color_hex(0x000000), LV_PART_MAIN);

    /* Slider */
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 260 * s, 12 * s);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_RIGHT, -20 * s, -20 * s);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);

    /* Force first render */
    lv_obj_invalidate(lv_scr_act());
}

/* Called from main thread — safe to touch LVGL widgets */
void dashboard_update(int tick, int gauge_val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Tick: %d", tick);
    lv_label_set_text(g_tick_lbl, buf);

    snprintf(buf, sizeof(buf), "Clicks: %d", g_click_count);
    lv_label_set_text(g_btn_lbl, buf);

    lv_arc_set_value(g_gauge, gauge_val);
}
