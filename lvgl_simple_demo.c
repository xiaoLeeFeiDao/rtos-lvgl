/*
 * Simple standalone LVGL v9 + SDL2 demo
 * No RTOS, no threads — single process, single thread
 */

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  480

static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture  *g_texture  = NULL;
static lv_display_t *g_display  = NULL;
static lv_color_t   *g_buf1     = NULL;

/* ---- Display driver: flush callback ---- */
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int w = lv_area_get_width(area);
    int h = lv_area_get_height(area);
    SDL_Rect rect = { area->x1, area->y1, w, h };

    SDL_UpdateTexture(g_texture, &rect, px_map, w * sizeof(lv_color_t));
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
    lv_display_flush_ready(disp);
}

/* ---- Input driver: read callback ---- */
static lv_indev_state_t g_mouse_state = LV_INDEV_STATE_RELEASED;
static int g_mouse_x = 0, g_mouse_y = 0;

static void mouse_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->state = g_mouse_state;
    data->point.x = g_mouse_x;
    data->point.y = g_mouse_y;
}

/* ---- UI: Dashboard Demo ---- */
static lv_obj_t *g_gauge     = NULL;
static lv_obj_t *g_tick_lbl  = NULL;
static lv_obj_t *g_btn_lbl   = NULL;
static lv_obj_t *g_slider_lbl = NULL;
static int g_click_count = 0;

static void btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        g_click_count++;
        char buf[32];
        snprintf(buf, sizeof(buf), "Clicked: %d", g_click_count);
        lv_label_set_text(g_btn_lbl, buf);
    }
}

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", (long)val);
    lv_label_set_text(g_slider_lbl, buf);
}

static void create_dashboard(void)
{
    /* Dark background */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* ---- Gauge (arc at top center) ---- */
    g_gauge = lv_arc_create(lv_scr_act());
    lv_obj_set_size(g_gauge, 180, 180);
    lv_obj_align(g_gauge, LV_ALIGN_TOP_MID, 0, 20);
    lv_arc_set_rotation(g_gauge, 135);
    lv_arc_set_bg_angles(g_gauge, 0, 270);
    lv_arc_set_range(g_gauge, 0, 100);
    lv_arc_set_value(g_gauge, 0);
    lv_obj_set_style_arc_color(g_gauge, lv_color_hex(0x00d4ff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g_gauge, 12, LV_PART_INDICATOR);

    /* Tick label (below gauge) */
    g_tick_lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(g_tick_lbl, "Tick: 0");
    lv_obj_align(g_tick_lbl, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_text_color(g_tick_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_tick_lbl, &lv_font_montserrat_14, LV_PART_MAIN);

    /* ---- Button (bottom left) ---- */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x00d4ff), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    g_btn_lbl = lv_label_create(btn);
    lv_label_set_text(g_btn_lbl, "Click me!");
    lv_obj_center(g_btn_lbl);

    /* ---- Slider (bottom right) ---- */
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 200, 10);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    g_slider_lbl = lv_label_create(lv_scr_act());
    lv_obj_align_to(g_slider_lbl, slider, LV_ALIGN_OUT_TOP_MID, 0, -5);
    lv_label_set_text(g_slider_lbl, "50");
    lv_obj_set_style_text_color(g_slider_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    /* ---- SDL2 init ---- */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    g_window = SDL_CreateWindow("LVGL Simple Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!g_window) { fprintf(stderr, "SDL_CreateWindow failed\n"); return 1; }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_renderer) { SDL_DestroyWindow(g_window); return 1; }

    int tex_w, tex_h;
    SDL_GetRendererOutputSize(g_renderer, &tex_w, &tex_h);
    fprintf(stderr, "[LVGL] Window: %dx%d, Texture: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT, tex_w, tex_h);

    g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC, tex_w, tex_h);

    /* ---- LVGL init ---- */
    lv_init();

    g_display = lv_display_create(tex_w, tex_h);
    if (!g_display) { fprintf(stderr, "lv_display_create failed\n"); return 1; }

    uint32_t buf_px = (uint32_t)(tex_w * tex_h);
    g_buf1 = (lv_color_t *)malloc(buf_px * sizeof(lv_color_t));
    if (!g_buf1) { fprintf(stderr, "buffer alloc failed\n"); return 1; }

    lv_display_set_buffers(g_display, g_buf1, NULL, buf_px * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(g_display, disp_flush);

    lv_indev_t *mouse = lv_indev_create();
    lv_indev_set_type(mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse, mouse_read);

    /* ---- Create UI ---- */
    create_dashboard();

    /* ---- Main loop ---- */
    SDL_Event event;
    uint32_t last_update = 0;
    uint32_t gauge_val = 0;
    int running = 1;

    fprintf(stderr, "[Demo] Running...\n");

    while (running) {
        uint32_t now = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0; break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) running = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    g_mouse_state = LV_INDEV_STATE_PRESSED;
                    float sx = (float)tex_w / WINDOW_WIDTH;
                    float sy = (float)tex_h / WINDOW_HEIGHT;
                    g_mouse_x = (int)(event.button.x * sx);
                    g_mouse_y = (int)(event.button.y * sy);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    g_mouse_state = LV_INDEV_STATE_RELEASED;
                    float sx = (float)tex_w / WINDOW_WIDTH;
                    float sy = (float)tex_h / WINDOW_HEIGHT;
                    g_mouse_x = (int)(event.button.x * sx);
                    g_mouse_y = (int)(event.button.y * sy);
                }
                break;
            case SDL_MOUSEMOTION:
                {
                    float sx = (float)tex_w / WINDOW_WIDTH;
                    float sy = (float)tex_h / WINDOW_HEIGHT;
                    g_mouse_x = (int)(event.motion.x * sx);
                    g_mouse_y = (int)(event.motion.y * sy);
                }
                break;
            }
        }
        if (!running) break;

        /* LVGL tick */
        lv_tick_inc(16);
        lv_timer_handler();

        /* Update gauge + tick label every second */
        if (now - last_update >= 1000) {
            last_update = now;
            char buf[32];
            snprintf(buf, sizeof(buf), "Tick: %u", now);
            lv_label_set_text(g_tick_lbl, buf);

            gauge_val = (gauge_val + 7) % 101;
            lv_arc_set_value(g_gauge, (int32_t)gauge_val);
        }

        SDL_Delay(16); /* ~60 FPS */
    }

    /* ---- Cleanup ---- */
    free(g_buf1);
    if (g_texture) SDL_DestroyTexture(g_texture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();

    fprintf(stderr, "[Demo] Exited.\n");
    return 0;
}
