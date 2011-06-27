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

LONG CDVD_handler(struct ExecBase *SysBase);

void CDVD_work()
{
    CDVD_handler(SysBase);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR cdrombase)
{
    APTR DOSBase;
    int ret;
    BPTR seg;

    DOSBase = OpenLibrary("dos.library", 0);
    if (!DOSBase)
    	return FALSE;

    seg = CreateSegList(CDVD_work);
    if (seg == BNULL) {
    	CloseLibrary(DOSBase);
	return FALSE;
    }

    ret = AddSegment("cdrom.handler", seg, CMD_SYSTEM);

    CloseLibrary(DOSBase);

    return ret;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
