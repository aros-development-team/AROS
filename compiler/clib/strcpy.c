/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/09/21 15:47:34  digulla
    Use Amiga types

    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>

UBYTE * StrCpy (UBYTE * dest, const UBYTE * src)
{
    UBYTE * ptr = dest;

    while ((*ptr++ = *src ++));

    return dest;
}
