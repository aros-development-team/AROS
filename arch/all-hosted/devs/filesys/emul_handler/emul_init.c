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
#include <libraries/expansion.h>
#include <resources/filesysres.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "emul_intern.h"
#include LC_LIBDEFS_FILE

#include <limits.h>
#include <string.h>
#include <stddef.h>

static LONG startup(struct emulbase *emulbase)
{
    D(bug("[Emulhandler] startup\n"));

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[EmulHandler] got hostlib.resource %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    KernelBase = OpenResource("kernel.resource");
    D(bug("[EmulHandler] KernelBase = %p\n", KernelBase));
    if (!KernelBase)
	return FALSE;

    emulbase->mempool = CreatePool(MEMF_ANY|MEMF_SEM_PROTECTED, 4096, 2000);
    if (emulbase->mempool)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(startup, -10)

/*********************************************************************************************/

static LONG cleanup(struct emulbase *emulbase)
{
    if (emulbase->mempool)
    	DeletePool(emulbase->mempool);

    return TRUE;
}

ADD2EXPUNGELIB(cleanup, 10);
