#include "lvgl_port.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"
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

    printf("[Complex] LVGL Widgets Demo starting...\n");

    if (lvgl_port_init(960, 540) != RTOS_OK) {
        fprintf(stderr, "[Complex] LVGL port init failed\n");
        return 1;
    }

    printf("[Complex] Creating widgets demo...\n");
    lv_demo_widgets();

    float scale = (float)lvgl_port_get_width() / 960;
    int mouse_down = 0;

    SDL_Event ev;
    printf("[Complex] Running. Use window or Ctrl+C to exit.\n");

    while (g_running) {
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
                lv_indev_state_t st = mouse_down ? LV_INDEV_STATE_PRESSED
                                                 : LV_INDEV_STATE_RELEASED;
                lvgl_port_update_mouse(st,
                                        ev.motion.x * scale, ev.motion.y * scale);
            }
        }

        if (!g_running) break;

        lv_tick_inc(16);
        lv_timer_handler();

        lvgl_port_present();
        SDL_Delay(16);
    }

    lvgl_port_cleanup();
    printf("[Complex] Demo exited.\n");
    return 0;
}
