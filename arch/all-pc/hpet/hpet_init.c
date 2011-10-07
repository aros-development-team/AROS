#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <resources/acpi.h>
#include <utility/hooks.h>
#include <proto/acpi.h>
#include <proto/exec.h>

#include "hpet_intern.h"

AROS_UFH3(static BOOL, hpetEnumFunc,
	  AROS_UFHA(struct Hook *, hook, A0),
	  AROS_UFHA(struct ACPI_TABLE_TYPE_HPET *, table, A2),
	  AROS_UFHA(struct HPETBase *, base, A1))
{
    AROS_USERFUNC_INIT

    ULONG n;

    if (table->addr.address_space_id != ACPI_SPACE_MEM)
    	return FALSE;

    n = (table->id & HPET_NUM_COMPARATORS_MASK) >> HPET_NUM_COMPARATORS_SHIFT;

    if (base->units)
    {
    	IPTR blk = table->addr.address + 0x0100;
    	ULONG i;

    	for (i = 0; i < n; i++)
    	{
    	    base->units[base->unitCnt + i].base  = table->addr.address;
    	    base->units[base->unitCnt + i].block = blk;
    	    base->units[base->unitCnt + i].Owner = NULL;

    	    blk += 0x0020;
    	}
    }

    base->unitCnt += n;
    return TRUE;

    AROS_USERFUNC_EXIT
}

static const struct Hook enumHook =
{
    .h_Entry = (APTR)hpetEnumFunc
};

static int hpet_Init(struct HPETBase *base)
{
    struct ACPIBase *ACPIBase;

    ACPIBase = OpenResource("acpi.resource");
    if (!ACPIBase)
    	return FALSE;

    /* During the 1st pass base->units is NULL, so we will just count HPETs */
    ACPI_ScanSDT(ACPI_MAKE_ID('H','P','E','T'), &enumHook, base);

    D(bug("[HPET] %u units total\n", base->unitCnt));
    if (!base->unitCnt)
    	return FALSE;

    base->units = AllocMem(sizeof(struct HPETUnit) * base->unitCnt, MEMF_CLEAR);
    if (!base->units)
    	return FALSE;

    InitSemaphore(&base->lock);

    /* Fill in the data */
    base->unitCnt = 0;
    ACPI_ScanSDT(ACPI_MAKE_ID('H','P','E','T'), &enumHook, base);

    return TRUE;
}

ADD2INITLIB(hpet_Init, 0);
