#include "bsp.h"
#include "lcd.h"
#include "keypad.h"
#include "usart.h"
#include "lvgl_port.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"

int main(void)
{
    bsp_clock_init();
    bsp_fsmc_gpio_init();
    usart1_init(9600);
    usart1_puts("[STM32] Widgets Demo\r\n");

    lcd_init();
    keypad_init();
    usart1_puts("[STM32] HW OK\r\n");

    if (lvgl_port_init() != RTOS_OK) {
        usart1_puts("LVGL FAIL\r\n");
        while (1);
    }
    usart1_puts("[STM32] LVGL OK\r\n");

    lv_demo_widgets();
    lv_obj_t *tv = lv_obj_get_child(lv_scr_act(), 0);
    int cur_tab = 0;
    usart1_puts("[STM32] Running\r\n");

    while (1) {
        lv_tick_inc(5);
        lv_timer_handler();
        lvgl_port_present();

        uint32_t key = keypad_read();
        if (key == LV_KEY_LEFT && tv) {
            cur_tab = cur_tab > 0 ? cur_tab - 1 : 2;
            lv_tabview_set_active(tv, cur_tab, LV_ANIM_ON);
        } else if (key == LV_KEY_RIGHT && tv) {
            cur_tab = (cur_tab + 1) % 3;
            lv_tabview_set_active(tv, cur_tab, LV_ANIM_ON);
        }

        for (volatile uint32_t d = 0; d < 50000; d++);
    }
}
