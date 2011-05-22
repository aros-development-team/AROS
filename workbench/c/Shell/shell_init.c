/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Shell Resource
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/symbolsets.h>
#include <aros/shcommands.h>

THIS_PROGRAM_HANDLES_SYMBOLSETS
DEFINESET(SHCOMMANDS)

#include LC_LIBDEFS_FILE

#undef DOSBase
static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct shcommand *sh;
    APTR DOSBase;
    int pos;

    D(bug("[Shell] Init\n"));

    DOSBase = OpenLibrary("dos.library", 0);
    if ( DOSBase == NULL ) {
    	D(bug("[Shell] What? No dos.library?\n"));
    	return FALSE;
    }

    ForeachElementInSet(SETNAME(SHCOMMANDS), 1, pos, sh) {
    	BPTR seg = CreateSegList((APTR)sh->sh_Command);
    	if (seg != BNULL)
    	    AddSegment(sh->sh_Name, seg, CMD_INTERNAL);
    }

    CloseLibrary(DOSBase);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
