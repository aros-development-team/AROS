/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <aros/startup.h>
#include <proto/dos.h>

#include "autoinit_intern.h"

char *_ProgramName = NULL;

void __initprogramname(struct ExecBase *SysBase)
{
    char *cmd = __get_command_name();

    _ProgramName = FilePart(cmd);

    __startup_entries_next();

    return;
}

ADD2SET(__initprogramname, PROGRAM_ENTRIES, -20);
