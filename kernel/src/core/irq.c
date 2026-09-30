#include <stdint.h>
#include "minemu/irq.h"
#include "minemu/platform.h"
#include "irq_table.h"

#define IRQ_SOURCE_COUNT 4u

static irq_handler_t handlers[IRQ_SOURCE_COUNT];

void irq_register(uint32_t source, irq_handler_t handler) {
    if (source < IRQ_SOURCE_COUNT) {
        handlers[source] = handler;
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {
    uint32_t source = (uint32_t)frame->exception_id;

    /* Spurious (MINEMU_IRQ_NONE): nothing is active, so no EOI. */
    if (source >= IRQ_SOURCE_COUNT) {
        return frame;
    }
    if (handlers[source]) {
        handlers[source]();
    }
    MINEMU_INTERRUPT->eoi = source;
    return frame;
}
