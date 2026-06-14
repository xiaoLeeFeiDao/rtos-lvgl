#include "usart.h"
#include "stm32f1xx.h"
#include "bsp.h"

void usart1_init(uint32_t baud)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9 = TX: Alternate Function push-pull, 50MHz */
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0;
    /* PA10 = RX: Input floating */
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFF0FF) | 0x00000400;

    /* Baud rate = APB2_CLK / baud */
    USART1->BRR = bsp_clock_apb2() / baud;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void usart1_putc(char c)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

void usart1_puts(const char *s)
{
    while (*s) {
        usart1_putc(*s++);
    }
}

/* Redirect printf to USART1 */
int _write(int fd, char *buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) usart1_putc(buf[i]);
    return len;
}

/* ---- Bare-metal syscall stubs (required by newlib) ---- */
void _exit(int status) { (void)status; while(1); }

void *_sbrk(int incr)
{
    static char *heap_end = (char *)0x68000000;
    char *prev = heap_end;
    if (incr > 0) {
        /* Align to 8 bytes */
        uint32_t offset = (uint32_t)heap_end & 7;
        if (offset) heap_end += (8 - offset);
    }
    char *ret = heap_end;
    heap_end += incr;
    /* Limit: 1MB external SRAM */
    if (heap_end > (char *)0x68100000) {
        heap_end -= incr;
        return (void *)-1;
    }
    return ret;
}

int _close(int fd)  { (void)fd; return -1; }
int _fstat(int fd, void *buf) { (void)fd; (void)buf; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd;(void)ptr;(void)dir; return 0; }
int _read(int fd, char *buf, int len) { (void)fd;(void)buf;(void)len; return 0; }
