/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

#include "nvdisk_intern.h"
#include <nvdisk_arch.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <libraries/nonvolatile.h>


/*****************************************************************************

    NAME */

    AROS_LH1(BOOL, MemInfoNVD,

/*  SYNOPSIS */

        AROS_LHA(struct NVInfo *, nvInfo, A0),

/*  LOCATION */

        struct Library *, nvdBase, 8, NVDisk)

/*  FUNCTION

    Retrieve the total and currently available nonvolatile storage capacity.

    INPUTS

    nvInfo  --  Pointer to an NVInfo structure that receives the capacity
                values on success.

    RESULT

    TRUE if the storage information was retrieved; FALSE otherwise.

    NOTES

    On success, nvi_MaxStorage and nvi_FreeStorage are set in bytes.
    The reported free capacity may reserve space for filesystem metadata.
    The caller must not rely on the output fields after a failure.

    EXAMPLE

    BUGS

    SEE ALSO

    nonvolatile.library/GetNVInfo()

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

    if (NVDisk_ArchActive(GPB(nvdBase)))
        return NVDisk_ArchInfo(GPB(nvdBase), nvInfo);

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
