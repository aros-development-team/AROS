/*
    (C) 2000 AROS - The Amiga Research OS
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


AROS_LH1(BOOL, MemInfo,

/*  SYNOPSIS */

	AROS_LHA(struct NVInfo *, nvInfo,  A0),

/*  LOCATION */

	struct Library *, nvdBase, 8, NVDisk)

/*  FUNCTION

    Return the information associated with the nonvolatile memory.

    INPUTS

    nvInfo  --  information structure to be filled by this function

    RESULT

    The 'nvInfo' structure will be filled with the appropriate values.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT    

    struct InfoData info;

    if(Info(GPB(nvdBase)->nvd_location, &info))
    {
	nvInfo->nvi_MaxStorage = info.id_NumBlocks*info.id_BytesPerBlock;
	nvInfo->nvi_FreeStorage =
	    (info.id_NumBlocks - info.id_NumBlocksUsed)*info.id_BytesPerBlock;
    }
    else
	return FALSE;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* MemInfo */
