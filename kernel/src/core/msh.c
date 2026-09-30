#include <stdint.h>
#include "../drivers/uart.h"
#include "msh.h"

#define MSH_MAX_LINE 20u

static int word_eq(const char *w, uint32_t n, const char *lit) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (lit[i] == '\0' || lit[i] != w[i]) {
            return 0;
        }
    }
    return lit[i] == '\0';
}

static void put_n(const char *s, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        uart_putc(s[i]);
    }
}

static void msh_execute(const char *line, uint32_t len) {
    uint32_t i = 0;

    while (i < len && line[i] == ' ') {     /* skip leading spaces */
        i++;
    }
    if (i == len) {                         /* empty or all spaces */
        return;
    }

    uint32_t start = i;                     /* first word */
    while (i < len && line[i] != ' ') {
        i++;
    }
    uint32_t wlen = i - start;

    if (word_eq(&line[start], wlen, "echo")) {
        while (i < len && line[i] == ' ') { /* collapse separator spaces */
            i++;
        }
        put_n(&line[i], len - i);
        uart_putc('\n');
        return;
    }

    uart_puts("command not found: ");
    put_n(&line[start], wlen);
    uart_putc('\n');
}

void msh_run(void) {
    char line[MSH_MAX_LINE];
    uint32_t len = 0;   /* logical length; may exceed MSH_MAX_LINE */

    uart_puts("msh> ");
    for (;;) {
        int c = uart_getc();

        if (c == '\n') {
            if (len > MSH_MAX_LINE) {
                uart_puts("msh: line too long (max 20 bytes)\n");
            } else {
                msh_execute(line, len);
            }
            len = 0;
            uart_puts("msh> ");
        } else if (c == 0x08 || c == 0x7f) {
            if (len > 0) {                  /* ignore on empty line */
                len--;
            }
        } else if (c == '\r') {
            /* not a terminator; ignored */
        } else {
            if (len < MSH_MAX_LINE) {
                line[len] = (char)c;
            }
            len++;                          /* keep counting past the cap */
        }
    }
}
