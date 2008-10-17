#include <aros/system.h>
#include <windows.h>
#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <stddef.h>
#include <exec/lists.h>
#include "kernel_intern.h"

#define D(x)

/*
 * IRQs are used in a bit different way from native ports. They are used by hostthread.resource 
 * for passing data back from host-side threads to AROS processes. IRQ handlers don't get their
 * parameters from their handler node, instead parameters are passed from host thread which caused
 * the IRQ. It's up to IRQ handler how to handle them.
 * We may use different IRQs here, however we will always use IRQ 0 and this is the only availible
 * IRQ in the system. This mechanism is experimental and is subject to change.
 */

void user_irq_handler_2(uint8_t irq, void *data1, void *data2)
{
    struct KernelBase *KernelBase = *KernelBasePtr;

    D(printf("[IRQ] IRQ %lu, data1 0x%p, data2 0x%p\n", irq, data1, data2));
    D(printf("[IRQ] KernelBase = 0x%p\n", KernelBase));
    if (KernelBase) {
        if (!IsListEmpty(&KernelBase->kb_Interrupts[irq]))
        {
            struct IntrNode *in, *in2;

            ForeachNodeSafe(&KernelBase->kb_Interrupts[irq], in, in2)
            {
                if (in->in_Handler) {
                    D(printf("[IRQ] Calling handler 0x%08lX\n"));
                    in->in_Handler(data1, data2);
                }
            }
        }
    }
}

/*
 * This is the only function to be called by modules other than kernel.resource.
 * Currently it is used only by hostthread.dll for causing IRQs.
 * Probably in future it will go completely public.
 */

unsigned long __declspec(dllexport) CauseIRQ(unsigned char irq, void *data1, void *data2)
{
    unsigned long res;

    D(printf("[kernel IRQ] Causing IRQ %u, data1 = 0x%p, data2 = 0x%p\n", irq, data1, data2));
    D(printf("[kernel IRQ] Switcher thread ID 0x %lu\n", SwitcherId));
    res = PostThreadMessage(SwitcherId, MSG_IRQ_0+irq, (WPARAM)data1, (LPARAM)data2);
    D(printf("[kernel IRQ] Result: %ld\n", res));
    return res;
}
