/*
        Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
#include <clib/rexxsyslib_protos.h>

	AROS_LH2(UBYTE *, CreateArgstring,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, string, A0),
	AROS_LHA(ULONG  , length, D0),

/*  LOCATION */
	struct Library *, RexxSysBase, 21, RexxSys)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* size is the length of the memory to allocate: size of RexxArg without Buff + length of string + 1 */
    ULONG size = sizeof(struct RexxArg) - 8 + length + 1;
    struct RexxArg *ra = AllocMem(size, MEMF_PUBLIC|MEMF_CLEAR);
  
    if (ra == NULL) ReturnPtr("CreateArgstring", UBYTE *, NULL);
  
    ra->ra_Size = length + 1;
    ra->ra_Length = length;
    CopyMem(string, ra->ra_Buff, length);
    *(ra->ra_Buff + length) = '\0';
    
    ReturnPtr("CreateArgString", UBYTE *, ra->ra_Buff);
    AROS_LIBFUNC_EXIT
} /* CreateArgstring */
