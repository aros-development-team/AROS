/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#include <proto/kernel.h>

/*
 * m68k virttophys implementation.
 * check if the address is in a rom image space
 * and provide the correct physical address
*/

AROS_LH1(void *, KrnVirtualToPhysical,
	AROS_LHA(void *, virtual, A0),
	struct KernelBase *, KernelBase, 20, Kernel)
{
    AROS_LIBFUNC_INIT

    /* if the platform has defined mappings for rom images, check if the address lies within each of the defined ranges...*/
    if (KernelBase->kb_PlatformData->romsize && KernelBase->kb_PlatformData->romimg)
    {
        APTR *curr_rom = KernelBase->kb_PlatformData->romimg;

        while (*curr_rom)
        {
            if ((curr_rom[0] <= virtual) && (curr_rom[0] + KernelBase->kb_PlatformData->romsize >= virtual))
                return curr_rom[1] + (virtual - curr_rom[0]);
            curr_rom = &curr_rom[2];
        }
    }
    return virtual;

    AROS_LIBFUNC_EXIT
}
