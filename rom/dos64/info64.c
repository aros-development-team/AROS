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

        The block starts with a standard InfoData; the 64-bit block
        counts are in id_NumBlocks64 and id_NumBlocksUsed64.

        If the filesystem does not support the 64-bit query the
        request is delegated to the 32-bit Info(), and the 64-bit
        counts are taken from ACTION_QUERY_ATTR (FQA_NumBlocks,
        FQA_NumBlocksUsed) when the filesystem answers it, otherwise
        from the 32-bit counts.

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
    struct MsgPort *port = fl ? fl->fl_Task : GetFileSysTask();
    SIPTR err = 0;
    QUAD count;
    LONG ret;

#if (__WORDSIZE == 64)
    ret = dos64_SendPkt(DOS64Base, port, ACTION_INFO64, (SIPTR)lock,
                        (SIPTR)MKBADDR(parameterBlock), 0, 0, 0, &err);
    if (ret)
        return ret;
    if (!dos64_UnsupportedAction(err))
        return DOSFALSE;
#endif

    /* The block starts with a standard InfoData, so Info() can fill it */
    ret = Info(lock, (struct InfoData *)parameterBlock);
    if (ret)
    {
        parameterBlock->id_NumBlocks64     = (UQUAD)(ULONG)parameterBlock->id_NumBlocks;
        parameterBlock->id_NumBlocksUsed64 = (UQUAD)(ULONG)parameterBlock->id_NumBlocksUsed;

        /*
         * Filesystems that exceed the 32-bit block counts report the full
         * counts through the MorphOS style attribute query.
         */
        count = 0;
        if (dos64_PktOK(dos64_SendPkt(DOS64Base, port, ACTION_QUERY_ATTR, FQA_NumBlocks,
                                      (SIPTR)&count, sizeof(count), 0, 0, &err), err))
            parameterBlock->id_NumBlocks64 = (UQUAD)count;
        count = 0;
        if (dos64_PktOK(dos64_SendPkt(DOS64Base, port, ACTION_QUERY_ATTR, FQA_NumBlocksUsed,
                                      (SIPTR)&count, sizeof(count), 0, 0, &err), err))
            parameterBlock->id_NumBlocksUsed64 = (UQUAD)count;
        SetIoErr(0);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* Info64 */
