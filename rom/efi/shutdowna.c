/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: shutdowna.c 41386 2011-09-16 13:42:40Z sonic $

    Desc: EFI replacement for exec.library/ShutdownA()
    Lang: english
*/

#include <aros/debug.h>
#include <exec/tasks.h>
#include <resources/efi.h>
#include <proto/exec.h>

#include "efi_intern.h"

AROS_LH1(ULONG, ShutdownA,
	 AROS_LHA(ULONG, action, D0),
	 struct ExecBase *, SysBase, 173, Efi)
{
    AROS_LIBFUNC_INIT

    struct EFIBase *EFIBase = OpenResource("efi.resource");
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
    	 * UPD 2: On AspireOne ACPI code works fine. I tried to write a test resident
    	 * hook that resets the machine... Reset works before PCI class init, and fails
    	 * after it. 
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

    /* Use EFI runtime services to perform the action */
    EFIBase->Runtime->ResetSystem(efiAction, 0, 0, NULL);

    /* Shut up the compiler, we should never reach this. */
    return 0;

    AROS_LIBFUNC_EXIT
}
