/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Installer V43.3
    Lang: english
*/

#define DEBUG 1
#include "Installer.h"
#include "main.h"
#include "cleanup.h"
#include "locale.h"
#include "gui.h"
#include "parse.h"
#include "variables.h"
#include "execute.h"

#include "version.h"


#ifdef DEBUG
char test_script[] = "SYS:Utilities/test.script";
#endif /* DEBUG */

char *filename = NULL;
BPTR inputfile;
char buffer[MAXARGSIZE];
int error = 0, grace_exit = 0;

InstallerPrefs preferences;
ScriptArg script;

IPTR args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
UBYTE **tooltypes;

/*
 * MAIN
 */
int main(int argc, char *argv[])
{
    struct RDArgs *rda = NULL;
    char *ttemp, *tstring;

    ScriptArg *currentarg, *dummy;
    int nextarg, endoffile, count;

    Locale_Initialize();

    if (argc != 0)
    { /* Invoked from Shell */
	preferences.fromcli = TRUE;
	/* evaluate args with RDArgs(); */
	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
	if (rda == NULL)
	{
	    PrintFault(IoErr(), INSTALLER_NAME);
	    exit(-1);
	}

	/* open script file */
	if (args[ARG_SCRIPT])
	{
	    filename = (STRPTR)args[ARG_SCRIPT];
	}
	else
	{
	    fprintf(stderr, "No SCRIPT specified!\n");
#ifdef DEBUG
	    fprintf(stderr, "Using %s instead...\n", test_script);
	    filename = test_script;
#else
	    FreeArgs(rda);
	    exit(-1);
#endif /* DEBUG */
	}
    }
    else
    { /* Invoked from Workbench */
	preferences.fromcli = FALSE;
	tooltypes = ArgArrayInit(argc, (UBYTE **)argv);

	/* open script file */
	ttemp = ArgString(tooltypes, "SCRIPT", NULL);
	if (ttemp == NULL)
	{
#ifdef DEBUG
	    fprintf(stderr, "No SCRIPT ToolType in Icon!\n");
	    ttemp = test_script;
#else
	    ArgArrayDone();
	    exit(-1);
#endif /* DEBUG */
	}
	filename = ttemp;
    }

    inputfile = Open(filename, MODE_OLDFILE);
    if (inputfile == NULL)
    {
#ifdef DEBUG
	fprintf(stderr, "Error opening scipt <%s>\n",filename);
	PrintFault(IoErr(), INSTALLER_NAME);
#endif /* DEBUG */
	exit(-1);
    }

    preferences.welcome = FALSE;
    preferences.transcriptstream = NULL;
    preferences.pretend = 0;

    if (argc)
    {
	preferences.debug = TRUE;
	if (args[ARG_NOLOG])
	{
	    preferences.novicelog = FALSE;
	}
	else
	{
	    preferences.novicelog = TRUE;
	}
	preferences.transcriptfile = StrDup((args[ARG_LOGFILE]) ? (char *)args[ARG_LOGFILE] : "install_log_file");
	preferences.nopretend = (int)args[ARG_NOPRETEND];
	if (args[ARG_MINUSER])
	{
	    preferences.minusrlevel = _NOVICE;
	    if (strcasecmp("average", (char *)args[ARG_MINUSER]) == 0)
	    {
		preferences.minusrlevel = _AVERAGE;
	    }
	    else if (strcasecmp("expert", (char *)args[ARG_MINUSER]) == 0)
	    {
		preferences.minusrlevel = _EXPERT;
	    }
	    else
	    {
		preferences.minusrlevel = _NOVICE;
	    }
	}
	else
	{
	    preferences.minusrlevel = _NOVICE;
	}
	if (args[ARG_DEFUSER])
	{
	    preferences.defusrlevel = preferences.minusrlevel;
	    if (strcasecmp("average", (char *)args[ARG_DEFUSER]) == 0)
	    {
		preferences.defusrlevel = _AVERAGE;
	    }
	    else if (strcasecmp("expert", (char *)args[ARG_DEFUSER]) == 0)
	    {
		preferences.defusrlevel = _EXPERT;
	    }
	    else
	    {
		preferences.defusrlevel = _NOVICE;
	    }
	}
	else
	{
	    preferences.defusrlevel = _NOVICE;
	}
	if (preferences.defusrlevel < preferences.minusrlevel)
	{
	    preferences.defusrlevel = preferences.minusrlevel;
	}
    }
    else
    {
	preferences.debug = TRUE;

	/* Create a log file in Novice mode? (TRUE) */
	if (strcmp("TRUE", ArgString(tooltypes, "LOG", "TRUE")) == 0)
	{
	    preferences.novicelog = TRUE;
	}
	else
	{
	    preferences.novicelog = FALSE;
	}

	/* Write to which LOGFILE? */
	preferences.transcriptfile = StrDup(ArgString(tooltypes, "LOGFILE", "install_log_file"));
	/* Is PRETEND possible? */
	preferences.nopretend = (strcmp("TRUE", ArgString(tooltypes, "PRETEND", "TRUE")) != 0);
	ttemp = ArgString(tooltypes, "MINUSER", "NOVICE");
	tstring = NULL;
	preferences.minusrlevel = _NOVICE;
	if (strcasecmp("average", ttemp) == 0)
	{
	    preferences.minusrlevel = _AVERAGE;
	    tstring = StrDup("AVERAGE");
	}
	else if (strcasecmp("expert", ttemp) == 0)
	{
	    preferences.minusrlevel = _EXPERT;
	    tstring = StrDup("EXPERT");
	}
	if (tstring == NULL)
	{
	    tstring = StrDup("NOVICE");
	}

	ttemp = ArgString(tooltypes, "DEFUSER", tstring);
	preferences.defusrlevel = preferences.minusrlevel;
	if (strcasecmp("average", ttemp) == 0)
	{
	    preferences.defusrlevel = _AVERAGE;
	}
	else if (strcasecmp("expert", ttemp) == 0)
	{
	    preferences.defusrlevel = _EXPERT;
	}
	if (preferences.defusrlevel < preferences.minusrlevel)
	{
	    preferences.defusrlevel = preferences.minusrlevel;
	}
	FreeVec(tstring);
	if (preferences.defusrlevel < preferences.minusrlevel)
	{
	    preferences.defusrlevel = preferences.minusrlevel;
	}
    }

    preferences.copyfail = COPY_FAIL;
    preferences.copyflags = 0;

    preferences.onerror.cmd = NULL;
    preferences.onerror.next = NULL;
    preferences.onerror.parent = NULL;
    preferences.onerrorparent = NULL;
    for ( count = 0 ; count < NUMERRORS ; count++ )
    {
	dummy = &(preferences.trap[count]);
	dummy->cmd = NULL;
	dummy->next = NULL;
	dummy->parent = NULL;
	preferences.trapparent[count] = NULL;
    }

    /* Init GUI -- i.e open empty window */
    init_gui();

    line = 1;

    endoffile = FALSE;
    script.arg = NULL;
    script.cmd = NULL;
    script.next = NULL;
    script.parent = NULL;
    script.intval = 0;
    script.ignore = 0;
    currentarg = script.cmd;
    /* parse script file */
    do
    {
	/* Allocate space for script cmd and save first one to scriptroot */
	if (script.cmd == NULL)
	{
	    script.cmd = (ScriptArg *)AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
	    if (script.cmd == NULL)
	    {
		end_alloc();
	    }
	    currentarg = script.cmd;
	    currentarg->parent = &script;
	}
	else
	{
	    currentarg->next = (ScriptArg *)AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
	    if (currentarg->next == NULL)
	    {
		end_alloc();
	    }
	    currentarg->next->parent = currentarg->parent;
	    currentarg = currentarg->next;
	}
	/* Set initial values */
	currentarg->arg = NULL;
	currentarg->cmd = NULL;
	currentarg->next = NULL;
	currentarg->intval = 0;
	currentarg->ignore = 0;

	nextarg = FALSE;
	do
	{
	    count = Read(inputfile, &buffer[0], 1);
	    if (count == 0)
	    {
		endoffile = TRUE;
	    }

	    if (!isspace(buffer[0]) && (endoffile == FALSE))
	    {
		/* This is text, is it valid ? */
		switch(buffer[0])
		{
		    case SEMICOLON: /* A comment, ok - Go on with next line */
			do
			{
			    count = Read(inputfile, &buffer[0], 1);
			} while (buffer[0] != LINEFEED && count != 0);
			line++;
			if (count == 0)
			{
			    endoffile = TRUE;
			}
			break;

		    case LBRACK: /* A command (...) , parse the content of braces */
			currentarg->cmd = (ScriptArg *)AllocVec(sizeof(ScriptArg), MEMF_PUBLIC);
			if (currentarg->cmd == NULL)
			{
			    end_alloc();
			}
			dummy = currentarg->cmd;
			dummy->parent = currentarg;
			dummy->arg = NULL;
			dummy->ignore = 0;
			dummy->intval = 0;
			dummy->cmd = NULL;
			dummy->next = NULL;
			parse_file(currentarg->cmd);
			nextarg = TRUE;
			break;

		    default: /* Plain text or closing bracket is not allowed */
			Close(inputfile);
			show_parseerror("Too many closing brackets!", line);
			cleanup();
			exit(-1);
			break;
		}
	    }
	    else
	    {
		if (buffer[0] == LINEFEED)
		{
		    line++;
		}
	    }
	} while (nextarg != TRUE && !endoffile);
    } while (!endoffile);

    /* Okay, we (again) have allocated one ScriptArg too much, so get rid of it */
    currentarg = script.cmd;
    if (currentarg->next != NULL)
    {
	while (currentarg->next->next != NULL)
	{
	    currentarg = currentarg->next;
	}
	FreeVec(currentarg->next);
	currentarg->next = NULL;
    }

    Close(inputfile);

    if (preferences.transcriptfile != NULL)
    {
	/* open transcript file */
	preferences.transcriptstream = Open(preferences.transcriptfile, MODE_NEWFILE);
	if (preferences.transcriptstream == NULL)
	{
	    PrintFault(IoErr(), INSTALLER_NAME);
	    cleanup();
	    exit(-1);
	}
    }

    /* Set variables which are not constant */
    set_preset_variables(argc);

    /* NOTE: Now everything from commandline(ReadArgs)/ToolTypes(Workbench)
	   will become invalid!
    */
    if (argc != 0)
    { /* Finally free ReadArgs struct (set_preset_variables() needed them) */
	FreeArgs(rda);
    }
    else
    { /* Or free tooltypes array if started from WB */
	ArgArrayDone();
    }

    /* If the script does not contain a (welcome) section, call the default one */
    if (preferences.welcome == FALSE)
    {
	request_userlevel(NULL);
    }


    /* Don't bother NOVICE */
    if (get_var_int("@user-level") == _NOVICE)
    {
	preferences.copyflags &= ~COPY_ASKUSER;
    }
    else
    {
	preferences.copyflags |= COPY_ASKUSER;
    }

    /* execute parsed script */
    execute_script(script.cmd, 0);

#ifdef DEBUG
    dump_varlist();
#endif /* DEBUG */

    final_report();
    cleanup();

return error;
}

