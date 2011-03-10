/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include LC_LIBDEFS_FILE

extern void WorkbookMain(void);

#undef DOSBase
static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    APTR DOSBase;
    BPTR seg;

    D(bug("[Workbook] Init\n"));

    DOSBase = OpenLibrary("dos.library", 0);
    if ( DOSBase == NULL ) {
    	D(bug("[Workbook] What? No dos.library?\n"));
    	return FALSE;
    }

    seg = CreateSegList(WorkbookMain);
    if (seg != BNULL) {
    	AddSegment("Workbook", seg, CMD_INTERNAL);
    }

    CloseLibrary(DOSBase);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
