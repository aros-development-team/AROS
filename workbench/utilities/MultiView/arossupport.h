/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#ifndef _AROS

#include <exec/types.h>

extern ULONG *FindMethod(ULONG *methods, ULONG searchmethodid);
extern struct DTMethod *FindTriggerMethod(struct DTMethod *methods, STRPTR command, ULONG method);

#endif /* _AROS */

