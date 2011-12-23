/*
    Copyright (C) 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    The shell program.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>	/* For Stricmp */

#include <aros/symbolsets.h>
#include <aros/shcommands.h>

#include "shellcommands_intern.h"

THIS_PROGRAM_HANDLES_SYMBOLSETS

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct shcommand *sh;
    APTR UtilityBase;
    int pos;

    D(bug("[ShellCommands] Init\n"));

    D(bug("[ShellCommands] shset = %p\n", SETNAME(SHCOMMANDS)));
    ForeachElementInSet(SETNAME(SHCOMMANDS), 1, pos, sh) {
    	D(bug("[ShellCommands] SYSTEM:%s\n", sh->sh_Name));
    }

    pos--;
    D(bug("[ShellCommands] %d commands\n", pos));

    if (pos <= 0)
    	return FALSE;

    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase == NULL) {
    	D(bug("[ShellCommands] Can't open utility.library?\n"));
    	return FALSE;
    }

    DOSBase = OpenLibrary("dos.library", 0);
    if ( DOSBase == NULL ) {
    	D(bug("[ShellCommands] What? No dos.library?\n"));
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    LIBBASE->sc_Commands = pos;
    LIBBASE->sc_Command = AllocMem(sizeof(LIBBASE->sc_Command[0])*pos, 0);
    if (LIBBASE->sc_Command == NULL) {
    	CloseLibrary(DOSBase);
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    ForeachElementInSet(SETNAME(SHCOMMANDS), 1, pos, sh) {
    	struct ShellCommandSeg *scs = &LIBBASE->sc_Command[pos-1];

    	scs->scs_Size = 0;	// Must be 0 to prevent UnLoadSeg
    	scs->scs_Next = 0;	// from killing us after CLI[1] exits
    	scs->scs_Name = sh->sh_Name;
    	__AROS_SET_FULLJMP(&scs->scs_Code, sh->sh_Command);
#ifdef __AROS_USE_FULLJMP
	CacheClearE(&scs->scs_Code, sizeof(struct FullJumpVec), CACRF_ClearI | CACRF_ClearD);
#endif
    	AddSegment(sh->sh_Name, MKBADDR(&scs->scs_Next), CMD_INTERNAL);
    	if (Stricmp(sh->sh_Name, "NewShell") == 0)
    	    AddSegment("NewCLI", MKBADDR(&scs->scs_Next), CMD_INTERNAL);
    }

    CloseLibrary(UtilityBase);

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0);
