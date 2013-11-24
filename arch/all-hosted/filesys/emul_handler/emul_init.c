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
    APTR ExpansionBase;

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
    if (!emulbase->mempool)
        return FALSE;

    /* Create a BootNode so we can boot from this device */
    if ((ExpansionBase = OpenLibrary("expansion.library", 0)))
    {
        struct DeviceNode *dn;
        IPTR pp[4 + sizeof(struct DosEnvec)/sizeof(IPTR)] = {};

        pp[0] 		      = (IPTR)"EMU";
        pp[1]		      = 0;
        pp[2]		      = 0;
        pp[DE_TABLESIZE  + 4] = DE_DOSTYPE;
        /* .... */
        pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
        pp[DE_MASK       + 4] = -1;
        pp[DE_BOOTPRI    + 4] = 0;
        pp[DE_DOSTYPE    + 4] = AROS_MAKE_ID('E', 'M', 'U', 0);

        dn = MakeDosNode(pp);
        if (dn)
        {
            dn->dn_SegList = CreateSegList(EmulHandlerMain);
            dn->dn_Handler = AROS_CONST_BSTR("emul-handler");
            dn->dn_StackSize = 16384;
            dn->dn_GlobalVec = (BPTR)-1;

            AddDosNode(0, ADNF_STARTPROC, dn);
        }

        CloseLibrary(ExpansionBase);
    }

    return TRUE;
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
