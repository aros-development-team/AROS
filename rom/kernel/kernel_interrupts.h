/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
    uint8_t             in_type;
    uint8_t             in_nr;
};

enum intr_types
{
    it_exception = 0xe0,
    it_interrupt = 0xf0
};

/* Functions to be called by machine-specific code */
int krnRunExceptionHandlers(struct KernelBase *KernelBase, uint8_t exception, void *ctx); /* Run user-supplied exception handlers */
void krnRunIRQHandlers(struct KernelBase *KernelBase, uint8_t exception);		   /* Run user-supplied IRQ handlers       */
