/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
*/
#include <proto/exec.h>
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

        AROS_LH2(UBYTE *, CreateArgstring,

/*  SYNOPSIS */
        AROS_LHA(CONST UBYTE *, string, A0),
        AROS_LHA(ULONG  , length, D0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 21, RexxSys)

/*  FUNCTION
        Allocates a Rexx argument string and copies exactly length bytes from
        string. The copied data is terminated with a NUL byte; its length
        is stored separately.

    INPUTS
        string - Source buffer containing at least length readable bytes.
        length - Number of bytes to copy, from 0 through 65535.

    RESULT
        Pointer to the copied argument data, or NULL if allocation fails.

    NOTES
        The caller owns the returned argument string and must release it
        with DeleteArgstring(). Treat the returned data as read-only.
        Embedded NUL bytes count toward the stored length. Use
        LengthArgstring() rather than strlen() to obtain that length.

    BUGS
        The function does not reject lengths above 65535, although the
        RexxArg length field is only 16 bits. LengthArgstring() cannot
        report such lengths correctly.

    SEE ALSO
        DeleteArgstring(), LengthArgstring()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* size is the length of the memory to allocate: size of RexxArg without Buff + length of string + 1 */
    ULONG size = sizeof(struct RexxArg) - 8 + length + 1;
    struct RexxArg *ra = (struct RexxArg *)AllocMem(size, MEMF_PUBLIC|MEMF_CLEAR);
    ULONG hash = 0;
    int i;
    
    if (ra == NULL) ReturnPtr("CreateArgstring", UBYTE *, NULL);
  
    ra->ra_Size = size;
    ra->ra_Length = length;
    /* FIXME: Maybe the next two fields only need to be intialized on m68k? */
    /* Initialize the deprecated fields to a sane value for compatibility under AmigaOS */
    ra->ra_Deprecated1 = 1<<1 | 1<<2 | 1<<6; /* Was ra_Flags = NSF_ALPHA | NSF_EXT */
    for (i=0; i<length; i++)
        hash += string[i];
    ra->ra_Deprecated2 = (UBYTE)(hash & 255); /* Was ra_Hash */
    CopyMem(string, ra->ra_Buff, length);
    *(ra->ra_Buff + length) = '\0';
    
    ReturnPtrReg("CreateArgString", UBYTE *, ra->ra_Buff, "a0");
    AROS_LIBFUNC_EXIT
} /* CreateArgstring */
