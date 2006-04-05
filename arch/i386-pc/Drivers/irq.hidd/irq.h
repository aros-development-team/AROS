#ifndef _HIDD_IRQ_H
#define _HIDD_IRQ_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the irq system HIDD.
    Lang: English.
*/

#include <asm/linkage.h>
//#include <asm/init.h>
#include <asm/irq.h>

#include <asm/segments.h>
#include <hidd/irq.h>
#include <exec/lists.h>

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <asm/ptrace.h>

/***** IRQ system HIDD *******************/

/* IDs */
#define IID_Hidd_IRQ        "hidd.bus.irq"
#define CLID_Hidd_IRQ       "hidd.bus.irq"

/* misc */

struct irq_staticdata
{
    OOP_Class		*irqclass;
    
    struct List		irqlist[16];    
    ULONG		transtable[16];
};

struct irqbase
{
    struct Library          library;
    struct ExecBase         *sysbase;
    BPTR                    seglist;
    struct irq_staticdata   isd;
};

void init_Servers  ( struct irq_staticdata * );

#define ISD(cl) (&((struct irqbase *)cl->UserData)->isd)

#endif /* _HIDD_IRQ_H */
