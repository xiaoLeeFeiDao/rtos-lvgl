#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "rtos_types.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

rtos_status_t lvgl_port_init(int width, int height);

/* LVGL display flush callback — called by LVGL from tick thread */
void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px);

/* LVGL mouse input callback */
void lvgl_port_mouse_read(lv_indev_t *indev, lv_indev_data_t *data);

/* Update mouse state from main thread (SDL events) */
void lvgl_port_update_mouse(lv_indev_state_t state, int x, int y);

/* LVGL tick task — runs in background RTOS thread */
void lvgl_tick_task(void *param);

/* Render present — must be called from main thread */
void lvgl_port_present(void);

int  lvgl_port_get_width(void);
int  lvgl_port_get_height(void);
void lvgl_port_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
