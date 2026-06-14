#include "lvgl_port.h"
#include "lvgl.h"
#include "rtos_time.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Internal state ---- */
static SDL_Texture  *g_tex  = NULL;
static SDL_Renderer *g_rend = NULL;
static int g_tw = 0, g_th = 0;

static lv_indev_state_t g_mouse_state = LV_INDEV_STATE_RELEASED;
static int g_mouse_x = 0, g_mouse_y = 0;

/* ---- LVGL display flush callback ---- */
void lvgl_port_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    /* XRGB8888 = 4 bytes per pixel. lv_color_t is 3 bytes (RGB888),
     * but the display's color format with LV_COLOR_DEPTH=32 is XRGB8888. */
    int px_size = 4;
    SDL_Rect rect = { area->x1, area->y1, w, h };
    void *pixels; int pitch;
    if (SDL_LockTexture(g_tex, &rect, &pixels, &pitch) == 0) {
        uint8_t *dst = pixels, *src = px;
        for (int y = 0; y < h; y++) {
            memcpy(dst, src, w * px_size);
            dst += pitch;
            src += w * px_size;
        }
        SDL_UnlockTexture(g_tex);
    }
    lv_display_flush_ready(disp);
}

/* ---- LVGL mouse input callback ---- */
void lvgl_port_mouse_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->state = g_mouse_state;
    data->point.x = g_mouse_x;
    data->point.y = g_mouse_y;
}

/* ---- Update mouse state from main thread ---- */
void lvgl_port_update_mouse(lv_indev_state_t state, int x, int y)
{
    g_mouse_state = state;
    g_mouse_x = x;
    g_mouse_y = y;
}

/* ---- Initialization ---- */
rtos_status_t lvgl_port_init(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return RTOS_ERR;

    SDL_Window *win = SDL_CreateWindow("RTOS LVGL Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { SDL_Quit(); return RTOS_ERR; }

    g_rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!g_rend) g_rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!g_rend) { SDL_DestroyWindow(win); SDL_Quit(); return RTOS_ERR; }

    SDL_GetRendererOutputSize(g_rend, &g_tw, &g_th);
    g_tex = SDL_CreateTexture(g_rend, SDL_PIXELFORMAT_XRGB8888,
                               SDL_TEXTUREACCESS_STREAMING, g_tw, g_th);

    /* Fill texture with background color (avoids garbage on first frame) */
    {
        uint32_t *ib = malloc(g_tw * g_th * 4);
        uint32_t bg = 0xFF1a1a2e;  /* XRGB8888 LE: B=2e,G=1a,R=1a,X=FF */
        for (int i = 0; i < g_tw * g_th; i++) ib[i] = bg;
        SDL_UpdateTexture(g_tex, NULL, ib, g_tw * 4);
        free(ib);
    }

    /* LVGL init */
    lv_init();
    lv_display_t *disp = lv_display_create(g_tw, g_th);

    /* XRGB8888 = 4 bytes per pixel. Match SDL texture format.
     * lv_color_t is only 3 bytes (R,G,B struct), so we allocate
     * raw byte buffers sized for the actual display pixel format. */
    int px_size = 4;
    uint32_t buf_px = g_tw * g_th / 10;
    uint32_t buf_sz = buf_px * px_size;
    uint8_t *buf1 = malloc(buf_sz);
    uint8_t *buf2 = malloc(buf_sz);
    lv_display_set_flush_cb(disp, lvgl_port_flush);
    lv_display_set_buffers(disp, buf1, buf2, buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *mouse = lv_indev_create();
    lv_indev_set_type(mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse, lvgl_port_mouse_read);

    return RTOS_OK;
}

/* ---- LVGL tick driver task ---- */
void lvgl_tick_task(void *param)
{
    (void)param;
    while (1) {
        lv_tick_inc(5);
        lv_timer_handler();
        rtos_delay_ms(5);
    }
}

/* ---- Render present (main thread only) ---- */
void lvgl_port_present(void)
{
    SDL_RenderCopy(g_rend, g_tex, NULL, NULL);
    SDL_RenderPresent(g_rend);
}

int lvgl_port_get_width(void)  { return g_tw; }
int lvgl_port_get_height(void) { return g_th; }

void lvgl_port_cleanup(void)
{
    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_rend) SDL_DestroyRenderer(g_rend);
    SDL_Quit();
}
