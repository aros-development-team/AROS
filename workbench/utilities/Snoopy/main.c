/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/arossupport.h>

#include "main.h"
#include "gui.h"
#include "patches.h"
#include "setup.h"

int main(void)
{
    setup_init();
    gui_init();
    patches_init();
    gui_handleevents();
    clean_exit(NULL);
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
    STRPTR name = SysBase->ThisTask->tc_Node.ln_Name;
    
    if (setup.onlyShowFails && result) return;
    if (setup.ignoreWB)
    {
	if ( ! stricmp(name, "wanderer:wanderer")) return;
	if ( ! stricmp(name, "new shell")) return;
	if ( ! stricmp(name, "newshell")) return;
	if ( ! stricmp(name, "boot shell")) return;
 	if ( ! stricmp(name, "background cli")) return;
    }
    
    RawPutChars("SNOOP ", 7);
    prettyprint(name, setup.nameLen);
    prettyprint(action, setup.actionLen);
    prettyprint(target, setup.targetLen);
    prettyprint(option, setup.optionLen);
    prettyprint(result ? "OK" : "Fail", 0);
    RawPutChar('\n');
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

