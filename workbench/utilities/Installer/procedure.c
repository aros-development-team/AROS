/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* procedure.c -- Here are all functions related to user-defined procedures */

#include "Installer.h"
#include "execute.h"
#include "cleanup.h"
#include "procedure.h"

/* External variables */
extern struct CommandList internal_commands[];


int numusrprocs = 0, numactiveusrprocs = 0;
struct ProcedureList *usrprocs = NULL, **activeusrprocs = NULL;


/*
 * Activate previously parsed user function
 */
void link_function(char *name, long int incarnation)
{
int i = 0, j = 0, inc = -1;

    for ( ; i < numactiveusrprocs && strcmp(name, activeusrprocs[i]->procname) != 0 ; i++ );

    while (j < numusrprocs && inc != incarnation)
    {
	if (strcmp(name, usrprocs[j].procname) == 0)
	{
	    inc++;
	}
	j++;
    }
    j--;

    if (i == numactiveusrprocs)
    {
	numactiveusrprocs++;
	activeusrprocs = ReAllocVec(activeusrprocs, sizeof(struct ProcedureList *) * numactiveusrprocs, MEMF_PUBLIC);
	if (activeusrprocs == NULL)
	{
	    end_alloc();
	}
    }
    activeusrprocs[i] = &((usrprocs[j]));
}


/*
 * Return user's function
 */
struct ProcedureList *find_proc(char *name)
{
int i;

    /* Check if procedure is in list */
    for ( i = 0 ; i < numactiveusrprocs && strcmp(name, activeusrprocs[i]->procname) != 0 ; i++ );
    if (i == numactiveusrprocs)
    {
	/* Not in list */
	fprintf(stderr, "<%s> - Procedure not found!\n", name);
	return NULL;
    }

return activeusrprocs[i];
}


/*
 * Remember user functions at parse time
 */
long int set_procedure(char **args, int num, ScriptArg *cmd)
{
int i;
char *name;
long int incarnation = 0;

    name = args[0];
    /* Check if name is in preset list */
    for ( i = 0 ; i < _MAXCOMMAND && strcmp(name, internal_commands[i].cmdsymbol) != 0 ; i++ );
    if (i < _MAXCOMMAND)
    {
	fprintf(stderr, "Procedure name <%s> already defined for internal function!\n", name);
	cleanup();
	exit(-1);
    }

    /* Check if name is in list */
    for ( i = 0 ; i < numusrprocs ; i++)
    {
	if (strcmp(name, usrprocs[i].procname) == 0)
	{
	    incarnation++;
	}
    }

    /* Enlarge list for one additional element */
    numusrprocs++;
    usrprocs = ReAllocVec(usrprocs, sizeof(struct ProcedureList) * numusrprocs, MEMF_PUBLIC);
    if (usrprocs == NULL)
    {
	end_alloc();
    }
    usrprocs[i].procbody = cmd;
    usrprocs[i].procname = name;
    usrprocs[i].arglist = &(args[1]);
    usrprocs[i].argnum = num-1;

return incarnation;
}


/*
 * Free the memory of the user function list
 */
void free_proclist()
{
int i; 

    for ( i = 0 ; i < numusrprocs ; i++)
    {
	FreeVec(usrprocs[i].procname);
    }
    FreeVec(usrprocs);
}

