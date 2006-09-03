/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include "main.h"
#include "gui.h"
#include "patches.h"
#include "setup.h"
#include "locale.h"

int main(void)
{
	Locale_Initialize();
    setup_init();
    gui_init();
    patches_init();
    gui_handleevents();
    clean_exit(NULL);
	Locale_Deinitialize();
    return RETURN_OK;
}

static void prettyprint(CONST_STRPTR str, LONG minlen)
{
    if ( ! str) str = "";
    LONG len = strlen(str);
    RawPutChars("| ", 2);
    RawPutChars(str, len);
    LONG i;
    for (i = len ; i < minlen ; i++)
    {
	RawPutChar(' ');
    }
}


void main_output(CONST_STRPTR action, CONST_STRPTR target, CONST_STRPTR option, LONG result)
{
    struct Task *thistask = SysBase->ThisTask;
    STRPTR name = thistask->tc_Node.ln_Name;

    if (setup.onlyShowFails && result) return;
    if (setup.ignoreWB)
    {
	if ( ! stricmp(name, "wanderer:wanderer")) return;
	if ( ! stricmp(name, "new shell")) return;
	if ( ! stricmp(name, "newshell")) return;
	if ( ! stricmp(name, "boot shell")) return;
	if ( ! stricmp(name, "background cli")) return;
    }

    // FIXME: Can Forbid/Permit cause locks?
    Forbid();

    RawPutChars("SNOOP ", 7);

    prettyprint(name, setup.nameLen);

    if (setup.showCliNr)
    {
	int clinum = 0;
	if (thistask->tc_Node.ln_Type == NT_PROCESS && ((struct Process *)thistask)->pr_CLI)
	{
	    clinum = ((struct Process *)thistask)->pr_TaskNum;
	}

	kprintf(" [%d]", clinum);
    }

    prettyprint(action, setup.actionLen);
    prettyprint(target, setup.targetLen);
    prettyprint(option, setup.optionLen);
    prettyprint(result ? MSG(MSG_OK) : MSG(MSG_FAIL), 0);
    RawPutChar('\n');
    Permit();
}

void clean_exit(char *s)
{
    gui_cleanup();
    if (s)
    {
	puts(s);
	exit(RETURN_FAIL);

    }
    exit(RETURN_OK);
}

