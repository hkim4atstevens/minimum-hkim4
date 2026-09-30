#include <stdarg.h>
#include <stdint.h>
#include "minemu/platform.h"
#include "uart.h"

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
