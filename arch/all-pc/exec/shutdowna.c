/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the machine, PC version.
    Lang: english
*/

#include <aros/debug.h>
#include <asm/io.h>
#include <exec/tasks.h>
#include <hardware/efi/runtime.h>
#include <resources/acpi.h>
#include <proto/acpi.h>
#include <proto/dos.h>

#include "exec_util.h"

AROS_LH1(ULONG, ShutdownA,
	 AROS_LHA(ULONG, action, D0),
	 struct ExecBase *, SysBase, 173, Exec)
{
    AROS_LIBFUNC_INIT

    APTR ACPIBase = PD(SysBase).acpiBase;

    if (PD(SysBase).efiRT)
    {
    	/*
    	 * If the system has EFI firmware, use its runtime interface.
    	 * Port 0xFE may not work on such machines (Mac).
    	 */
    	IPTR efiAction;

    	switch (action)
    	{
    	case SD_ACTION_COLDREBOOT:
    	    /*
    	     * FIXME: Cold restart doesn't work here. ResetSystem() just returns.
    	     * I don't know why...
    	     * In GRUB it seems to work. I tried EFI_Reset_Warm, i tried Disable(),
    	     * Supervisor()... Nothing helped. Some Mac quirk?
    	     * UPD: ACPI method fails too. I verified the code flow, it really writes
    	     * the required value into required register... Still no luck...
    	     *							Sonic.
    	     */
    	    efiAction = EFI_Reset_Cold;
    	    break;

    	case SD_ACTION_POWEROFF:
    	    efiAction = EFI_Reset_Shutdown;
    	    break;

    	default:
    	    /* Unknown action */
    	    return 0;
    	}

	D(bug("[ShutdownA] Trying EFI action %ld...\n", efiAction));

    	PD(SysBase).efiRT->ResetSystem(efiAction, 0, 0, NULL);
    }

    if (ACPIBase)
    {
        /* No EFI (or survived somehow). Trying ACPI. */
	struct ACPI_TABLE_TYPE_FADT *fadt = ACPI_FindSDT(ACPI_MAKE_ID('F','A','C','P'));
	BOOL acpi20 = FALSE;

	D(bug("[ShutdownA] Trying ACPI, FADT 0x%p\n", fadt));

	if (fadt && fadt->header.length > offsetof(struct ACPI_TABLE_TYPE_FADT, reset_value))
	{
	    D(bug("[ShutdownA] ACPI >=2.0 detected\n"));
	    acpi20 = TRUE;
	}

        switch (action)
        {
        case SD_ACTION_COLDREBOOT:
	    /* Use reset register */
            if (acpi20 && (fadt->flags & FACP_FF_RESET_REG_SUP))
            {
            	D(bug("[ShutdownA] Reset register 0x%p, value 0x%02X\n", (IPTR)fadt->reset_reg.address, fadt->reset_value));

            	ACPI_WriteReg(&fadt->reset_reg, fadt->reset_value);
            	/* We really should not return from that */
            }
        }

        if (acpi20)
        {
            /* If we don't have AT keyboard controller, it won't reset us. Return. */
            if (!(fadt->pc_arch & FACP_PC_8042))
            {
            	D(bug("[ShutdownA] 8042 keyboard controller not present\n"));
            	return;
            }
        }
    }

    /* Nothing worked, poor-man fallback */
    if (action == SD_ACTION_COLDREBOOT)
    {
    	struct DosLibrary *DOSBase;

    	/*
    	 * Don't call reset callbacks because their action is not
    	 * recoverable.
    	 * On IntelMac port 0xFE doesn't work, so the function should
    	 * be able to return cleanly.
    	 *
        Exec_DoResetCallbacks((struct IntExecBase *)SysBase); */
        outb(0xFE, 0x64);

	/*
	 * Keyboard controller can be slow, so we need to wait for some time.
	 * If we don't do this, we'll can see "Unsupported action" error, immediately
	 * followed by a restart, which looks strange.
	 * we use dos.library/Delay() here for simplicity. It works from within tasks.
	 */
	DOSBase = (APTR)OpenLibrary("dos.library", 36);
	if (DOSBase)
	{
	    Delay(50);
	    CloseLibrary((struct Library *)DOSBase);
	}
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
