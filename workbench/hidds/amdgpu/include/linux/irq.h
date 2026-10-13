/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_IRQ_H_
#define _LINUX_IRQ_H_

#include <linux/types.h>
#include <linux/interrupt.h>

typedef unsigned long irq_hw_number_t;

struct irq_data;
struct irq_desc;
typedef void (*irq_flow_handler_t)(struct irq_desc *desc);

struct irq_chip {
    const char *name;
    void (*irq_mask)(struct irq_data *data);
    void (*irq_unmask)(struct irq_data *data);
};

void irq_set_chip_and_handler(unsigned int irq, const struct irq_chip *chip, irq_flow_handler_t handle);
void handle_simple_irq(struct irq_desc *desc);

#endif
