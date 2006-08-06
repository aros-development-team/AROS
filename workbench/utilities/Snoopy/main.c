/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <stdio.h>
#include <stdlib.h>

#include <proto/dos.h>

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

void main_output(CONST_STRPTR action, CONST_STRPTR target, CONST_STRPTR option, LONG result)
{
    if (setup.onlyShowFails && result) return;

    if ( ! action) action = "          ";
    if ( ! target) target = "          ";
    if ( ! option) option = "     ";

    //FIXME: pretty printing, 'bug' doesn't allow something like %10s
    bug("SNOOP: %s | %s | %s | %s | %s\n", SysBase->ThisTask->tc_Node.ln_Name, action, target, option,
	    result ? "OK" : "Fail");
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

