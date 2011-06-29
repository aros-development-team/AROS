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

/*
 * This is executed after all platform-specific initialization is done.
 * It will add "EMU: device to boot up from.
 */
static LONG automount(struct emulbase *emulbase)
{
    struct Library *ExpansionBase;
    struct DeviceNode *dlv = NULL;

    ExpansionBase = OpenLibrary("expansion.library",0);
    if (ExpansionBase)
    {
        IPTR pp[4 + sizeof(struct DosEnvec)/sizeof(IPTR)] = {};

        D(bug("[Emulhandler] startup: got ExpansionBase\n"));

        pp[0] 		      = (IPTR)"EMU";
        pp[1]		      = 0;
        pp[2]		      = 0;
        pp[DE_TABLESIZE  + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK  + 4] = 128;
        /* .... */
        pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
        pp[DE_MASK       + 4] = 0x7FFFFFFE;
        pp[DE_BOOTPRI    + 4] = 0;
        pp[DE_DOSTYPE    + 4] = AROS_MAKE_ID('E','M','U','L');

        dlv = MakeDosNode(pp);

        if (dlv)
        {
            dlv->dn_SegList = CreateSegList(EmulHandler_work);
            D(bug("[Emulhandler] startup allocated dlv %p, handler %p\n", dlv, dlv->dn_SegList));
            AddBootNode(dlv->dn_Priority, 0, dlv, NULL);
        }
    }

    CloseLibrary(ExpansionBase);

    return dlv ? TRUE : FALSE;
}

ADD2INITLIB(automount, 10)

/*********************************************************************************************/

static LONG cleanup(struct emulbase *emulbase)
{
    if (emulbase->mempool)
    	DeletePool(emulbase->mempool);

    return TRUE;
}

ADD2EXPUNGELIB(cleanup, 10);
