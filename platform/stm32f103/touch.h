#ifndef TOUCH_H
#define TOUCH_H
#include <stdint.h>

void touch_init(void);
int  touch_read(uint16_t *x, uint16_t *y);  /* returns 1 if pressed */

#endif
