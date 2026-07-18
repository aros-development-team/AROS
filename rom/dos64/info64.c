/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Get 64-bit information about a volume.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2(LONG, Info64,

/*  SYNOPSIS */
        AROS_LHA(BPTR,               lock,           D1),
        AROS_LHA(struct InfoData64 *, parameterBlock, D2),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 16, Dos64)

/*  FUNCTION
        Get information about a volume in the system, with 64-bit
        block counts.

        If the filesystem does not support the 64-bit query the
        request is delegated to the 32-bit Info() and the result is
        widened; block counts are then limited to what the filesystem
        reports in 32 bits.

    INPUTS
        lock           - a lock on any file on the volume for which
                         information should be supplied, or 0 for the
                         volume of GetFileSysTask()
        parameterBlock - pointer to a longword-aligned InfoData64
                         structure

    RESULT
        Boolean indicating success or failure. If TRUE (success) the
        'parameterBlock' is filled with information on the volume.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Info(), <dos/dos.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileLock *fl = BADDR(lock);
    struct InfoData32 *id32;
    LONG ret;

#if (__WORDSIZE == 64)
    SIPTR err = 0;

    ret = dos64_SendPkt(DOS64Base, fl ? fl->fl_Task : GetFileSysTask(),
                        ACTION_INFO64, (SIPTR)lock,
                        (SIPTR)MKBADDR(parameterBlock), 0, 0, 0, &err);
    if (ret)
        return ret;
    if (!dos64_UnsupportedAction(err))
        return DOSFALSE;
#else
    (void)fl;
#endif

    id32 = AllocVec(sizeof(struct InfoData32), MEMF_PUBLIC | MEMF_CLEAR);
    if (id32 == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    ret = Info(lock, (struct InfoData *)id32);
    if (ret)
    {
        parameterBlock->id_NumSoftErrors  = id32->id_NumSoftErrors;
        parameterBlock->id_UnitNumber     = id32->id_UnitNumber;
        parameterBlock->id_DiskState      = id32->id_DiskState;
        parameterBlock->id_NumBlocks      = (UQUAD)(ULONG)id32->id_NumBlocks;
        parameterBlock->id_NumBlocksUsed  = (UQUAD)(ULONG)id32->id_NumBlocksUsed;
        parameterBlock->id_BytesPerBlock  = id32->id_BytesPerBlock;
        parameterBlock->id_DiskType       = id32->id_DiskType;
        parameterBlock->id_VolumeNode     = id32->id_VolumeNode;
        parameterBlock->id_InUse          = id32->id_InUse;
    }

    FreeVec(id32);
    return ret;

    AROS_LIBFUNC_EXIT
} /* Info64 */
