#ifndef KEYPAD_H
#define KEYPAD_H
#include <stdint.h>

void keypad_init(void);
/* Returns LV_KEY code or 0 if no key pressed */
uint32_t keypad_read(void);

#endif
