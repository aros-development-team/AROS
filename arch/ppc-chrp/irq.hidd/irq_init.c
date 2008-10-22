#define DEBUG 0

#include <inttypes.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "irq.h"
#include LC_LIBDEFS_FILE

static int Irq_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    struct irq_staticdata *isd = &LIBBASE->isd;

    D(bug("[IRQ] IRQ: Initializing\n"));

    D(bug("[IRQ]     Init OK\n"));
    return TRUE;
}

ADD2INITLIB(Irq_Init, 0)
