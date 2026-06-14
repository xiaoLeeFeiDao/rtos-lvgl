#include "lvgl_port.h"
#include "dashboard.h"
#include <SDL.h>
#include <stdio.h>
#include <signal.h>

static volatile int g_running = 1;
static void on_signal(int s) { (void)s; g_running = 0; }

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    printf("[Main] RTOS LVGL Demo starting...\n");

    if (lvgl_port_init(960, 540) != RTOS_OK) {
        fprintf(stderr, "[Main] LVGL port init failed\n");
        return 1;
    }

    /* Create dashboard UI */
    printf("[Main] Creating UI...\n");
    dashboard_create();

    float scale = (float)lvgl_port_get_width() / 960;

    /* ---- Single-threaded main loop (matches working standalone) ---- */
    SDL_Event ev;
    Uint32 last_sec = 0;
    int tick = 0;
    float gauge_val = 0;
    int mouse_down = 0;  /* track button state */

    printf("[Main] Running. Use window or Ctrl+C to exit.\n");

    while (g_running) {
        /* ---- SDL events (mouse / window) ---- */
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT ||
                (ev.type == SDL_WINDOWEVENT &&
                 ev.window.event == SDL_WINDOWEVENT_CLOSE))
                g_running = 0;

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                mouse_down = 1;
                lvgl_port_update_mouse(LV_INDEV_STATE_PRESSED,
                                        ev.button.x * scale, ev.button.y * scale);
            }
            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                mouse_down = 0;
                lvgl_port_update_mouse(LV_INDEV_STATE_RELEASED,
                                        ev.button.x * scale, ev.button.y * scale);
            }
            if (ev.type == SDL_MOUSEMOTION) {
                /* Keep current press state; only update coords */
                lv_indev_state_t st = mouse_down ? LV_INDEV_STATE_PRESSED
                                                 : LV_INDEV_STATE_RELEASED;
                lvgl_port_update_mouse(st,
                                        ev.motion.x * scale, ev.motion.y * scale);
            }
        }

        if (!g_running) break;

        /* ---- LVGL tick + render (main thread, like standalone) ---- */
        lv_tick_inc(16);
        lv_timer_handler();

        /* ---- Dashboard update every 1 second ---- */
        Uint32 now = SDL_GetTicks();
        if (now - last_sec >= 1000) {
            last_sec = now;
            dashboard_update(tick, (int)gauge_val);
            gauge_val += 7;
            if (gauge_val > 100) gauge_val -= 100;
            tick++;
        }

        /* ---- Present to screen ---- */
        lvgl_port_present();
        SDL_Delay(16);
    }

    lvgl_port_cleanup();
    printf("[Main] Demo exited.\n");
    return 0;
}
