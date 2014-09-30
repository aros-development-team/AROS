/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/bptr.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <string.h>

BSTR CreateBSTR(CONST_STRPTR src)
{
    STRPTR dst;
    ULONG len = strlen(src);
    ULONG size = AROS_BSTR_MEMSIZE4LEN(len);

    dst = AllocVec(size, MEMF_ANY);
    if (!dst)
	return BNULL;

#ifdef AROS_FAST_BSTR
    CopyMem(src, dst, size);
#else
    /* Our BSTR's are also NUL-terminated: len + 1 */
    CopyMem(src, dst + 1, len + 1);
    dst[0] = len;
#endif

    return MKBADDR(dst);
}
