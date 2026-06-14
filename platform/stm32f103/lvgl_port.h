#ifndef LVGL_PORT_STM32_H
#define LVGL_PORT_STM32_H

#include "rtos_types.h"
#include "lvgl.h"

rtos_status_t lvgl_port_init(void);
void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px);
void lvgl_port_touch_read(lv_indev_t *indev, lv_indev_data_t *data);
void lvgl_port_present(void);

#endif
