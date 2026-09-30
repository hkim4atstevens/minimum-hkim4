#ifndef MINIMUM_IRQ_TABLE_H
#define MINIMUM_IRQ_TABLE_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_register(uint32_t source, irq_handler_t handler);

#endif
