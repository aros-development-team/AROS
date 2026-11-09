/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Camd initialization code.
*/

#include <exec/types.h>
#include <exec/libraries.h>

#ifndef __amigaos4__
#  include <aros/symbolsets.h>
#endif

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "camd_intern.h"
#ifndef __amigaos4__
#  include LC_LIBDEFS_FILE
#  define DEBUG 1
#  include <aros/debug.h>



/****************************************************************************************/

/* A client that left an endpoint session or snapshot open still refers to
   CAMD's drivers and registry, so CAMD stays loaded. */
static int ExpungeCamd(struct CamdBase *CamdBase)
{
    if (EndpointClientsRemain(CamdBase))
        return FALSE;
    UninitCamd(CamdBase);
    return TRUE;
}

/* Counted per task, so that a task's endpoint objects can go with its last
   close of the library. */
static int OpenCamd(struct CamdBase *CamdBase)
{
    return TaskOpenedCamd(CamdBase);
}

static int CloseCamd(struct CamdBase *CamdBase)
{
    TaskClosedCamd(CamdBase);
    return TRUE;
}

ADD2INITLIB(InitCamd, 0);
ADD2OPENLIB(OpenCamd, 0);
ADD2CLOSELIB(CloseCamd, 0);
ADD2EXPUNGELIB(ExpungeCamd, 0);

#else
#  include "camd_aos4_init.c"
#endif
