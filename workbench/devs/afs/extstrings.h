#ifndef EXTSTRINGS_H
#define EXTSTRINGS_H

#include <exec/types.h>

UBYTE capitalch(UBYTE, UBYTE);
LONG noCaseStrCmp(char *, char *, UBYTE);
LONG StrCmp(STRPTR, STRPTR);
ULONG StrLen(STRPTR);

#endif
