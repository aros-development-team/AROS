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
    uint8_t             in_type;
    uint8_t             in_nr;
};

enum intr_types
{
    it_exception = 0xe0,
    it_interrupt = 0xf0
};

/* Interrupt controller definitions ... */

struct IntrInstance;

typedef UBYTE   icid_t;
typedef UWORD   icintrid_t;

#define ICINTR_ICID(icintr)     ((icintr >> 8) & 0xFF)
#define ICINTR_INST(icintr)     (icintr & 0xFF)

/*
 * Details:
 * 
 * .ln_Node = "Controller Type" Name - set by the "driver".
 * .ln_Type = icid_t - value filled in by/returned from krnAddInterruptController.
 * .ln_Pri = use count - value filled in by/returned from krnAddInterruptController.
*/
struct IntrController
{
    struct Node ic_Node;
    ULONG        ic_Type;                                                     /* IC drivers private "type"                 */
    ULONG        ic_Flags;
    APTR        ic_Private;
    icid_t      (*ic_Register)(struct KernelBase *);                          /* one time initialization called during Add */
    BOOL        (*ic_Init)(struct KernelBase *, icid_t);                      /*                                           */
    BOOL        (*ic_IntrEnable)(APTR, icid_t, icid_t);
    BOOL        (*ic_IntrDisable)(APTR, icid_t, icid_t);
    BOOL        (*ic_IntrAck)(APTR, icid_t, icid_t);
};

/*
 * Interrupt controller needs to re-enable
 * the interrupt after acknowledging/processing
 */
#define ICF_ACKENABLE   (1 << 0)

#define ICF_READY       (1 << 30)
#define ICF_DISABLED    (1 << 31)

static inline struct IntrController *krnGetInterruptController(struct KernelBase *KernelBase, icid_t icid)
{
    struct IntrController *intContr;
    ForeachNode(&KernelBase->kb_ICList, intContr)
    {
        if (intContr->ic_Node.ln_Type == icid)
        {
            return intContr;
        }
    }
    return NULL;
}

/* Functions to be called by machine-specific code */
int krnRunExceptionHandlers(struct KernelBase *, uint8_t, void *); /* Run user-supplied exception handlers */
icintrid_t krnAddInterruptController(struct KernelBase *, struct IntrController *);
struct IntrController *krnFindInterruptController(struct KernelBase *, ULONG);
int krnInitInterruptControllers(struct KernelBase *);
void krnRunIRQHandlers(struct KernelBase *, uint8_t);		   /* Run user-supplied IRQ handlers       */

#endif /* !KERNEL_INTERRUPTS_H */
