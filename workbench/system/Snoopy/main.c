/*
    Copyright © 2006-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>
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


// eye-catching function name because this is what we'd see in the debugger
static void SNOOPY_breakpoint(void)
{
    // interrupting makes only sense on "hosted" where we have a debugger
    #if (AROS_FLAVOUR & AROS_FLAVOUR_EMULATION)
	#if defined(__i386__) || defined(__x86_64__)
	    asm("int3");
	#elif defined(__powerpc__)
	    asm("trap");
	#else
	    // TODO: other platforms
	    kprintf("[SNOOP] interrupt not supported on this platform\n");
	#endif
    #endif
}


void main_output(CONST_STRPTR action, CONST_STRPTR target, CONST_STRPTR option,
    IPTR result, BOOL canInterrupt, BOOL expand)
{
    char pathbuf[MAX_STR_LEN+1];

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

    if (setup.match && ! MatchPatternNoCase(setup.parsedpattern, name))
    {
	// pattern doesn't fit
	return;
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

    if (setup.showPaths && target != NULL)
    {
        if (expand)
        {
            //Expand filename to full path
            target = MyNameFromLock(((struct Process *)thistask)->pr_CurrentDir,
                target, pathbuf, MAX_STR_LEN);
        }
    }
    prettyprint(target, setup.targetLen);

    prettyprint(option, setup.optionLen);
    prettyprint(result ? MSG(MSG_OK) : MSG(MSG_FAIL), 0);
    RawPutChar('\n');
    Permit();
    
    // We're calling the breakpoint function from the output function
    // so that we don't have to check the setup parameters again.
    // canInterrupt is for cases where a patch prints multiple lines.
    if (canInterrupt && setup.breakPoint) SNOOPY_breakpoint();
}


void main_parsepattern(void)
{
    setup.match = FALSE;

    if (!setup.pattern) return;
    if (setup.pattern[0] == '\0') return;

    Forbid(); // avoid that parsed pattern is used while we change it
    setup.parsedpattern[0] = '\0';
    if (ParsePatternNoCase(setup.pattern, setup.parsedpattern, PARSEDPATTERNLEN) != -1)
    {
	setup.match = TRUE;
    }
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

