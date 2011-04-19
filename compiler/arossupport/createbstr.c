#include <dos/bptr.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <string.h>

BSTR CreateBSTR(CONST_STRPTR src)
{
    STRPTR dst;
    ULONG len = AROS_BSTR_MEMSIZE4LEN(strlen(src));

    dst = AllocVec(len, MEMF_ANY);
    if (!dst)
	return BNULL;

#ifdef AROS_FAST_BSTR
    CopyMem(src, dst, len);
#else
    CopyMem(src, dst + 1, len);
    dst[0] = len;
#endif

    return MKBADDR(dst);
}
