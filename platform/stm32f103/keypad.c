#include "keypad.h"
#include "stm32f1xx.h"
#include "lvgl.h"

/* PE2=KEY2, PE3=KEY1, PE4=KEY0 — already input from LCD FSMC init.
 * Just enable pull-ups here.
 */
static uint8_t key_prev_state = 0xFF;

void keypad_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPEEN;
    GPIOE->ODR |= (1<<2) | (1<<3) | (1<<4);  /* pull-ups */
}

uint32_t keypad_read(void)
{
    uint8_t raw = (GPIOE->IDR >> 2) & 0x07;
    uint8_t changed = raw ^ key_prev_state;
    uint8_t pressed = (~raw) & changed & 0x07;
    key_prev_state = raw;

    if (pressed & 0x01) return LV_KEY_LEFT;   /* KEY2=PE2 */
    if (pressed & 0x02) return LV_KEY_RIGHT;  /* KEY1=PE3 */
    if (pressed & 0x04) return LV_KEY_ENTER;  /* KEY0=PE4 */

    return 0;
}
