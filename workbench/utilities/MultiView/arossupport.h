/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef _AROS

#include <exec/types.h>

extern ULONG *FindMethod(ULONG *methods, ULONG searchmethodid);
extern struct DTMethod *FindTriggerMethod(struct DTMethod *methods, STRPTR command, ULONG method);

#endif /* _AROS */

