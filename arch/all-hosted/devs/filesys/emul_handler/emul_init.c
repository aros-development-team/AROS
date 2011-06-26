/*
 Copyright  1995-2011, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying host OS filesystem.
 Lang: english
 */

/*********************************************************************************************/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <libraries/expansion.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/expansion.h>

#include "emul_intern.h"

#include <limits.h>
#include <string.h>
#include <stddef.h>

#define VOLNAME	    "System"

#ifdef AROS_FAST_BSTR
#define CONST_BSTR(x) ((BSTR)x)
#else
#define CONST_BSTR(x) (&(const struct { BYTE len; TEXT data[]; }){sizeof(x)-1, x})
#endif

static LONG startup(struct emulbase *emulbase)
{
    D(bug("[Emulhandler] startup\n"));

    emulbase->mempool = CreatePool(MEMF_ANY|MEMF_SEM_PROTECTED, 4096, 2000);
    if (emulbase->mempool)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(startup, 10)

/*********************************************************************************************/

static LONG cleanup(struct emulbase *emulbase)
{
    if (emulbase->mempool)
    	DeletePool(emulbase->mempool);

    return TRUE;
}

ADD2EXPUNGELIB(cleanup, 10);


