#ifndef IRQ_H_
#define IRQ_H_

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

extern const char IRQ_Table[];

struct irq_staticdata
{
    OOP_Class *irqclass;
    HIDDT_IRQ_HwInfo hwinfo;
    APTR kernelBase;
    OOP_AttrBase irqAttrBase;
};

struct irqbase
{
    struct Library          library;
    struct irq_staticdata   isd;
};

#define ISD(cl) (&((struct irqbase *)cl->UserData)->isd)

#endif /*IRQ_H_*/
