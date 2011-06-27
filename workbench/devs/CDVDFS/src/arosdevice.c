/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

LONG handler(struct ExecBase *SysBase);

void CDVDFS_work()
{
    handler(SysBase);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR cdrombase)
{
    APTR DOSBase;
    int ret;

    DOSBase = OpenLibrary("dos.library", 0);
    if (!DOSBase)
    	return FALSE;

    cdrombase->ab_SegList = CreateSegList(CDVDFS_work);
    if (cdrombase->ab_SegList == BNULL) {
    	CloseLibrary(DOSBase);
	return FALSE;
    }

    ret = AddSegment("cdrom.handler", cdrombase->ab_SegList, CMD_SYSTEM);

    CloseLibrary(DOSBase);

    return ret;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
