#ifndef EXTSTRINGS_H
#define EXTSTRINGS_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include <string.h>

UBYTE capitalch(UBYTE, UBYTE);
LONG noCaseStrCmp(const char *, const char *, UBYTE, int);
LONG StrCmp(CONST_STRPTR, CONST_STRPTR);
ULONG StrLen(CONST_STRPTR);
void StrCpyToBstr(const char *, char *, int);
#define StrCpyFromBstr(src,dst) CopyMem(src+1,dst,src[0]); dst[(ULONG)src[0]]=0

#endif
