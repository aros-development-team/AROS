/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* variables.c -- Here are all functions related to variables */

#include "Installer.h"
#include "texts.h"
#include "main.h"
#include "gui.h"
#include "cleanup.h"
#include "variables.h"

/* External variables */
extern InstallerPrefs preferences;
extern IPTR args[TOTAL_ARGS];
extern UBYTE **tooltypes;


int numvariables = 0;
struct VariableList *variables = NULL;


/*
 * Get entry of variable in global list
 */
struct VariableList *find_var(char *name)
{
int i;

    /* Check if variable is in list */
    for ( i = 0 ; i < numvariables && strcmp(name, variables[i].varsymbol) != 0 ; i++ );
    if (i == numvariables)
    {
	return NULL;
    }
    else
    {
	return &(variables[i]);
    }
}


/*
 * Return the value of a variable
 * string if non-NULL else integer
 */
void *get_variable(char *name)
{
struct VariableList *entry;

    /* Find variable in lists */
    entry = find_var(name);

    /* Return Pointer to value */
    if (entry != NULL)
    {
	if (entry->vartext == NULL)
	{
	    return (void *)(entry->varinteger);
	}
	else
	{
	    return (void *)entry->vartext;
	}
    }
    else
    {
	return NULL;
    }
}


/*
 * Get the string value of a variable
 */
char *get_var_arg(char *name)
{
struct VariableList *entry;

    /* Find variable in lists */
    entry = find_var(name);

    if (entry != NULL)
    {
	return entry->vartext;
    }
    else
    {
	return NULL;
    }
}


/*
 * Get the integer value of a variable
 */
long int get_var_int(char *name)
{
struct VariableList *entry;

    /* Find variable in lists */
    entry = find_var(name);

    if (entry != NULL)
    {
	return entry->varinteger;
    }
    else
    {
	return 0;
    }
}


/*
 * Set the value of a variable
 * Add variable to global list if not existent
 */
void set_variable(char *name, char *text, long int intval)
{
int i;

    /* Check if variable is in list */
    for ( i = 0 ; i < numvariables && strcmp(name, variables[i].varsymbol) != 0 ; i++ );
    if (i == numvariables)
    {
	/* Enlarge list for one additional element */
	numvariables++;
	variables = ReAllocVec(variables, sizeof(struct VariableList) * numvariables, MEMF_PUBLIC);
	outofmem(variables);
	variables[i].varsymbol = NULL;
	variables[i].vartext = NULL;
    }
    else
    {
	/* Free space for strings to be replaced in dynamic list */
	FreeVec(variables[i].vartext);
	variables[i].vartext = NULL;
    }

    /* Change values in list */

    /* Duplicate variable name if it does not exist yet */
  
    if (variables[i].varsymbol == NULL)
    {
	variables[i].varsymbol = StrDup(name);
	outofmem(variables[i].varsymbol);
    }

    /* Duplicate variable text if existent */
    if (text != NULL)
    {
	variables[i].vartext = StrDup(text);
	outofmem(variables[i].vartext);
    }

    /* Set integer value */
    variables[i].varinteger = intval;

}


/*
 * Set initial variables at INIT stage
 */
void set_preset_variables(int argc)
{
char *ttemp;

    if (argc)
    { /* Started from Shell */
	if (args[ARG_APPNAME])
	{
	    set_variable("@app-name", (STRPTR)args[ARG_APPNAME], 0);
	}
	else
	{
	    set_variable("@app-name", "DemoApp", 0);
	}

	if (args[ARG_LANGUAGE])
	{
	    set_variable("@language", (char *)args[ARG_LANGUAGE], 0);
	}
	else
	{
	    set_variable("@language", "english", 0);
	}
    }
    else
    { /* Started from Workbench */
	ttemp = ArgString(tooltypes, "APPNAME", NULL);
	if (ttemp)
	{
	    set_variable("@app-name", ttemp, 0);
	}
	else
	{
	    set_variable("@app-name", "DemoApp", 0);
	}
	set_variable("@language", ArgString(tooltypes, "LANGUAGE", "english"), 0);
    }

    set_variable("@abort-button", ABORT_BUTTON, 0);
    set_variable("@default-dest", DEFAULT_DEST, 0);
    set_variable("@installer-version", NULL, (INSTALLER_VERSION << 16) + INSTALLER_REVISION);
    set_variable("@user-level", NULL, preferences.defusrlevel);
    set_variable("@pretend",    NULL, preferences.pretend);

    /* Set help texts */
    set_variable("@askchoice-help",  ASKCHOICE_HELP,  0);
    set_variable("@asknumber-help",  ASKNUMBER_HELP,  0);
    set_variable("@askoptions-help", ASKOPTIONS_HELP, 0);
    set_variable("@askstring-help",  ASKSTRING_HELP,  0);

    /* Set other variables to (NULL|0) */
    set_variable("@askdir-help",    NULL, 0);
    set_variable("@askdisk-help",   NULL, 0);
    set_variable("@askfile-help",   NULL, 0);
    set_variable("@copyfiles-help", NULL, 0);
    set_variable("@copylib-help",   NULL, 0);
    set_variable("@each-name",      NULL, 0);
    set_variable("@each-type",      NULL, 0);
    set_variable("@error-msg",      NULL, 0);
    set_variable("@execute-dir",    NULL, 0);
    set_variable("@icon",           NULL, 0);
    set_variable("@ioerr",          NULL, 0);
    set_variable("@makedir-help",   NULL, 0);
    set_variable("@special-msg",    NULL, 0);
    set_variable("@startup-help",   NULL, 0);
}


#ifdef DEBUG
/*
 * Dump values of all variables
 */
void dump_varlist()
{
int i;

    printf("DUMP of all %d variables:\n", numvariables);
    for ( i = 0 ; i < numvariables ; i++ )
    {
	printf("%s = %s | %ld\n", variables[i].varsymbol, variables[i].vartext, variables[i].varinteger);
    }

}
#endif /* DEBUG */


/*
 * Free the memory allocated by global varlist
 */
void free_varlist()
{
int i; 

    for ( i = 0 ; i < numvariables ; i++ )
    {
	FreeVec(variables[i].varsymbol);
	FreeVec(variables[i].vartext);
    }
    FreeVec(variables);
}

