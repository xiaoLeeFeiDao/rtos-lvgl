#include "lvgl_port.h"
#include "lcd.h"
#include "touch.h"
#include <string.h>

static lv_color_t g_buf1[320 * 10];

void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);

    lcd_set_window((uint16_t)area->x1, (uint16_t)area->y1,
                   (uint16_t)area->x2, (uint16_t)area->y2);

    uint16_t *pixels = (uint16_t *)px;
    uint32_t count = (uint32_t)w * h;
    for (uint32_t i = 0; i < count; i++)
        lcd_write_ram(*pixels++);

    lv_display_flush_ready(disp);
}

void lvgl_port_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t x, y;
    if (touch_read(&x, &y)) {
        data->state   = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state   = LV_INDEV_STATE_RELEASED;
    }
}

rtos_status_t lvgl_port_init(void)
{
    lv_init();

    lv_display_t *disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_flush_cb(disp, lvgl_port_flush);
    lv_display_set_buffers(disp, g_buf1, NULL,
                           sizeof(g_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_port_touch_read);

    return RTOS_OK;
}

void lvgl_port_present(void) {}
