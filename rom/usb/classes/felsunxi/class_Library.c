/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include "felsunxi_intern.h"

static const STRPTR libname = MOD_NAME_STRING;

static int libInit(LIBBASETYPEPTR LIBBASE) {
    mybug(0,("FELSunxi libInit\n"));

    NEWLIST(&LIBBASE->device_list);
    InitSemaphore(&LIBBASE->device_listlock);

    return(TRUE);
}

static int libOpen(LIBBASETYPEPTR LIBBASE) {
    mybug(0,("FELSunxi libOpen\n"));
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR LIBBASE) {
    mybug(0,("FELSunxi libExpunge\n"));
    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2OPENLIB(libOpen, 0)
ADD2EXPUNGELIB(libExpunge, 0)
