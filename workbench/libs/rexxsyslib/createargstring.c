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
	AROS_LHA(CONST UBYTE *, string, A0),
	AROS_LHA(ULONG  , length, D0),

/*  LOCATION */
	struct Library *, RexxSysBase, 21, RexxSys)

/*  FUNCTION
        This function will create a RexxArg structure and copy the supplied
        string into it.

    INPUTS
        string - String to copy into the RexxArg structure
        length - Length of the string to copy.

    RESULT
        Will return a pointer the string part of the allocated RexxArg
        structure.

    NOTES
        Pointer to the string returned by this function may be used as a
        null terminated C string but should be considered read-only.

    EXAMPLE

    BUGS

    SEE ALSO
        DeleteArgstring(), LengthArgstring()

    INTERNALS


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
    /* Initialize the depricated fields to a sane value for compatibility under AmigaOS */
    ra->ra_Depricated1 = 1<<1 | 1<<2 | 1<<6; /* Was ra_Flags = NSF_ALPHA | NSF_EXT */
    for (i=0; i<length; i++)
	hash += string[i];
    ra->ra_Depricated2 = (UBYTE)(hash & 255); /* Was ra_Hash */
    CopyMem(string, ra->ra_Buff, length);
    *(ra->ra_Buff + length) = '\0';
    
    ReturnPtr("CreateArgString", UBYTE *, ra->ra_Buff);
    AROS_LIBFUNC_EXIT
} /* CreateArgstring */
