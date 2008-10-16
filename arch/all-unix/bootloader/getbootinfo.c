/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    GetBootInfo() function.
*/
#include "bootloader_intern.h"
#include <proto/bootloader.h>
#include <proto/utility.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	AROS_LH1(APTR, GetBootInfo,

/*  SYNOPSIS */
	AROS_LHA(ULONG, infoType, D0),

/*  LOCATION */
	struct BootLoaderBase *, BootLoaderBase, 1, Bootloader)

/*  FUNCTION
	Return information from the bootloader

    INPUTS
	infoType - The type of information requestes

    RESULT
	Pointer to resource or NULL if failed

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    switch (infoType)
    {
	case BL_Args:
	    if (BootLoaderBase->Flags & MB_FLAGS_CMDLINE)
		return (APTR)&(BootLoaderBase->Args);
	    break;
	case BL_Drives:
	    if (BootLoaderBase->Flags & MB_FLAGS_DRIVES)
		return (APTR)&(BootLoaderBase->DriveInfo);
	    break;
	default:
	    bug("[BootLdr] GetBootInfo: Unknown info requested\n");
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* GetBootInfo */
