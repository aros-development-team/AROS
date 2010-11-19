/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/symbolsets.h>

#include "shellcommands_intern.h"

THIS_PROGRAM_HANDLES_SYMBOLSETS
DEFINESET(SHCOMMANDS)

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct shcommand *sh;
    int pos;

    D(bug("[ShellCommands] shset = %p\n", SETNAME(SHCOMMANDS)));
    ForeachElementInSet(SETNAME(SHCOMMANDS), 1, pos, sh) {
    	D(bug("[ShellCommands] SYSTEM:%s\n", sh->sh_Name));
    }

    pos--;
    D(bug("[ShellCommands] %d commands\n", pos));

    if (pos <= 1)
    	return FALSE;

    D(bug("[ShellCommands] Init\n"));
    DOSBase = OpenLibrary("dos.library", 0);
    if ( DOSBase == NULL ) {
    	D(bug("[ShellCommands] What? No dos.library?\n"));
    	return FALSE;
    }

    LIBBASE->sc_Commands = pos;
    LIBBASE->sc_Command = AllocMem(sizeof(LIBBASE->sc_Command[0])*pos, 0);
    if (LIBBASE->sc_Command == NULL) {
    	CloseLibrary(DOSBase);
    	return FALSE;
    }

    ForeachElementInSet(SETNAME(SHCOMMANDS), 1, pos, sh) {
    	struct ShellCommand *sc = &LIBBASE->sc_Command[pos-1];

    	sc->scs_Size = AROS_ALIGN(sizeof(*sc))/sizeof(ULONG);
    	sc->scs_Next = 0;
    	sc->scs_Name = sh->sh_Name;
    	__AROS_SET_FULLJMP(&sc->scs_Code, sh->sh_Command);
    	AddSegment(sh->sh_Name, MKBADDR(&sc->scs_Next), 0);
    }

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
