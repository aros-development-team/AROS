/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: BSTR helpers
    Lang: English
*/

#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#define  DEBUG  0
#include <aros/debug.h>

WORD isdosdeviceb(BSTR ss)
{
	UBYTE b;
	UBYTE *s = BADDR(ss);
	
	for (b = 0; b < s[0]; b++) {
		if (s[b + 1] == ':')
			return b;
	}
	return -1;
}
WORD isdosdevicec(CONST_STRPTR s)
{
	UBYTE b = 0;
	while (s[b]) {
		if (s[b] == ':')
			return b;
		b++;
	}
	return -1;
}

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
	UBYTE tmp[256];
	UBYTE *ss2 = BADDR(s2);
	LONG len = strlen(s1);
	if (len != ss2[0])
		return TRUE;
	memcpy(tmp, ss2 + 1, len);
	tmp[len] = 0;
	return stricmp(s1, tmp);
}
BOOL CMPNICBSTR(CONST_STRPTR s1, BSTR s2, UBYTE length)
{
	UBYTE tmp[256];
	UBYTE *ss2 = BADDR(s2);
	if (ss2[0] < length || strlen(s1) < length)
		return TRUE;
	memcpy(tmp, ss2 + 1, ss2[0]);
	tmp[ss2[0]] = 0;
	return strnicmp(s1, tmp, length);
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
