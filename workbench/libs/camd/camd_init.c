/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

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

static int Expunge(struct CambBase *CamdBase)
{
    UninitCamd(CamdBase);
    return TRUE;
}

ADD2INITLIB(InitCamd, 0);
ADD2EXPUNGELIB(UninitCamd, 0);

#else
#  include "camd_aos4_init.c"
#endif
