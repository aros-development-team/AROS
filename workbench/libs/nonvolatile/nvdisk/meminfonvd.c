/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "nvdisk_intern.h"
#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>


AROS_LH1(BOOL, MemInfoNVD,

/*  SYNOPSIS */

	AROS_LHA(struct NVInfo *, nvInfo, A0),

/*  LOCATION */

	struct Library *, nvdBase, 8, NVDisk)

/*  FUNCTION

    Return the information associated with the nonvolatile memory.

    INPUTS

    nvInfo  --  information structure to be filled by this function

    RESULT

    The 'nvInfo' structure will be filled with the appropriate values. In
    case of failure, NULL will be returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    3 blocks of free space are not reported as free, as it may be strange
    for a user that queries the available storage memory, finds out that
    free memory exists but still gets errors when using StoreNV() as the
    memory was used for directory purposes.

    HISTORY

    November 2000,  SDuvan  --  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

    struct InfoData info;

    if(Info(GPB(nvdBase)->nvd_location, &info))
    {
	nvInfo->nvi_MaxStorage = info.id_NumBlocks*info.id_BytesPerBlock;

	/* 3 blocks are subtracted at it may be the case a new application wants
	   to store a new item. Then the 'appName' directory will take one block,
	   the new file will take at least one block and possibly a new directory
	   block may need to be created containing the 'appName' directory */

	if(info.id_NumBlocks < 3)
	    nvInfo->nvi_FreeStorage = 0;
	else
	{
	    nvInfo->nvi_FreeStorage =
		(info.id_NumBlocks - info.id_NumBlocksUsed - 3)*info.id_BytesPerBlock;
	}
    }
    else
	return FALSE;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* MemInfo */
