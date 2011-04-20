#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>

#include "irq.h"
#include LC_LIBDEFS_FILE

static int Irq_Init(LIBBASETYPEPTR LIBBASE)
{
    struct irq_staticdata *isd = &LIBBASE->isd;

    D(bug("[IRQ] Initializing\n"));

    isd->irqAttrBase = OOP_ObtainAttrBase(IID_Hidd_IRQ);
    if (!isd->irqAttrBase)
    	return FALSE;

    isd->kernelBase = OpenResource("kernel.resource");
    if (!isd->kernelBase)
    	return FALSE;

    /* Initialize emulated HwInfo */
    isd->hwinfo.sysBase = SysBase;

    D(bug("[IRQ] Init OK\n"));

    return TRUE;
}

static int Irq_Cleanup(LIBBASETYPEPTR LIBBASE)
{
    struct irq_staticdata *isd = &LIBBASE->isd;

    if (isd->irqAttrBase)
    	OOP_ReleaseAttrBase(IID_Hidd_IRQ);

    return TRUE;
}

ADD2INITLIB(Irq_Init, 0)
ADD2EXPUNGELIB(Irq_Cleanup, 0)
