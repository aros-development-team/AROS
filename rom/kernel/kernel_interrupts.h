#ifndef KERNEL_INTERRUPTS_H
#define KERNEL_INTERRUPTS_H
/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <inttypes.h>

/*
 * Internal definitions. Needed only if you replace some related function.
 */
struct IntrNode
{
    struct MinNode      in_Node;
    void                *in_Handler;
    void                *in_HandlerData;
    void                *in_HandlerData2;
    uint32_t             in_nr;
    uint8_t             in_type;
};

enum intr_types
{
    it_exception = 0xe0,
    it_interrupt = 0xf0
};

/* Functions to be called by machine-specific code */
int krnRunExceptionHandlers(struct KernelBase *, uint8_t, void *); /* Run user-supplied exception handlers */
void krnRunIRQHandlers(struct KernelBase *, uint8_t);		   /* Run user-supplied IRQ handlers       */

#endif /* !KERNEL_INTERRUPTS_H */
