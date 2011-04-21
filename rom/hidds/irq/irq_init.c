#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>

#include "irq_intern.h"
#include LC_LIBDEFS_FILE

static int Irq_Init(LIBBASETYPEPTR LIBBASE)
{
    struct irq_staticdata *isd = &LIBBASE->isd;

    D(bug("[IRQ] Initializing\n"));

    isd->kernelBase = OpenResource("kernel.resource");
    if (!isd->kernelBase)
    	return FALSE;

    /* Initialize emulated HwInfo */
    isd->hwinfo.sysBase = SysBase;

    D(bug("[IRQ] Init OK\n"));

    return TRUE;
}

ADD2INITLIB(Irq_Init, 0)
