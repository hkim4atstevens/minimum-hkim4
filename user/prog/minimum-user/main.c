/*
 * msh: the minimum shell, running in user mode.
 *
 * All console I/O goes through ioctl() on UART0; the shell never touches
 * MMIO directly.
 *
 * Input is normalized as it is typed: leading spaces are dropped and runs
 * of spaces collapse to one, so the stored line is "WORD ARG ARG ...".
 * The command word and the argument text are each limited to MSH_MAX_LINE
 * (20) bytes; a line that exceeds either limit is discarded silently and
 * the prompt is shown again.
 */
#include <stdint.h>

#include "minemu/user_abi.h"

#define MSH_MAX_LINE 20

/* Room for "WORD ARGS " at their limits; anything longer is overlong anyway. */
#define MSH_BUF_SIZE (2 * MSH_MAX_LINE + 2)

/* ---------- console I/O ---------- */

static void con_write(const char *s, uint32_t n) {
    if (n != 0) {
        (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)s, n);
    }
}

static void con_puts(const char *s) {
    uint32_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    con_write(s, n);
}

static void con_putc(char c) {
    con_write(&c, 1);
}

/* Block (by polling) until one byte of input is available. */
static char con_getc(void) {
    char c;
    while (ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)&c, 1U) != 1) {
    }
    return c;
}

/* ---------- command execution ---------- */

static int word_eq(const char *w, uint32_t n, const char *lit) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (lit[i] == '\0' || lit[i] != w[i]) {
            return 0;
        }
    }
    return lit[i] == '\0';
}

/* line holds a normalized line: no leading spaces, no repeated spaces. */
static void msh_execute(const char *line, uint32_t len) {
    if (len > 0 && line[len - 1] == ' ') { /* at most one trailing space */
        len--;
    }
    if (len == 0) { /* empty or all-space line */
        return;
    }

    uint32_t wlen = 0;
    while (wlen < len && line[wlen] != ' ') {
        wlen++;
    }
    uint32_t args = (wlen < len) ? wlen + 1 : len; /* skip the single separator */
    uint32_t alen = len - args;

    if (wlen > MSH_MAX_LINE || alen > MSH_MAX_LINE) {
        return; /* overlong: discard */
    }

    if (word_eq(line, wlen, "echo")) {
        con_write(&line[args], alen);
        con_putc('\n');
        return;
    }

    con_puts("command not found: ");
    con_write(line, wlen);
    con_putc('\n');
}

/* ---------- line editor ---------- */

void minemu_user_main(void) {
    static char line[MSH_BUF_SIZE];

    for (;;) {
        uint32_t len = 0; /* bytes of normalized input; may exceed the buffer */

        con_puts("msh> ");
        for (;;) {
            char c = con_getc();

            if (c == '\n') {
                break;
            }
            if (c == '\r') {
                continue; /* not a terminator; ignored */
            }
            if (c == 0x08 || c == 0x7f) {
                if (len > 0) { /* ignore on empty line */
                    len--;
                }
                continue;
            }
            if (c == ' ') {
                /* Drop leading spaces and collapse repeats. Past the buffer we
                 * can't see the previous byte; the line is overlong anyway. */
                if (len == 0 || (len <= MSH_BUF_SIZE && line[len - 1] == ' ')) {
                    continue;
                }
            }
            if (len < MSH_BUF_SIZE) {
                line[len] = c;
            }
            len++; /* keep counting past the buffer so backspace stays in sync */
        }

        if (len > MSH_BUF_SIZE) {
            continue; /* overlong: discard */
        }
        msh_execute(line, len);
    }
}
