/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    GetBootInfo() function.
*/

#include <aros/debug.h>
#include <aros/bootloader.h>

#include "bootloader_intern.h"

/*****************************************************************************

    NAME */
#include <proto/bootloader.h>

	AROS_LH1(APTR, GetBootInfo,

/*  SYNOPSIS */
	AROS_LHA(ULONG, infoType, D0),

/*  LOCATION */
	struct BootLoaderBase *, BootLoaderBase, 1, Bootloader)

/*  FUNCTION
	Return information from the bootloader

    INPUTS
	infoType - The type of information requested. Valid types are:
	  BL_Video      (struct VesaInfo*) - VESA video mode data
	  BL_LoaderName (STRPTR)           - Name of bootloader
	  BL_Args	(struct List *)    - Kernel arguments. Data item is struct Node.
					     ln_Name points to argument text.
	  BL_Drives	(struct List *)	   - Legacy disk drives present in the system.
					     Data item is struct DriveInfoNode.

    RESULT
	Pointer to data or NULL if not supplied

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[BootLdr] GetBootInfo(%u)\n", infoType));
    switch (infoType)
    {
        case BL_LoaderName:
            return BootLoaderBase->LdrName;

	case BL_Args:
	    if (BootLoaderBase->Flags & BL_FLAGS_CMDLINE)
	        return &(BootLoaderBase->Args);
	    break;

	case BL_Video:
	    return BootLoaderBase->Vesa;

	case BL_Drives:
	    if (BootLoaderBase->Flags & BL_FLAGS_DRIVES)
		return (APTR)&(BootLoaderBase->DriveInfo);
	    break;
    }

    D(bug("[BootLdr] GetBootInfo: Unknown info requested\n"));
    return NULL;

    AROS_LIBFUNC_EXIT
} /* GetBootInfo */
