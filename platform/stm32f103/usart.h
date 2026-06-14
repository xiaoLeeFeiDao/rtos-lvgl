#ifndef USART_H
#define USART_H
#include <stdint.h>

void usart1_init(uint32_t baud);
void usart1_putc(char c);
void usart1_puts(const char *s);

#endif
