#ifndef EXTSTRINGS_H
#define EXTSTRINGS_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "os.h"
#include <string.h>

/* helper typedef's to distinguish BSTR's originating on disk */
typedef char * FSBSTR;
typedef const char * CONST_FSBSTR;

UBYTE capitalch(UBYTE, UBYTE);
LONG noCaseStrCmp(const char *, CONST_FSBSTR, UBYTE, int);
ULONG StrLen(CONST_STRPTR);
void StrCpyToBstr(const char *, FSBSTR, int);
void StrCpyFromBstr(CONST_FSBSTR, char *, int);
#endif
