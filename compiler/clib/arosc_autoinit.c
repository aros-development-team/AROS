/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <aros/symbolsets.h>
#include "arosc_init.h"

static AROS_SET_LIBFUNC(__arosc_libopen, struct Library *, aroscbase)
{
    return arosc_internalinit();
}

static AROS_SET_LIBFUNC(__arosc_libclose, struct Library *, aroscbase)
{
    arosc_internalexit();

    return 1;
}

ADD2OPENLIB(__arosc_libopen, 0);
ADD2CLOSELIB(__arosc_libclose, 0);

ADD2LIBS("arosc.library", 39, struct Library *, aroscbase);
