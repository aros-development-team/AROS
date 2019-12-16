#ifndef X11_KEYTABLE_H
#define X11_KEYTABLE_H

/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "x11_types.h"

#define timeval sys_timeval
#include <X11/keysym.h>
#undef timeval

struct _keytable
{
    KeySym keysym;
    WORD   hiddqual;
    WORD   hiddcode;
};

#endif /* !X11_KEYTABLE_H */
