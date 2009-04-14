/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/libraries.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include "muiscreen_intern.h"

static int InitBase(LIBBASETYPEPTR LIBBASE)
{
    NEWLIST(&LIBBASE->clients);
    return TRUE;
}

ADD2INITLIB(InitBase, 0);
