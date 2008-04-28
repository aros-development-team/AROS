#ifndef IRQ_H_
#define IRQ_H_

#include <inttypes.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <exec/execbase.h>

#include <oop/oop.h>

#include <hidd/irq.h>

/***** IRQ system HIDD *******************/

/* IDs */
#define IID_Hidd_IRQ        "hidd.bus.irq"
#define CLID_Hidd_IRQ       "hidd.bus.irq"

/* misc */

struct irq_staticdata
{
    OOP_Class           *irqclass;

    struct List         irqlist[16];
    ULONG               transtable[16];
};

struct irqbase
{
    struct Library          library;
    struct irq_staticdata   isd;
};

#define ISD(cl) (&((struct irqbase *)cl->UserData)->isd)

#endif /*IRQ_H_*/
