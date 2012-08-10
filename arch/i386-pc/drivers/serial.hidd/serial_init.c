 /*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

AROS_UFIP(serial_int_13);
AROS_UFIP(serial_int_24);

static int PCSer_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd; /* SerialHidd static data */
    struct Interrupt *irq;

    EnterFunc(bug("SerialHIDD_Init()\n"));

    /* Install COM1 and COM3 interrupt */
    irq = &csd->intHandler[0];
    irq->is_Node.ln_Name = "COM1/COM3";
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Pri=127;		/* Set the highest pri */
    irq->is_Code = (VOID_FUNC)serial_int_13;
    irq->is_Data = (APTR)csd;
    AddIntServer(INTB_KERNEL + 4, irq);

    /* Install COM2 and COM4 interrupt */
    irq = &csd->intHandler[1];
    irq->is_Node.ln_Name = "COM2/COM4";
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Pri=127;		/* Set the highest pri */
    irq->is_Code = (VOID_FUNC)serial_int_24;
    irq->is_Data = (APTR)csd;
    AddIntServer(INTB_KERNEL + 3, irq);

    D(bug("  Got Interrupts\n"));
    ReturnInt("SerialHIDD_Init", ULONG, TRUE);
}

ADD2INITLIB(PCSer_Init, 0)
