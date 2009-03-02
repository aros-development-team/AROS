/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    GetBootInfo() function.
*/
#define DEBUG 0

#include "bootloader_intern.h"
#include <proto/bootloader.h>
#include <proto/utility.h>
#include <aros/debug.h>
#include <aros/bootloader.h>

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
 	case BL_LoaderName:
	    return BootLoaderBase->LdrName;
	case BL_Args:
	    D(bug("[BootLdr] BL_ARGS requested\n"));
	    if (BootLoaderBase->Flags & BL_FLAGS_CMDLINE)
		return (APTR)&(BootLoaderBase->Args);
	    break;
	default:
	    D(bug("[BootLdr] GetBootInfo: Unknown info requested\n"));
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* GetBootInfo */
