/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* cleanup.c -- here are all functions used before exiting program */

#include "Installer.h"
#include "execute.h"
#include "locale.h"
#include "gui.h"
#include "procedure.h"
#include "variables.h"
#include "cleanup.h"

/* External variables */
extern ScriptArg script;
extern InstallerPrefs preferences;
extern int error;


void free_script(ScriptArg *first)
{
    if (first != NULL)
    {
	free_script(first->cmd);
	free_script(first->next);
	FreeVec(first->arg);
	FreeVec(first);
    }
}

void cleanup()
{
    if (preferences.transcriptstream != NULL)
    {
	Close(preferences.transcriptstream);
    }

    free_script(script.cmd);
    free_varlist();
    deinit_gui();
    Locale_Deinitialize();
}

void end_malloc()
{
    end_alloc();
}

void end_alloc()
{
#ifdef DEBUG
    fprintf(stderr, "Couldn't allocate memory!\n");
#endif /* DEBUG */
    cleanup();
    exit(-1);
}

void outofmem(void * ptr)
{
    if (ptr == NULL)
    {
	error = OUTOFMEMORY;
	traperr("Out of memory!\n", NULL);
    }
}

