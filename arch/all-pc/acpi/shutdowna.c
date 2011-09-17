/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: shutdowna.c 41386 2011-09-16 13:42:40Z sonic $

    Desc: ACPI replacement for exec.library/ShutdownA()
    Lang: english
*/

#include <aros/debug.h>
#include <exec/tasks.h>
#include <resources/acpi.h>
#include <proto/acpi.h>

AROS_LH1(ULONG, ShutdownA,
	 AROS_LHA(ULONG, action, D0),
	 struct ExecBase *, SysBase, 173, Acpi)
{
    AROS_LIBFUNC_INIT

    APTR ACPIBase = OpenResource("acpi.resource");
    struct ACPI_TABLE_TYPE_FADT *fadt = ACPI_FindSDT(ACPI_MAKE_ID('F','A','C','P'));

    D(bug("[ACPI.ShutdownA] FADT 0x%p\n", fadt));

    switch (action)
    {
    case SD_ACTION_COLDREBOOT:
	/* Use reset register */
        D(bug("[ACPI.ShutdownA] Reset register 0x%p, value 0x%02X\n", (IPTR)fadt->reset_reg.address, fadt->reset_value));
        ACPI_WriteReg(&fadt->reset_reg, fadt->reset_value);
        /* We really should not return from that */
	break;
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
