/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_IRQDOMAIN_H_
#define _LINUX_IRQDOMAIN_H_

#include <linux/types.h>
#include <linux/irq.h>

struct irq_domain;
struct fwnode_handle;

struct irq_domain_ops {
    int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
    void (*unmap)(struct irq_domain *d, unsigned int virq);
};

struct irq_domain *irq_domain_create_linear(struct fwnode_handle *fwnode, unsigned int size,
                                            const struct irq_domain_ops *ops, void *host_data);
void irq_domain_remove(struct irq_domain *d);
unsigned int irq_create_mapping(struct irq_domain *d, irq_hw_number_t hwirq);
unsigned int irq_find_mapping(struct irq_domain *d, irq_hw_number_t hwirq);
int generic_handle_domain_irq(struct irq_domain *d, unsigned int hwirq);

#endif
