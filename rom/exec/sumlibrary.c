/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: Build checksum for a library.
*/
#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, SumLibrary,

/*  SYNOPSIS */
        AROS_LHA(struct Library *, library,A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 71, Exec)

/*  FUNCTION
        Builds the checksum over a given library's jumptable and either puts
        it into the library->lib_Sum field (if the library is marked as changed)
        or compares it with this field and Alert()s at mismatch.

    INPUTS
        library - Pointer to library structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AddLibrary(), RemLibrary(), MakeLibrary(), MakeFunctions(), InitStruct()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE oldflags;
    ULONG sum;

    /* Arbitrate for library base */
    Forbid();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&library->lib_SpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif

    /*
        If the library checksumming is already in progress or if the
        checksum is unused skip this part
    */
    if(library->lib_Flags&LIBF_SUMUSED&&!(library->lib_Flags&LIBF_SUMMING))
    {
        ULONG *lp;

        /* Memorize library flags */
        oldflags=library->lib_Flags;
        library->lib_Flags&=~LIBF_CHANGED;

        /*
         * Keep the lock across the sum: releasing it would let another
         * core expunge the library while we read its jumptable.
         */

        /* Build checksum. Note: library bases are LONG aligned */
        sum=0;
        /* Get start of jumptable */
        lp=(ULONG *)((UBYTE *)library+library->lib_NegSize);
        /* And sum it up */
        while(lp<(ULONG *)library)
            sum+=*lp++;

        /*
            Alert() if the library wasn't marked as changed and if the
            checksum mismatches.
        */
        if(!(oldflags&LIBF_CHANGED)&&library->lib_Sum!=sum)
            Alert(AT_DeadEnd|AN_LibChkSum);

        /* Set new checksum */
        library->lib_Sum=sum;
    }

#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&library->lib_SpinLock);
#endif
    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* SumLibrary */

