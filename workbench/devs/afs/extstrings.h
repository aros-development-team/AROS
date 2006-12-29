#ifndef EXTSTRINGS_H
#define EXTSTRINGS_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"

#ifdef strlen
#undef strlen
#endif
#define strlen StrLen

UBYTE capitalch(UBYTE, UBYTE);
LONG noCaseStrCmp(char *, char *, UBYTE, int);
LONG StrCmp(STRPTR, STRPTR);
ULONG StrLen(STRPTR);
void StrCpyToBstr(char *, char *, int);
#define StrCpyFromBstr(src,dst) CopyMem(src+1,dst,src[0]); dst[(ULONG)src[0]]=0

#endif
