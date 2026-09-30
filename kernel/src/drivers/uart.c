#include <stdarg.h>
#include <stdint.h>
#include "minemu/irq.h"
#include "minemu/platform.h"
#include "../core/irq_table.h"
#include "uart.h"

/* ---------- output ---------- */

void uart_putc(char c) {
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY)) {
    }
    MINEMU_UART0->tx_data = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

static void put_hex(uint32_t v) {
    const char *digits = "0123456789abcdef";
    int started = 0;
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t d = (v >> shift) & 0xf;
        if (d || started || shift == 0) {
            uart_putc(digits[d]);
            started = 1;
        }
    }
}

static void put_udec(uint32_t v) {
    static const uint32_t pow10[] = {
        1000000000u, 100000000u, 10000000u, 1000000u,
        100000u, 10000u, 1000u, 100u, 10u, 1u
    };
    int started = 0;
    for (int i = 0; i < 10; i++) {
        char d = '0';
        while (v >= pow10[i]) {
            v -= pow10[i];
            d++;
        }
        if (d != '0' || started || i == 9) {
            uart_putc(d);
            started = 1;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            uart_putc(*fmt);
            continue;
        }
        fmt++;
        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            uart_puts(s ? s : "(null)");
            break;
        }
        case 'c': uart_putc((char)va_arg(ap, int)); break;
        case 'u': put_udec(va_arg(ap, uint32_t)); break;
        case 'x': put_hex(va_arg(ap, uint32_t)); break;
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            if (v < 0) {
                uart_putc('-');
                put_udec((uint32_t)0 - (uint32_t)v);
            } else {
                put_udec((uint32_t)v);
            }
            break;
        }
        case '%': uart_putc('%'); break;
        case '\0': fmt--; break;
        default: uart_putc('%'); uart_putc(*fmt); break;
        }
    }
    va_end(ap);
}

/* ---------- input ----------
 * Ring buffer shared between the IRQ handler (producer) and uart_getc
 * (consumer). The handler runs with IRQs masked by hardware; the consumer
 * masks IRQs itself while touching the buffer.
 * head/tail count up forever; index = count & (SIZE - 1).
 */

#define RX_BUF_SIZE 64u /* must be a power of two */

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint32_t rx_head;    /* written only by the IRQ handler */
static volatile uint32_t rx_tail;    /* written only by uart_getc */
static volatile uint32_t rx_dropped_count;

static void uart_rx_irq_handler(void) {
    /* Drain everything, or the RX interrupt fires again right after EOI. */
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) {
        uint8_t byte = (uint8_t)MINEMU_UART0->rx_data;
        if (rx_head - rx_tail < RX_BUF_SIZE) {
            rx_buf[rx_head & (RX_BUF_SIZE - 1)] = byte;
            rx_head++;
        } else {
            rx_dropped_count++;
        }
    }
}

void uart_rx_irq_init(void) {
    irq_register(MINEMU_IRQ_UART0, uart_rx_irq_handler);
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    MINEMU_INTERRUPT->enable =
        MINEMU_INTERRUPT->enable | (UINT32_C(1) << MINEMU_IRQ_UART0);
}

int uart_getc(void) {
    for (;;) {
        minemu_irq_disable();
        if (rx_tail != rx_head) {
            uint8_t c = rx_buf[rx_tail & (RX_BUF_SIZE - 1)];
            rx_tail++;
            minemu_irq_enable();
            return c;
        }
        minemu_irq_enable();
        /* IRQs are open here, so a pending UART interrupt can fill the buffer. */
    }
}

uint32_t uart_rx_dropped(void) {
    minemu_irq_disable();
    uint32_t n = rx_dropped_count;
    minemu_irq_enable();
    return n;
}
