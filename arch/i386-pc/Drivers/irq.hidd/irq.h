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
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
    OOP_Class		*irqclass;
    
    struct List		irqlist[16];    
    ULONG		transtable[16];
};

OOP_Class *init_irqclass  ( struct irq_staticdata * );
VOID free_irqclass  ( struct irq_staticdata * );
void init_Servers  ( struct irq_staticdata * );

#define ISD(cl) ((struct irq_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)ISD(cl)->oopbase)
#define UtilityBase	((struct Library *)ISD(cl)->utilitybase)
#define SysBase		(ISD(cl)->sysbase)

#endif /* _HIDD_IRQ_H */
