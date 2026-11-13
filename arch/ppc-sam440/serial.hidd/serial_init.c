 /*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/symbolsets.h>

#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

void serial_int(void *data, void *data2);

static inline BOOL GetIRQs(int *irq);
{
    struct Library *ProcessorBase = OpenResource(PROCESSORNAME);
    ULONG pvr = 0;

    if (ProcessorBase) {
        struct TagItem tags[] = {
            { GCIT_Model, (IPTR)&pvr },
            { TAG_END }
        };
        GetCPUInfo(tags);
    }

    switch (pvr) {
    case PVR_PPC460EX_B:
        irq[0] = INTR_UIC1_UART0;
        irq[1] = INTR_UIC0_UART1;
        irq[2] = INTR_UIC1_UART2;
        irq[3] = INTR_UIC1_UART3;
        break;
    case PVR_PPC440EP_B:
    case PVR_PPC440EP_C:
        /* ppc440 */
        irq[0] = INTR_U0;
        irq[1] = INTR_U1;
        irq[2] = INTR_U2;
        irq[3] = INTR_U3;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static int PPC4xxSer_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    struct class_static_data *csd = &LIBBASE->hdg_csd; /* SerialHidd static data */

    EnterFunc(bug("SerialHIDD_Init()\n"));

    if (GetIRQs(&csd->cs_IRQ[0])) {
        /* Install interrupts */
        for (i = 0; i < 4; i++)
            KrnAddHandler(csd->cs_IRQ[i], serial_int, csd->cs_Unit[i]);

        D(bug("  Got Interrupts\n"));
        ReturnInt("SerialHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("SerialHIDD_Init", ULONG, FALSE);
}

ADD2INITLIB(PPC4xxSer_Init, 0)
