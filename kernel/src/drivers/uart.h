#ifndef MINIMUM_UART_H
#define MINIMUM_UART_H

#include <stdint.h>

/* Output (polled) */
void uart_putc(char c);
void uart_puts(const char *s);
void kprintf(const char *fmt, ...);

/* Input (interrupt-driven) */
void uart_rx_irq_init(void);    /* register handler, enable UART0 RX IRQ */
int uart_getc(void);            /* block until a byte arrives, return it */
uint32_t uart_rx_dropped(void); /* bytes lost because the buffer was full */

#endif
