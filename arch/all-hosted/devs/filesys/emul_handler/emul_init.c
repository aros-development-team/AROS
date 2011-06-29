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

/*
 * This is executed after all platform-specific initialization is done.
 * It will register the handler in FileSystem.resource and add "EMU: device to boot up from
 */
#define ID_EMUL_DISK  AROS_MAKE_ID('E','M','U',0);
#define HANDLER_STACK 16384

extern const char GM_UNIQUENAME(LibName)[];

static LONG automount(struct emulbase *emulbase)
{
    struct FileSysResource *fsr;
    struct Library *ExpansionBase;
    struct DeviceNode *dlv = NULL;
    BPTR seglist = CreateSegList(EmulHandler_work);
    BSTR handler;

    if (!seglist)
    	return FALSE;

    handler = CreateBSTR(GM_UNIQUENAME(LibName));

    /* Register ourselves in FileSystem.resource if available */
    fsr = OpenResource("FileSystem.resource");
    if (fsr)
    {
	struct FileSysEntry *fse = AllocMem(sizeof(struct FileSysEntry), MEMF_CLEAR);

	if (fse)
	{
	    fse->fse_DosType      = ID_EMUL_DISK;
	    fse->fse_Version      = (VERSION_NUMBER << 16) | REVISION_NUMBER;
	    fse->fse_PatchFlags   = FSEF_HANDLER|FSEF_STACKSIZE|FSEF_SEGLIST;
	    fse->fse_Handler      = handler;
	    fse->fse_StackSize    = HANDLER_STACK;
	    fse->fse_SegList      = seglist;
	    fse->fse_Node.ln_Name = AROS_BSTR_ADDR(fse->fse_Handler);

	    Enqueue(&fsr->fsr_FileSysEntries, &fse->fse_Node);
	}
	else
	    FreeMem(fse, sizeof(struct FileSysEntry));
    }

    ExpansionBase = OpenLibrary("expansion.library",0);
    if (ExpansionBase)
    {
        IPTR pp[4 + sizeof(struct DosEnvec)/sizeof(IPTR)] = {};

        D(bug("[Emulhandler] startup: got ExpansionBase\n"));

        pp[0] 		      = (IPTR)"EMU";
        pp[1]		      = 0;
        pp[2]		      = 0;
        pp[DE_TABLESIZE  + 4] = DE_DOSTYPE;
        pp[DE_SIZEBLOCK  + 4] = 128;
        /* .... */
        pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
        pp[DE_MASK       + 4] = -1;
        pp[DE_BOOTPRI    + 4] = 0;
        pp[DE_DOSTYPE    + 4] = ID_EMUL_DISK;

        dlv = MakeDosNode(pp);

        if (dlv)
        {
            dlv->dn_Handler   = handler;
            dlv->dn_StackSize = HANDLER_STACK;
            dlv->dn_SegList   = seglist;
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
