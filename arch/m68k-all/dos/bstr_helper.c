/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.

    Desc: BSTR helpers
    Lang: English
*/

#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#define  DEBUG  0
#include <aros/debug.h>

BSTR C2BSTR(CONST_STRPTR src)
{
	STRPTR dst;
	
	dst = AllocVec(strlen(src) + 2, MEMF_ANY);
	if (!dst)
		return 0;
	dst[0] = strlen(src);
	strcpy(dst + 1, src);
	return MKBADDR(dst);
}
char *BSTR2C(BSTR srcs)
{
	UBYTE *src = BADDR(srcs);
	char *dst;
	
	dst = AllocVec(src[0] + 1, MEMF_ANY);
	if (!dst)
		return NULL;
	memcpy (dst, src + 1, src[0]);
	dst[src[0]] = 0;
	return dst;
}

BOOL CMPBSTR(BSTR s1, BSTR s2)
{
	UBYTE *ss1 = BADDR(s1);
	UBYTE *ss2 = BADDR(s2);
	return memcmp(ss1, ss2, ss1[0] + 1);
}
BOOL CMPCBSTR(CONST_STRPTR s1, BSTR s2)
{
	UBYTE *ss2 = BADDR(s2);
	LONG len = strlen(s1);
	if (len != ss2[0])
		return TRUE;
	return memcmp(s1, ss2 + 1, len);
}

BOOL CMPICBSTR(CONST_STRPTR s1, BSTR s2)
{
	int length = strlen(s1);

	if (length != AROS_BSTR_strlen(s2))
	    return TRUE;

	return strnicmp(s1, AROS_BSTR_ADDR(s2), length);
}

BOOL CMPNICBSTR(CONST_STRPTR s1, BSTR s2, UBYTE length)
{
	if (AROS_BSTR_strlen(s2) < length || strlen(s1) < length)
		return TRUE;
	return strnicmp(s1, AROS_BSTR_ADDR(s2), length);
}

void BSTR2CINLINE(char *s)
{
	UBYTE len = s[0];
	memmove(s, s + 1, len);
	s[len] = 0;
}

void fixfib(struct FileInfoBlock *fib)
{
	BSTR2CINLINE(fib->fib_FileName);
	BSTR2CINLINE(fib->fib_Comment);
}
