/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id: getbootinfo.c 29778 2008-10-16 19:40:21Z neil $

    GetBootInfo() function.
*/
#include <aros/debug.h>
#include <aros/bootloader.h>
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
	    return (APTR)&(BootLoaderBase->Args);
	default:
	    D(bug("[BootLdr] GetBootInfo: Unknown info requested\n"));
	    return NULL;
    }

    AROS_LIBFUNC_EXIT
} /* GetBootInfo */
