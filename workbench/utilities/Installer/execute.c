/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* execute.c -- Here are all functions used to execute the script */

#include "Installer.h"
#include "execute.h"
#include "cmdlist.h"
#include "misc.h"
#include "gui.h"
#include "procedure.h"
#include "cleanup.h"
#include "variables.h"

#include <sys/stat.h>

/* External variables */
extern InstallerPrefs preferences;
extern int error, grace_exit;

/* Internal funtion declarations */
static void callback(char, char **);


#define ExecuteCommand()				\
    if (current->cmd != NULL)				\
    {							\
	execute_script(current->cmd, level + 1);	\
    }
#define ExecuteNextCommand()				\
    if (current->next->cmd != NULL)			\
    {							\
	execute_script(current->next->cmd, level + 1);	\
    }

int doing_abort = FALSE;
char * callbackstring = NULL, * globalstring = NULL;


/*
 * identify first arg with command and next ones as parameters to it
 * command has to be keyword or quoted string
 * parameters are converted as needed, <cmd> executed
 */
void execute_script(ScriptArg *commands, int level)
{
ScriptArg *current, *dummy = NULL;
struct ParameterList *parameter;
struct ProcedureList *usrproc;
int cmd_type, slen;
long int i = 0, j;
char *clip = NULL, **mclip = NULL, *string = NULL;
void *params;

    current = commands;
    /* Assume commands->cmd/arg to be first cmd/arg in parentheses */

    /* If first one is a (...)-function execute it */
    if (current->cmd != NULL)
    {
	execute_script(current->cmd, level + 1);
	/* So next ones are (...)-functions, too: execute them */
	while (current->next != NULL)
	{
	    current = current->next;
	    if (current->cmd != NULL)
	    {
		execute_script(current->cmd, level + 1);
	    }
	    else
	    {
		error = SCRIPTERROR;
		traperr("Argument in list of commands!\n", NULL);
	    }
	}
	FreeVec(current->parent->arg);
	current->parent->arg = NULL;
	current->parent->intval = current->intval;
	if (current->arg != NULL)
	{
	    current->parent->arg = StrDup(current->arg);
	    outofmem(current->parent->arg);
	}
    }
    else
    {
	cmd_type = eval_cmd(current->arg);
	FreeVec(current->parent->arg);
	current->parent->arg = NULL;
	current->parent->intval = 0;
	switch (cmd_type)
	{
	    case _UNKNOWN: /* Unknown command */
		error = SCRIPTERROR;
		traperr("Unknown command <%s>!\n", current->arg);
		break;

	    case _ABORT: /* Output all strings, execute onerrors and exit abnormally */
		string = collect_strings(current->next, LINEFEED, level);
		show_abort(string);
		FreeVec(string);
		if (preferences.transcriptstream != NULL)
		{
		    Write(preferences.transcriptstream, "Aborting script.\n", 17);
		}
		error = USERABORT;
		traperr("Aborting!", NULL);
		break;

	    case _AND:		/* logically AND two arguments		*/
	    case _BITAND:	/* bitwise AND two arguments		*/
	    case _BITOR:	/* bitwise OR two arguments		*/
	    case _BITXOR:	/* bitwise XOR two arguments		*/
	    case _DIFF:		/* returns 1 if 1st != 2nd else 0	*/
	    case _DIV:		/* divide 1st by 2nd intval		*/
	    case _EQUAL:	/* returns 1 if 1st = 2nd else 0	*/
	    case _LESS:		/* returns 1 if 1st < 2nd else 0	*/
	    case _LESSEQ:	/* returns 1 if 1st <= 2nd else 0	*/
	    case _MINUS:	/* subtract 2nd from 1st intval		*/
	    case _MORE:		/* returns 1 if 1st > 2nd else 0	*/
	    case _MOREEQ:	/* returns 1 if 1st >= 2nd else 0	*/
	    case _OR:		/* logically OR two arguments		*/
	    case _SHIFTLEFT:	/* shift 1st left by 2nd arg bits	*/
	    case _SHIFTRGHT:	/* shift 1st right by 2nd arg bits	*/
	    case _XOR:		/* logically XOR two arguments		*/
		if (current->next != NULL  && current->next->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    ExecuteNextCommand();
		    i = getint(current);
		    current = current->next;
		    j = getint(current);
		    switch (cmd_type)
		    {
			case _AND :
			    current->parent->intval = i && j;
			    break;
			case _BITAND :
			    current->parent->intval = i & j;
			    break;
			case _BITOR :
			    current->parent->intval = i | j;
			    break;
			case _BITXOR :
			    current->parent->intval = i ^ j;
			    break;
			case _DIFF :
			    current->parent->intval = (i != j) ? 1 : 0;
			    break;
			case _DIV :
			    if (j == 0)
			    {
				error = BADPARAMETER;
				traperr("Division by zero!\n", NULL);
			    }
			    current->parent->intval = (int)(i / j);
			    break;
			case _EQUAL :
			    current->parent->intval = (i == j) ? 1 : 0;
			    break;
			case _LESS :
			    current->parent->intval = (i < j) ? 1 : 0;
			    break;
			case _LESSEQ :
			    current->parent->intval = (i <= j) ? 1 : 0;
			    break;
			case _MINUS :
			    current->parent->intval = i - j;
			    break;
			case _MORE :
			    current->parent->intval = (i > j) ? 1 : 0;
			    break;
			case _MOREEQ :
			    current->parent->intval = (i >= j) ? 1 : 0;
			    break;
			case _OR :
			    current->parent->intval = i || j;
			    break;
			case _SHIFTLEFT :
			    current->parent->intval = i << j;
			    break;
			case _SHIFTRGHT :
			    current->parent->intval = i >> j;
			    break;
			case _XOR :
			    current->parent->intval = (i && !j) || (j && !i);
			    break;
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _CAT: /* Return concatenated strings */
		string = collect_strings(current->next, 0, level);
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _COMPLETE: /* Display how much we have done in percent */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		}
		else
		{
		    i = 0;
		}
		current->parent->intval = i;
		show_complete(i);
		break;

	    case _DEBUG: /* printf() all strings to shell */
		string = collect_strings(current->next, 0, level);
		if (preferences.debug && preferences.fromcli)
		{
		    printf("%s\n", string);
		}
		/* Set return value */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _EXIT: /* Output all strings and exit */
		/* print summary where app has been installed unless (quiet) is given */
		parameter = get_parameters(current->next, level);
		string = collect_strings(current->next, LINEFEED, level);
		show_exit(string);
		if (GetPL(parameter, _QUIET).intval == 0)
		{
		    final_report();
		}
		FreeVec(string);
		free_parameterlist(parameter);
#ifdef DEBUG
		dump_varlist();
#endif /* DEBUG */
		cleanup();
		exit(0);
		break;

	    case _IF: /* if 1st arg != 0 execute 2nd cmd else execute optional 3rd cmd */
		if (current->next != NULL && current->next->next != NULL)
		{
		    char *stringarg = NULL;
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		    if (i == 0)
		    {
			current = current->next;
			stringarg = current->arg;
		    }
		    if (current->next != NULL)
		    {
			current = current->next;
			ExecuteCommand();
			current->parent->intval = current->intval;
			if (current->arg != NULL)
			{
			    current->parent->arg = StrDup(current->arg);
			    outofmem(current->parent->arg);
			}
		    }
		    else if (stringarg)
		    {
			current->parent->arg = StrDup("\"\"");
			outofmem(current->parent->arg);
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _IN: /* Return (arg1) bitwise-and with bit numbers given as following args */
		/* Get base integer into i */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		}
		/* Write the corresponding bits of i into parent */
		while (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    j = getint(current);
		    current->parent->intval |= i & (1 << j);
		}
		break;

	    case _BITNOT: /* bitwise invert argument */
	    case _NOT: /* logically invert argument */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		    current->parent->intval = (cmd_type == _NOT) ? !i : ~i;
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires one argument!\n", current->arg);
		}
		break;

	    case _PLUS: /* Sum up all arguments and return that value */
		while (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		    current->parent->intval += i;
		}
		break;

	    case _PROCEDURE: /* Link user function to global function name-space */
		link_function(current->next->arg, current->next->intval);
		break;

	    case _SELECT: /* Return the nth item of arguments, NULL|0 if 0 */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		    if (i > 0)
		    {
			j = 0;
			for (; i > 0 ; i--)
			{
			    if (current->next != NULL)
			    {
				current = current->next;
			    }
			    else
			    {
				j = 1;
			    }
			}
			if (j == 0)
			{
			    current->parent->intval = current->intval;
			    if (current->arg != NULL)
			    {
				current->parent->arg = StrDup(current->arg);
				outofmem(current->parent->arg);
			    }
			}
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _SYMBOLSET: /* assign values to variables -- allow strings and commands as variablenames */
		/* take odd args as names and even as values */
		if (current->next != NULL)
		{
		    char *clip2;
		    current = current->next;
		    while (current != NULL && current->next != NULL)
		    {
			i = current->next->intval;
			clip = NULL;
			string = NULL;
			ExecuteCommand();
			if (current->arg == NULL)
			{
			    /* There is no varname */
			    error = BADPARAMETER;
			    traperr("Variable name to <%s> is not a string!\n", current->parent->cmd->arg);
			}
			if (current->arg != NULL && (current->arg[0] == SQUOTE || current->arg[0] == DQUOTE))
			{
			    /* There is a quoted varname */
			    /* Strip off quotes */
			    string = strip_quotes(current->arg);
			}
			else
			{
			    /* Varname is stored in variable */
			    clip2 = get_var_arg(current->arg);
			    if (clip2 == NULL)
			    {
				/* There is no varname */
				error = BADPARAMETER;
				traperr("Variable name to <%s> is not a string!\n", current->parent->cmd->arg);
			    }
			    string = StrDup(clip2);
			    outofmem(string);
			}
			ExecuteNextCommand();
			if (current->next->arg != NULL)
			{
			    if ((current->next->arg)[0] == SQUOTE || (current->next->arg)[0] == DQUOTE)
			    {
				/* Strip off quotes */
				clip = strip_quotes(current->next->arg);
				i = 0;
			    }
			    else
			    {
				/* value is a variable */
				clip2 = get_var_arg(current->next->arg);
				if (clip2 == NULL)
				{
				    clip = NULL;
				}
				else
				{
				    clip = StrDup(clip2);
				    outofmem(clip);
				}
				i = get_var_int(current->next->arg);
			    }
			}
			set_variable(string, clip, i);
			FreeVec(string);
			FreeVec(clip);
			dummy = current;
			current = current->next->next;
		    }
		}
		/* SET returns the value of the of the last assignment */
		if (dummy->next->arg != NULL)
		{
		    if ((dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE)
		    {
			dummy->parent->arg = StrDup(dummy->next->arg);
			outofmem(dummy->parent->arg);
		    }
		    else
		    {
			clip = get_var_arg(dummy->next->arg);
			if (clip)
			{
			    /* Add surrounding quotes to string */
			    dummy->parent->arg = addquotes(clip);
			}
			dummy->parent->intval = get_var_int(dummy->next->arg);
		    }
		}
		else
		{
		    dummy->parent->intval = dummy->next->intval;
		}
		break;

	    case _SYMBOLVAL: /* return values of variables -- allow strings and commands as variablenames */
		if (current->next != NULL)
		{
		    string = NULL;
		    clip = NULL;
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg == NULL)
		    {
			/* There is no varname */
			error = BADPARAMETER;
			traperr("Variable name to <%s> is not a string!\n", current->parent->cmd->arg);
		    }
		    if (current->arg[0] == SQUOTE || current->arg[0] == DQUOTE)
		    {
			/* There is a quoted varname */
			/* Strip off quotes */
			string = strip_quotes(current->arg);
			current->parent->arg = get_var_arg(string);
			current->parent->intval = get_var_int(string);
			FreeVec(string);
		    }
		    else
		    {
			/* Varname is stored in variable */
			current->parent->arg = get_var_arg(current->arg);
			current->parent->intval = get_var_int(current->arg);
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires one argument!\n", current->arg);
		}
		break;

	    case _SET: /* assign values to variables */
		/* take odd args as names and even as values */
		if (current->next != NULL)
		{
		    current = current->next;
		    while (current != NULL && current->next != NULL)
		    {
			if (current->cmd != NULL)
			{
			    /* There is a command instead of a varname */
			    error = BADPARAMETER;
			    traperr("<%s> expected variablename, found function instead!\n", current->parent->cmd->arg);
			}
			if (current->arg == NULL)
			{
			    /* There is no varname */
			    error = BADPARAMETER;
			    traperr("Variable name to <%s> is not a string!\n", current->parent->cmd->arg);
			}
			if (current->arg != NULL && (current->arg[0] == SQUOTE || current->arg[0] == DQUOTE))
			{
			    /* There is a quoted varname */
			    error = BADPARAMETER;
			    traperr("<%s> expected symbol, found quoted string instead!\n", current->parent->cmd->arg);
			}
			ExecuteNextCommand();
			if (current->next->arg != NULL)
			{
			    if ((current->next->arg)[0] == SQUOTE || (current->next->arg)[0] == DQUOTE)
			    {
				/* Strip off quotes */
				clip = strip_quotes(current->next->arg);
				set_variable(current->arg, clip, current->next->intval);
				FreeVec(clip);
			    }
			    else
			    {
				/* value is a variable */
				set_variable(current->arg, get_var_arg(current->next->arg), get_var_int(current->next->arg));
			    }
			}
			else
			{
			    set_variable(current->arg, current->next->arg, current->next->intval);
			}
			dummy = current;
			current = current->next->next;
		    }
		}
		/* SET returns the value of the of the last assignment */
		if (dummy->next->arg != NULL)
		{
		    if ((dummy->next->arg)[0] == SQUOTE || (dummy->next->arg)[0] == DQUOTE)
		    {
			dummy->parent->arg = StrDup(dummy->next->arg);
			outofmem(dummy->parent->arg);
		    }
		    else
		    {
			clip = get_var_arg(dummy->next->arg);
			if (clip)
			{
			    /* Add surrounding quotes to string */
			    dummy->parent->arg = addquotes(clip);
			}
			dummy->parent->intval = get_var_int(dummy->next->arg);
		    }
		}
		else
		{
		    dummy->parent->intval = dummy->next->intval;
		}
		break;

	    case _STRLEN: /* Return the length of the string, 0 for integer argument */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    current->parent->intval = strlen(current->arg) - 2;
			}
			else
			{
			    if ((clip = get_var_arg(current->arg)) == NULL)
			    {
				current->parent->intval = 0;
			    }
			    else
			    {
				current->parent->intval = strlen(clip);
			    }
			}
		    }
		}
		break;

	    case _STRING: /* Call RawDoFmt with string as format and args and return output */

		/* Prepare base string */
		/* Strip off quotes */
		clip = strip_quotes(current->arg);

		/* Now get arguments into typeless array (void *params) */
		params = AllocVec(sizeof(IPTR), MEMF_PUBLIC);
		outofmem(params);
		((char **)params)[0] = NULL;
		mclip = NULL;
		i = 0;
		j = 0;
		while (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    /* Strip off quotes */
			    mclip = ReAllocVec(mclip, sizeof(char *) * (j+1), MEMF_PUBLIC);
			    outofmem(mclip);
			    mclip[j] = strip_quotes(current->arg);
			    ((char **)params)[i] = mclip[j];
			    j++;
			}
			else
			{
			    ((char **)params)[i] = (char *)get_variable(current->arg);
			}
		    }
		    else
		    {
			((char **)params)[i] = (char *)(current->intval);
		    }
		    i++;
		    params = ReAllocVec(params, sizeof(IPTR)*(i+1), MEMF_PUBLIC);
		    outofmem(params);
		}
		/* Call RawDoFmt() with parameter list */
		/* Store that produced string as return value */
		string = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
		outofmem(string);
		callbackstring = string;
		globalstring = callbackstring;
		RawDoFmt(clip, params, (VOID_FUNC)&callback, &globalstring);
		string = callbackstring;
		/* Free temporary space */
		FreeVec(clip);
		if (mclip)
		{
		    while (j > 0)
		    {
			FreeVec(mclip[--j]);
		    }
		    FreeVec(mclip);
		}
		/* Add surrounding quotes to string */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _SUBSTR: /* Return the substring of arg1 starting with arg2+1 character up to arg3 or end if !arg3 */
		if (current->next != NULL && current->next->next != NULL)
		{
		    /* Get string */
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    /* Strip off quotes */
			    string = strip_quotes(current->arg);
			}
			else
			{
			    clip = get_var_arg(current->arg);
			    if (clip != NULL)
			    {
				string = StrDup(clip);
				outofmem(string);
			    }
			    else
			    {
				string = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
				outofmem(string);
				sprintf(string, "%ld", get_var_int(current->arg));
			    }
			}
		    }
		    else
		    {
			string = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
			outofmem(string);
			sprintf(string, "%ld", current->intval);
		    }
		    current = current->next;
		    /* Get offset */
		    i = getint(current);
		    slen = strlen(string) - i;
		    if (i < 0)
		    {
			FreeVec(string);
			error = BADPARAMETER;
			traperr("Negative argument to <%s>!\n", current->parent->cmd->arg);
		    }
		    /* Get number of chars to copy */
		    if (current->next != NULL)
		    {
			current = current->next;
			ExecuteCommand();
			j = getint(current);
			if (j < 0)
			{
			    j = 0;
			}
			slen = j;
		    }
		    else
		    {
			j = slen;
		    }
		    clip = AllocVec(slen + 1, MEMF_PUBLIC);
		    outofmem(clip);
		    strncpy(clip, (string + i), j);
		    clip[j] = 0;
		    FreeVec(string);
		    current->parent->arg = clip;
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires at least two arguments!\n", current->arg);
		}
		break;

	    case _TIMES: /* Multiply all arguments and return that value */
		if (current->next == NULL)
		{
		    error = SCRIPTERROR;
		    traperr("No arguments to <%s>!\n", current->arg);
		}
		current->parent->intval = 1;
		while (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current);
		    current->parent->intval *= i;
		}
		break;

	    case _TRANSCRIPT: /* concatenate strings into logfile */
		string = collect_strings(current->next, 0, level);
		if (preferences.transcriptfile != NULL)
		{
		    Write(preferences.transcriptstream, string, strlen(string));
		    Write(preferences.transcriptstream, "\n", 1);
		}
		/* Add surrounding quotes to string */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _UNTIL: /* execute 2nd cmd until 1st arg != 0 */
		if (current->next != NULL && current->next->next != NULL)
		{
		    current = current->next;
		    if (current->next->cmd == NULL)
		    {
			/* We don't have a block, so what can we execute ??? */
			error = SCRIPTERROR;
			traperr("<%s> has no command-block!\n", current->parent->cmd->arg);
		    }
		    i = 0;
		    while (i == 0)
		    {
			/* Execute command */
			ExecuteNextCommand();

			/* Now check condition */
			ExecuteCommand();
			i = getint(current);

			/* condition is true -> return values and exit */
			if (i != 0)
			{
			    current->parent->intval = current->next->intval;
			    if (current->next->arg != NULL)
			    {
				current->parent->arg = StrDup(current->next->arg);
				outofmem(current->parent->arg);
			    }
			}
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _USER: /* Change the current user-level -- Use only to debug scripts */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			string = NULL;
			if ((current->arg[0] == SQUOTE || current->arg[0] == DQUOTE))
			{
			    /* Strip off quotes */
			    string = strip_quotes(current->arg);
			    clip = string;
			}
			else
			{
			    clip = get_var_arg(current->arg);
			}
			if (clip != NULL)
			{
			    i = atol(clip);
			    if (strcasecmp(clip, "novice") == 0)
				i = _NOVICE;
			    if (strcasecmp(clip, "average") == 0)
				i = _AVERAGE;
			    if (strcasecmp(clip, "expert") == 0)
				i = _EXPERT;
			    FreeVec(string);
			}
			else
			{
			    i = get_var_int(current->arg);
			}
		    }
		    else
		    {
			i = current->intval;
		    }
		    if (i < _NOVICE || i > _EXPERT)
		    {
			error = BADPARAMETER;
			traperr("New user-level not in [Novice|Average|Expert] !\n", NULL);
		    }
		    else
		    {
			set_variable("@user-level", NULL, i);
			current->parent->intval = i;
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires one argument!\n", current->arg);
		}
		break;

	    case _WELCOME: /* Display strings instead of "Welcome to the <APPNAME> App installation utility" */
		string = collect_strings(current->next, SPACE, level);
		request_userlevel(string);

		/* Set return value */
		/* Add surrounding quotes to string */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _WHILE: /* while 1st arg != 0 execute 2nd cmd */
		if (current->next != NULL && current->next->next != NULL)
		{
		    current = current->next;
		    if (current->next->cmd == NULL)
		    {
			/* We don't have a block, so what can we execute ??? */
			error = SCRIPTERROR;
			traperr("<%s> has no command-block!\n", current->parent->cmd->arg);
		    }
		    i = 1;
		    while (i != 0)
		    {
			ExecuteCommand();

			/* Now check condition */
			i = getint(current);
			if (i != 0)
			{
			    ExecuteNextCommand();
			}
			else
			{
			    current->parent->intval = current->next->intval;
			    if (current->next->arg != NULL)
			    {
				current->parent->arg = StrDup(current->next->arg);
				outofmem(current->parent->arg);
			    }
			}
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _MESSAGE: /* Display strings and offer Proceed, Abort, Help */
		string = collect_strings(current->next, LINEFEED, level);
		parameter = get_parameters(current->next, level);
		show_message(string, parameter);

		/* Set return value */
		/* Add surrounding quotes to string */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		free_parameterlist(parameter);
		break;

	    case _WORKING: /* Display strings below "Working on Installation" */
		string = collect_strings(current->next, LINEFEED, level);
		show_working(string);

		/* Set return value */
		/* Add surrounding quotes to string */
		current->parent->arg = addquotes(string);
		FreeVec(string);
		break;

	    case _DATABASE: /* Return information on the hardware Installer is running on */
		if (current->next != NULL)
		{
		    current = current->next;
		    clip = strip_quotes(current->arg);
		    i = database_keyword(clip);
		    FreeVec(clip);
#warning TODO: compute return values for "database"
		    switch (i)
		    {
			case _VBLANK :
			    clip = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
			    outofmem(clip);
			    sprintf(clip, "%c%d%c", DQUOTE, SysBase->VBlankFrequency, DQUOTE);
			    current->parent->arg = StrDup(clip);
			    outofmem(current->parent->arg);
			    FreeVec(clip);
			    break;

			case _CPU:
			    break;

			case _GRAPHICS_MEM:
			    current->parent->intval = AvailMem(MEMF_CHIP);
			    break;

			case _TOTAL_MEM:
			    current->parent->intval = AvailMem(MEMF_TOTAL);
			    break;

			case _FPU:
			    break;

			case _CHIPREV:
			    break;

			default :
			    break;
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires one argument!\n", current->arg);
		}
		break;

	    case _ASKBOOL: /* Ask user for a boolean */
		parameter = get_parameters(current->next, level);
		current->parent->intval = request_bool(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKNUMBER: /* Ask user for a number */
		parameter = get_parameters(current->next, level);
		current->parent->intval = request_number(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKSTRING: /* Ask user for a string */
		parameter = get_parameters(current->next, level);
		current->parent->arg = request_string(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKCHOICE: /* Ask user to choose one item */
		parameter = get_parameters(current->next, level);
		current->parent->intval = request_choice(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKDIR: /* Ask user for a directory */
		parameter = get_parameters(current->next, level);
		current->parent->arg = request_dir(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKDISK: /* Ask user to insert a disk */
		parameter = get_parameters(current->next, level);
		current->parent->arg = request_disk(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKFILE: /* Ask user for a filename */
		parameter = get_parameters(current->next, level);
		current->parent->arg = request_file(parameter);
		free_parameterlist(parameter);
		break;

	    case _ASKOPTIONS: /* Ask user to choose multiple items */
		parameter = get_parameters(current->next, level);
		current->parent->intval = request_options(parameter);
		free_parameterlist(parameter);
		break;

	    case _ONERROR: /* link onerror to preferences */
		current = current->next;
		/* reset parent of old onerror statement */
		dummy = preferences.onerror.cmd;
		while (dummy != NULL)
		{
		    dummy->parent = preferences.onerrorparent;
		    dummy = dummy->next;
		}
		/* set new onerror statement */
		preferences.onerror.cmd = current->cmd;
		preferences.onerrorparent = current;
		/* set new parent of new onerror statement */
		dummy = current->cmd;
		while (dummy != NULL)
		{
			    dummy->parent = &(preferences.onerror);
			    dummy = dummy->next;
		}
		break;

	    case _TRAP: /* link trap to preferences */
		if (current->next != NULL  && current->next->next->cmd != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    i = getint(current) - 1;
		    current = current->next;
		    /* reset parent of old trap statement */
		    dummy = preferences.trap[i].cmd;
		    while (dummy != NULL)
		    {
			dummy->parent = preferences.trapparent[i];
			dummy = dummy->next;
		    }
		    /* set new onerror statement */
		    preferences.trap[i].cmd = current->cmd;
		    preferences.trapparent[i] = current;
		    /* set new parent of new onerror statement */
		    dummy = current->cmd;
		    while (dummy != NULL)
		    {
			dummy->parent = &(preferences.trap[i]);
			dummy = dummy->next;
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _RENAME: /* Rename a file or relabel a disk if disk parameter */
		if (current->next != NULL && current->next->next != NULL)
		{
		int success = DOSFALSE,
		    usrconfirm = FALSE;
		    /* Get strings */
		    current = current->next;
		    ExecuteCommand();
		    ExecuteNextCommand();
		    if (current->arg != NULL && current->next->arg != NULL)
		    {
			string = strip_quotes(current->arg);
			clip = strip_quotes(current->next->arg);
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires two strings as arguments!\n", current->parent->cmd->arg);
		    }
		    if (current->next->next)
		    {
			parameter = get_parameters(current->next->next, level);
			if (GetPL(parameter, _CONFIRM).used == 1)
			{
			    usrconfirm = request_confirm(parameter);
			}
			if (GetPL(parameter, _DISK).used == 1)
			{
			    /* Relabel disk */
			    if ((preferences.pretend == 0 || GetPL(parameter, _SAFE).used == 1) && usrconfirm)
			    
			    {
				success = Relabel(string,clip);
			    }
			}
			else
			{
			    /* Rename file */
			    if ((preferences.pretend == 0 || GetPL(parameter, _SAFE).used == 1) && usrconfirm)
			    {
				success = Rename(string,clip);
			    }
			}
			free_parameterlist(parameter);
		    }
		    else
		    {
			if (preferences.pretend == 0)
			{
			    success = Rename(string,clip);
			}
		    }
		    if (success == DOSTRUE)
		    {
			current->parent->intval = 1;
		    }
		    else
		    {
			current->parent->intval = 0;
			set_variable("@ioerr", NULL, IoErr());
		    }
		    FreeVec(string);
		    FreeVec(clip);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two arguments!\n", current->arg);
		}
		break;

	    case _EXECUTE: /* Execute an AmigaDOS script */
#warning TODO: Check me for correctness
		if (current->next != NULL)
		{
		int success = 0;
		BPTR infile;
		int safe = FALSE;

		    current = current->next;
		    ExecuteCommand();

		    if (current->arg != NULL)
		    {
			string = strip_quotes(current->arg);
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires a file string as argument!\n", current->parent->cmd->arg);
		    }
		    if (current->next)
		    {
			parameter = get_parameters(current->next, level);
			safe = GetPL(parameter, _SAFE).used;
			free_parameterlist(parameter);
		    }
		    if (preferences.pretend == 0 || safe)
		    {
			infile = Open(string, MODE_OLDFILE);
			if(infile != NULL)
			{
			    if (preferences.transcriptstream != NULL)
			    {
				Write(preferences.transcriptstream, "Started AmigaDOS script: \"", 26);
				Write(preferences.transcriptstream, string, strlen(string));
				Write(preferences.transcriptstream, "\"\n", 2);
			    }
			    success = Execute(NULL, infile, preferences.transcriptstream);
			    Close(infile);
			}
			else
			{
			    success = FALSE;
			}
		    }
		    if (success)
		    {
			current->parent->intval = 1;
		    }
		    else
		    {
			current->parent->intval = 0;
			set_variable("@ioerr", NULL, IoErr());
		    }
		    FreeVec(string);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a string argument!\n", current->arg);
		}
		break;

	    case _RUN: /* Execute a command line */
#warning TODO: Check me for correctness
		if (current->next != NULL)
		{
		BPTR seg;
		    parameter = get_parameters(current->next, level);
		    if (preferences.pretend == 0 || GetPL(parameter, _SAFE).used == 1)
		    {
			string = collect_strings(current->next, SPACE, level);
			if (string == NULL)
			{
			    error = BADPARAMETER;
			    traperr("<%s> requires a string parameter!\n", current->parent->cmd->arg);
			}
			for (i = 0 ; string[i] != 0 && string[i] != SPACE ; i++);
			if (string[i] == SPACE)
			{
			    string[i] = 0;
			    clip = &(string[i+1]);
			    j = strlen(clip);
			}
			else
			{
			    clip = NULL;
			    j = 0;
			}
			if (get_var_int("@user-level") >= GetPL(parameter, _CONFIRM).intval)
			{
			    if ((seg = LoadSeg(string)) == NULL)
			    {
				/* Couldn't load file -- set @ioerr and handle trap/onerror */
				i = IoErr();
#ifdef DEBUG
				PrintFault(i, INSTALLER_NAME);
#endif /* DEBUG */
				set_variable("@ioerr", NULL, i);
				error = DOSERROR;
				traperr("Couldn't load binary %s\n", string);
			    }
			    if (preferences.transcriptstream != NULL)
			    {
				Write(preferences.transcriptstream, "Started program: \"", 18);
				Write(preferences.transcriptstream, string, strlen(string));
				Write(preferences.transcriptstream, "\"\n", 2);
			    }
#define STACKSIZE 10000
			    current->parent->intval = RunCommand(seg, STACKSIZE, clip, j);
#warning FIXME: is @ioerr set if command not run?
			    set_variable("@ioerr", NULL, IoErr());
			    UnLoadSeg(seg);
			}
			FreeVec(string);
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires arguments!\n", current->arg);
		}
		break;

	    case _STARTUP: /* Add a section to S:Startup-Sequence */
		ExecuteNextCommand();
		if (current->next->arg != NULL)
		{
		    string = strip_quotes(current->next->arg);
		    parameter = get_parameters(current->next, level);
		    if (request_confirm(parameter))
		    {
			modify_userstartup(string, parameter);
		    }
		    free_parameterlist(parameter);
		    FreeVec(string);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a name-string as argument!\n", current->arg);
		}
		break;

	    case _DELETE: /* Delete file */
#warning TODO: Implement (optional) and (delopts)
		if (current->next != NULL)
		{
		int success = -1, usrconfirm = FALSE;

		    current = current->next;
		    ExecuteCommand();

		    if (current->arg != NULL)
		    {
			string = strip_quotes(current->arg);
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires a file string as argument!\n", current->parent->cmd->arg);
		    }
		    if (current->next)
		    {
			parameter = get_parameters(current->next, level);
			if (GetPL(parameter, _CONFIRM).used == 1)
			{
			    usrconfirm = request_confirm(parameter);
			}
			/* Delete file */
			if ((preferences.pretend == 0 || GetPL(parameter, _SAFE).used == 1) && usrconfirm)
			{
			    success = DeleteFile(string);
			}
			free_parameterlist(parameter);
		    }
		    else
		    {
			if (preferences.pretend == 0)
			{
			    success = DeleteFile(string);
			}
		    }
		    if (success == 0)
		    {
			current->parent->intval = 1;
		    }
		    else
		    {
			current->parent->intval = 0;
			set_variable("@ioerr", NULL, IoErr());
		    }
		    FreeVec(string);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a string argument!\n", current->arg);
		}
		break;

	    case _MAKEDIR: /* Create directory */
#warning TODO: Implement (infos)
		if (current->next != NULL)
		{
		BPTR success = 0;
		int usrconfirm = FALSE;

		    current = current->next;
		    ExecuteCommand();

		    if (current->arg != NULL)
		    {
			string = strip_quotes(current->arg);
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires a file string as argument!\n", current->parent->cmd->arg);
		    }
		    if (current->next)
		    {
			parameter = get_parameters(current->next, level);
			if (GetPL(parameter, _CONFIRM).used == 1)
			{
			    usrconfirm = request_confirm(parameter);
			}
			/* Create directory */
			if ((preferences.pretend == 0 || GetPL(parameter, _SAFE).used == 1) && usrconfirm)
			{
			    success = CreateDir(string);
			}
			free_parameterlist(parameter);
		    }
		    else
		    {
			if (preferences.pretend == 0)
			{
			    success = CreateDir(string);
			}
		    }
		    /* return value of CreateDir() is a lock or 0 */
		    if (success != 0)
		    {
			UnLock(success);
			current->parent->intval = 1;
		    }
		    else
		    {
			current->parent->intval = 0;
			set_variable("@ioerr", NULL, IoErr());
		    }
		    FreeVec(string);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a string argument!\n", current->arg);
		}
		break;

	    case _EXISTS:
#warning TODO: Implement (noreq)
		if (current->next != NULL)
		{
		struct stat sb;

		    current = current->next;
		    ExecuteCommand();

		    if (current->arg != NULL)
		    {
			string = strip_quotes(current->arg);
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires a file string as argument!\n", current->parent->cmd->arg);
		    }
		    parameter = NULL;
		    if (current->next)
		    {
			parameter = get_parameters(current->next, level);
		    }
		    if (parameter)
		    {
			free_parameterlist(parameter);
		    }
		    if(stat(string, &sb) == -1)
		    {
			current->parent->intval = 0;
		    }
		    else
		    {
			if(sb.st_mode & S_IFDIR)
			{
			    /* Directory */
			    current->parent->intval = 2;
			}
			else if(sb.st_mode & S_IFREG)
			{
			    /* File */
			    current->parent->intval = 1;
			}
			else
			{
			    current->parent->intval = 0;
			}
		    }
		    FreeVec(string);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a string argument!\n", current->arg);
		}
		break;

	    case _FILEONLY: /* Return the file part of a pathname */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
		    char *tempnam;
			if((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    string = strip_quotes(current->arg);
			}
			else
			{
			    if((clip = get_var_arg(current->arg)) == NULL)
			    {
				string = StrDup(current->arg);
			    }
			    else
			    {
				string = strip_quotes(clip);
			    }
			}
			tempnam = FilePart(string);
			i = strlen(tempnam);
			clip = AllocVec(sizeof(char) * i + 3, MEMF_PUBLIC);
			clip[0] = '"';
			strcpy(&clip[1], tempnam);
			clip[ i + 1 ] = '"';
			clip[ i + 2 ] = 0;
			current->parent->arg = clip;
			FreeVec(string);
		    }
		}
		break;

	    case _PATHONLY: /* Return the path part of a pathname */
		if (current->next != NULL)
		{
		char *tempnam;

		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    string = strip_quotes(current->arg);
			}
			else
			{
			    if((clip = get_var_arg(current->arg)) == NULL)
			    {
				string = StrDup(current->arg);
			    }
			    else
			    {
				  string = strip_quotes(clip);
			    }
			}
			tempnam = PathPart(string);
			clip = AllocVec(sizeof(char) * (tempnam - string + 3), MEMF_PUBLIC);
			clip[0] = '"';
			strncpy(&clip[1], string, tempnam - string);
			clip[ tempnam - string + 1 ] = '"';
			clip[ tempnam - string + 2 ] = 0;
			current->parent->arg = clip;
			FreeVec(string);
		    }
		}
		break;

	    case _EARLIER: /* Return TRUE if file1 is older than file2 */
		if (current->next != NULL && current->next->next != NULL)
		{
		char *file1 = NULL, *file2 = NULL;
		BPTR lock1, lock2;
		struct FileInfoBlock *fib1, *fib2;

		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    string = strip_quotes(current->arg);
			}
			else
			{
			    if((clip = get_var_arg(current->arg)) == NULL)
			    {
				string = StrDup(current->arg);
			    }
				else
			    {
				  string = strip_quotes(clip);
			    }
			}
			file1 = string;
		    }
		    else
		    {
			error = BADPARAMETER;
			traperr("<%s> requires two string arguments!\n", current->arg);
		    }

		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    string = strip_quotes(current->arg);
			}
			else
			{
			    if((clip = get_var_arg(current->arg)) == NULL)
			    {
				string = StrDup(current->arg);
			    }
				else
			    {
				  string = strip_quotes(clip);
			    }
			}
			file2 = string;
		    }
		    else
		    {
			error = BADPARAMETER;
			traperr("<%s> requires two string arguments!\n", current->arg);
		    }

		    if ((lock1 = Lock(file1, SHARED_LOCK)) == NULL)
		    {
			error = DOSERROR;
			traperr("File not found <%s>\n", file1);
		    }
		    if ((lock2 = Lock(file2, SHARED_LOCK)) == NULL)
		    {
			error = DOSERROR;
			traperr("File not found <%s>\n", file2);
		    }
		    if ((fib1 = AllocDosObject((ULONG)DOS_FIB, NULL)) == NULL)
		    {
			error = DOSERROR;
			traperr("Could not AllocDosObject FIB for file <%s>\n", file1);
		    }
		    if ((fib2 = AllocDosObject((ULONG)DOS_FIB, NULL)) == NULL)
		    {
			error = DOSERROR;
			traperr("Could not AllocDosObject FIB for file <%s>\n", file2);
		    }
		    if (Examine(lock1, fib1) == FALSE)
		    {
			error = DOSERROR;
			traperr("Could not Examine() file <%s>\n", file1);
		    }
		    if (Examine(lock2, fib2) == FALSE)
		    {
			error = DOSERROR;
			traperr("Could not Examine() file <%s>\n", file2);
		    }
		    if (fib1->fib_Date.ds_Days < fib2->fib_Date.ds_Days)
		    {
			current->parent->intval = 1;
		    }
		    else if (fib1->fib_Date.ds_Days == fib2->fib_Date.ds_Days)
		    {
			if (fib1->fib_Date.ds_Minute < fib2->fib_Date.ds_Minute)
			{
			    current->parent->intval = 1;
			}
			else if (fib1->fib_Date.ds_Minute == fib2->fib_Date.ds_Minute)
			{
			    if (fib1->fib_Date.ds_Tick < fib2->fib_Date.ds_Tick)
			    {
				current->parent->intval = 1;
			    }
			}
		    }
		    FreeDosObject(DOS_FIB, fib2);
		    FreeDosObject(DOS_FIB, fib1);
		    UnLock(lock2);
		    UnLock(lock1);
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires two string arguments!\n", current->arg);
		}
		break;

	    case _GETDISKSPACE: /* Return the number of free bytes on a device, or -1 on bad pathname or when info could not be obtained */
		if (current->next != NULL)
		{
		BPTR lock;
		struct InfoData infodata;

		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    string = strip_quotes(current->arg);
			}
			else
			{
			    if((clip = get_var_arg(current->arg)) == NULL)
			    {
				string = StrDup(current->arg);
			    }
				else
			    {
				string = strip_quotes(clip);
			    }
			}
			current->parent->intval = -1;
			if ((lock = Lock(string, SHARED_LOCK)) != NULL)
			{
			    if (Info(lock, &infodata) != FALSE)
			    {
				current->parent->intval = (infodata.id_NumBlocks - infodata.id_NumBlocksUsed) * infodata.id_BytesPerBlock;
			    }
			    UnLock(lock);
			}
			FreeVec(string);
		    }
		}
		break;

	    case _GETENV: /* Get Variable from ENV: */
		if (current->next != NULL)
		{
		    current = current->next;
		    ExecuteCommand();

		    if (current->arg != NULL)
		    {
		    char buffer[1024] = {0};
			string = strip_quotes(current->arg);

#warning FIXME: flag must be one of GVF_GLOBAL_ONLY or GVF_LOCAL_ONLY???
			i = GetVar(string, buffer, 1023, 0);

			FreeVec(string);

			if(i != -1)
			{
			    current->parent->arg = StrDup(buffer);
			}
		    }
		    else
		    {
			error = SCRIPTERROR;
			traperr("<%s> requires a string as argument!\n", current->parent->cmd->arg);
		    }
		}
		else
		{
		    error = SCRIPTERROR;
		    traperr("<%s> requires a string as argument!\n", current->arg);
		}
		break;

      /* Here are all unimplemented commands */
	    case _COPYFILES	:
	    case _COPYLIB	:
	    case _EXPANDPATH	:
	    case _FOREACH	:
	    case _GETASSIGN	:
	    case _GETDEVICE	:
	    case _GETSIZE	:
	    case _GETSUM	:
	    case _GETVERSION	:
	    case _ICONINFO	:
	    case _MAKEASSIGN	:
	    case _PATMATCH	:
	    case _PROTECT	:
	    case _REXX		:
	    case _TACKON	:
	    case _TEXTFILE	:
	    case _TOOLTYPE	:
		fprintf(stderr, "Unimplemented command <%s>\n", current->arg);
		break;

	    case _USERDEF: /* User defined routine */
		usrproc = find_proc(current->arg);
		/* Set argument variables */
		i = 0;
		while (current->next != NULL && i < usrproc->argnum)
		{
		    current = current->next;
		    ExecuteCommand();
		    if (current->arg != NULL)
		    {
			if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
			{
			    /* Strip off quotes */
			    clip = strip_quotes(current->arg);
			    set_variable(usrproc->arglist[i], clip, 0);
			    FreeVec(clip);
			}
			else
			{
			    /* value is a variable */
			    set_variable(usrproc->arglist[i], get_var_arg(current->arg), get_var_int(current->arg));
			}
		    }
		    else
		    {
			set_variable(usrproc->arglist[i], NULL, current->intval);
		    }
		    i++;
		}
		/* Execute procedure */
		dummy = usrproc->procbody;
		execute_script(dummy->cmd, level + 1);
		current->parent->intval = dummy->intval;
		if (dummy->arg != NULL)
		{
		    current->parent->arg = StrDup(dummy->arg);
		    outofmem(current->parent->arg);
		}
		break;

      /* Here are all tags, first the ones which have to be executed */
	    case _DELOPTS: /* unset copying/deleting options if we are called global */
		/* as parameter to a function we have got an ignore=1 before */
#warning FIXME: (delopts) is only local
		if (current->parent->ignore == 0)
		{
		    /* Read in strings */
		    parameter = AllocVec(sizeof(struct ParameterList), MEMF_PUBLIC);
		    outofmem(parameter);
		    collect_stringargs(current, level, parameter);
		    /* Store data in preferences */
		    for (i = 0 ; i < parameter->intval ; i++)
		    {
			/* These are mutually exclusive */
#warning FIXME: How are (fail-)strings interpreted in "delopts" ?
			if (strcasecmp(parameter->arg[i], "fail") == 0)
			{
			}
			if (strcasecmp(parameter->arg[i], "nofail") == 0)
			{
			}
			if (strcasecmp(parameter->arg[i], "oknodelete") == 0)
			{
			}

			/* These may be combined in any way */
			if (strcasecmp(parameter->arg[i], "force") == 0)
			{
			    preferences.copyflags &= ~COPY_ASKUSER;
			}
			if (strcasecmp(parameter->arg[i], "askuser") == 0)
			{
			    preferences.copyflags &= ~COPY_ASKUSER;
			}
		    }

		    free_parameterlist(parameter);
		}
		break;

	    case _OPTIONAL: /* set copying/deleting options if we are called global */
		/* as parameter to a function we have got an ignore=1 before */
#warning FIXME: (optional) is only local
		if (current->parent->ignore == 0)
		{
		    /* Read in strings */
		    parameter = AllocVec(sizeof(struct ParameterList), MEMF_PUBLIC);
		    outofmem(parameter);
		    collect_stringargs(current, level, parameter);
		    /* Store data in preferences */
		    for (i = 0 ; i < parameter->intval ; i++)
		    {
			/* These are mutually exclusive */
			if (strcasecmp(parameter->arg[i], "fail") == 0)
			{
			    preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE);
			    preferences.copyfail |= COPY_FAIL;
			}
			if (strcasecmp(parameter->arg[i], "nofail") == 0)
			{
			    preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE);
			    preferences.copyfail |= COPY_NOFAIL;
			}
			if (strcasecmp(parameter->arg[i], "oknodelete") == 0)
			{
			    preferences.copyfail &= ~(COPY_FAIL | COPY_NOFAIL | COPY_OKNODELETE);
			    preferences.copyfail |= COPY_OKNODELETE;
			}

			/* These may be combined in any way */
			if (strcasecmp(parameter->arg[i], "force") == 0)
			{
			    preferences.copyflags |= COPY_ASKUSER;
			}
			if (strcasecmp(parameter->arg[i], "askuser") == 0)
			{
			    preferences.copyflags |= COPY_ASKUSER;
			}
		    }

		    free_parameterlist(parameter);
		}
		break;

#ifdef DEBUG
	    case _ALL:
	    case _APPEND:
	    case _ASSIGNS:
	    case _CHOICES:
	    case _COMMAND:
	    case _CONFIRM:
	    case _DEFAULT:
	    case _DEST:
	    case _DISK:
	    case _FILES:
	    case _FONTS:
	    case _HELP:
	    case _INCLUDE:
	    case _INFOS:
	    case _NEWNAME:
	    case _NEWPATH:
	    case _NOGAUGE:
	    case _NOPOSITION:
	    case _PATTERN:
	    case _PROMPT:
	    case _RANGE:
	    case _RESIDENT:
	    case _SAFE:
	    case _SETDEFAULTTOOL:
	    case _SETPOSITION:
	    case _SETSTACK:
	    case _SETTOOLTYPE:
	    case _SOURCE:
	    case _SWAPCOLORS:
	    case _QUIET:
		/* We are tags -- we don't want to be executed */
		current->parent->ignore = 1;
		break;
#endif /* DEBUG */

	    default:
#ifdef DEBUG
		/* Hey! Where did you get this number from??? It's invalid -- must be a bug. */
		fprintf(stderr, "Unknown command ID %d called <%s>!\n", cmd_type, current->arg);
		cleanup();
		exit(-1);
#else /* DEBUG */
		/* We are tags -- we don't want to be executed */
		current->parent->ignore = 1;
#endif /* DEBUG */
		break;
	}
    }
}


/*
 * Get an ID for the command string
 */
int eval_cmd(char * argument)
{
int i;

    if (argument[0] == SQUOTE || argument[0] == DQUOTE)
    {
	return _STRING;
    }
    else
    {
	for (i = 0 ; i < _MAXCOMMAND && strcasecmp(internal_commands[i].cmdsymbol, argument) != 0 ; i++);
	if (i != _MAXCOMMAND)
	{
	    return internal_commands[i].cmdnumber;
	}
	else
	{
	    if (find_proc(argument) != NULL)
	    {
		return _USERDEF;
	    }
	    else
	    {
		return _UNKNOWN;
	    }
	}
    }
}


/*
 * Callback function for RawDoFmt()
 */
static void callback(char chr, char ** data)
{
static int i = 0, j = 1;
static char * string = NULL;

    if (callbackstring != string)
    {
	string = callbackstring;
	i = 0;
	j = 1;
    }
    i++;
    if (i > MAXARGSIZE)
    {
	j++;
	i = 1;
	callbackstring = ReAllocVec(callbackstring, MAXARGSIZE * j, MEMF_PUBLIC);
	outofmem(callbackstring);
	globalstring += (callbackstring - string);
	string = callbackstring;
    }
    *(*data)++ = chr;
}


/*
 * Strip off quotes from a string
 * Does not check for quotes!
 */
char *strip_quotes(char *string)
{
int slen;
char *clip;

    /* Strip off quotes */
    slen = strlen(string);
    clip = (char *)AllocVec(slen - 1, MEMF_PUBLIC);
    outofmem(clip);
    strncpy(clip, string+1, slen-2);
    clip[slen-2] = 0;

return clip;
}


/*
 * Convert data entry to <int>
 * <string>s are atol()'d, <cmd>s are *not* executed
 */
long int getint(ScriptArg *argument)
{
long int i;
char * clip;

    if (argument->arg != NULL)
    {
	if ((argument->arg)[0] == SQUOTE || (argument->arg)[0] == DQUOTE)
	{
	    /* Strip off quotes */
	    clip = strip_quotes(argument->arg);
	    i = atol(clip);
	    FreeVec(clip);
	}
	else
	{
	    clip = get_var_arg(argument->arg);
	    if (clip != NULL)
	    {
		i = atol(clip);
	    }
	    else
	    {
		i = get_var_int(argument->arg);
	    }
	}
    }
    else
    {
	i = argument->intval;
    }

return i;
}


/*
 * Get an ID for hardware descriptor
 */
int database_keyword(char *name)
{
    if (strcasecmp(name, "vblank") == 0)
	return _VBLANK;
    else if (strcasecmp(name, "cpu") == 0)
	return _CPU;
    else if (strcasecmp(name, "graphics-mem") == 0)
	return _GRAPHICS_MEM;
    else if (strcasecmp(name, "total-mem") == 0)
	return _TOTAL_MEM;
    else if (strcasecmp(name, "fpu") == 0)
	return _FPU;
    else if (strcasecmp(name, "chiprev") == 0)
	return _CHIPREV;

return _UNKNOWN;
}


/*
 * Concatenate all arguments as a string with separating character
 * if character is 0 strings are concatenated without separator
 * <int>s are converted to strings, <cmd>s are executed,
 * <parameter>s are not considered
 */
char *collect_strings(ScriptArg *current, char separator, int level)
{
char *string = NULL, *clip, *dummy;
int i;

    while (current != NULL)
    {
	ExecuteCommand();
	/* Concatenate string unless it was a parameter which will be ignored */
	if (current->ignore == 0)
	{
	    if (current->arg != NULL)
	    {
		if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
		{
		    /* Strip off quotes */
		    clip = strip_quotes(current->arg);
		}
		else
		{
		    dummy = get_var_arg(current->arg);
		    if (dummy != NULL)
		    {
			clip = StrDup(dummy);
			outofmem(clip);
		    }
		    else
		    {
			clip = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
			outofmem(clip);
			sprintf(clip, "%ld", get_var_int(current->arg));
		    }
		}
	    }
	    else
	    {
		clip = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
		outofmem(clip);
		sprintf(clip, "%ld", current->intval);
	    }
	    i = (string == NULL) ? 0 : strlen(string);
	    string = ReAllocVec(string, i + strlen(clip) + 2, MEMF_PUBLIC);
	    outofmem(string);
	    if (i == 0)
	    {
		string[0] = 0;
	    }
	    else
	    {
		string[i] = separator;
		string[i+1] = 0;
	    }
	    strcat(string, clip);
	    FreeVec(clip);
	}
	current = current->next;
    }

return string;
}


/*
 * Free the allocated space for string list
 */
void free_parameter(struct ParameterList pl)
{
int j;

    if (pl.arg)
    {
	for (j = 0 ; j < pl.intval ; j++)
	{
	    FreeVec(pl.arg[j]);
	}
	FreeVec(pl.arg);
	pl.arg = NULL;
    }
}


/*
 * Free a complete (struct ParameterList *)
 */
void free_parameterlist(struct ParameterList *pl)
{
int i;

    if (pl)
    {
	for (i = 0 ; i < NUMPARAMS ; i++)
	{
	    free_parameter(pl[i]);
	}
	FreeVec(pl);
    }
}


/*
 * args are scanned for known parameters
 * the used entry will be set and <int>s and <string>s read in ParameterList
 * intval contains number of <string>s
 */
struct ParameterList *get_parameters(ScriptArg *script, int level)
{
struct ParameterList *pl;
ScriptArg *current;
long int i;
int cmd;
char *string, *clip;

    pl = AllocVec(NUMPARAMS * sizeof(struct ParameterList), MEMF_PUBLIC|MEMF_CLEAR);
    outofmem(pl);
    while (script != NULL)
    {
	/* Check if we have a command as argument */
	if (script->cmd != NULL)
	{
	    current = script->cmd->next;
	    /* Check if we don't have a block as argument */
	    if (script->cmd->arg != NULL)
	    {
		/* Check if we have a parameter as command */
		cmd = eval_cmd(script->cmd->arg);
		if (cmd > _PARAMETER && cmd <= (_PARAMETER + NUMPARAMS))
		{
		    /* This is a parameter */
		    GetPL(pl, cmd).used = 1;
		    if (cmd > (_PARAMETER + NUMARGPARAMS))
		    {
			/* This is a boolean parameter */
			GetPL(pl, cmd).intval = 1;
		    }
		    else
		    {
			/* This parameter may have arguments */
			switch (cmd)
			{
			    /* Parameters with args */
			    case _APPEND	: /* $ */
			    case _CHOICES	: /* $... */
			    case _COMMAND	: /* $... */
			    case _DELOPTS	: /* $... */
			    case _DEST	: /* $ */
			    case _HELP	: /* $... */
			    case _INCLUDE	: /* $ */
			    case _NEWNAME	: /* $ */
			    case _OPTIONAL	: /* $... */
			    case _PATTERN	: /* $ */
			    case _PROMPT	: /* $... */
			    case _SETDEFAULTTOOL: /* $ */
			    case _SETTOOLTYPE	: /* $ [$] */
			    case _SOURCE	: /* $ */
				collect_stringargs(current, level, &(GetPL(pl, cmd)));
				break;

			    case _CONFIRM: /* ($->)# */
				i = _EXPERT;
				if (current != NULL)
				{
				    ExecuteCommand();
				    if (current->arg != NULL)
				    {
					string = NULL;
					if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
					{
					    /* Strip off quotes */
					    string = strip_quotes(current->arg);
					    clip = string;
					}
					else
					{
					    clip = get_var_arg(current->arg);
					}
					if (clip != NULL)
					{
					    i = atol(clip);
					    if (strcasecmp(clip, "novice") == 0)
					    {
						i = _NOVICE;
					    }
					    if (strcasecmp(clip, "average") == 0)
					    {
						i = _AVERAGE;
					    }
					    if (strcasecmp(clip, "expert") == 0)
					    {
						i = _EXPERT;
					    }
					    FreeVec(string);
					}
					else
					{
					    i = get_var_int(current->arg);
					}
				    }
				    else
				    {
					i = current->intval;
				    }
				}
				if (i < _NOVICE || i > _EXPERT)
				{
				    error = BADPARAMETER;
				    traperr("Userlevel out of range!\n", NULL);
				}
				GetPL(pl, cmd).intval = i;
				break;

			    case _DEFAULT: /* * */
				i = 0;
				string = NULL;
				if (current != NULL)
				{
				    ExecuteCommand();
				    if (current->arg != NULL)
				    {
					if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
					{
					    /* Strip off quotes */
					    string = strip_quotes(current->arg);
					}
					else
					{
					    clip = get_var_arg(current->arg);
					    if (clip != NULL)
					    {
						string = StrDup(clip);
						outofmem(string);
					    }
					    else
					    {
						i = get_var_int(current->arg);
					    }
					}
				    }
				    else
				    {
					i = current->intval;
				    }
				    GetPL(pl, cmd).intval = i;
				    if (string != NULL)
				    {
					/* To avoid problems with multiple definitions of default */
					/* take last (this) one as true and clear previous	*/
					if (GetPL(pl, cmd).arg != NULL)
					{
					    FreeVec(GetPL(pl, cmd).arg[0]);
					}
					else
					{
					    GetPL(pl, cmd).arg = AllocVec(sizeof(char *), MEMF_PUBLIC);
					    outofmem(GetPL(pl, cmd).arg);
					}
					GetPL(pl, cmd).arg[0] = string;
				    }
				}
				else
				{
				    error = SCRIPTERROR;
				    traperr("<%s> requires one argument!\n", script->cmd->arg);
				}
				break;

			    case _RANGE	: /* # # */
			    case _SETPOSITION	: /* # # */
				i = 0;
				if (current != NULL && current->next != NULL)
				{
				    ExecuteCommand();
				    GetPL(pl, cmd).intval = getint(current);
				    current = current->next;
				    ExecuteCommand();
				    GetPL(pl, cmd).intval2 = getint(current);
				}
				else
				{
				    error = SCRIPTERROR;
				    traperr("<%s> requires two arguments!\n", script->cmd->arg);
				}
				break;

			    case _SETSTACK: /* # */
				i = 0;
				if (current != NULL)
				{
				    ExecuteCommand();
				    if (current->arg != NULL)
				    {
					if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
					{
					    /* Strip off quotes */
					    string = strip_quotes(current->arg);
					    i = atol(string);
					    FreeVec(string);
					}
					else
					{
					    clip = get_var_arg(current->arg);
					    if (clip != NULL)
					    {
						i = atol(clip);
					    }
					    else
					    {
						i = get_var_int(current->arg);
					    }
					}
				    }
				    else
				    {
					i = current->intval;
				    }
				    GetPL(pl, cmd).intval = i;
				}
				else
				{
				  error = SCRIPTERROR;
				  traperr("<%s> requires one argument!\n", script->cmd->arg);
				}
				break;


			    default: /* We do only collect tags -- this is a command */
				break;
			}
		    }
		}
		else if (cmd == _IF)
		{
		    /* This parameter is masqueraded */
		    switch (cmd)
		    {
			/* "Allowed" Commands (for compatibility) */

			/* (if (= 1 1) (command "blah")) is allowed, but should be
			 * (command (if (= 1 1) "blah"))
			 */
			case _IF: /* if 1st arg != 0 get parameter from 2nd cmd else get optional 3rd cmd parameter */
			    if (current != NULL && current->next != NULL)
			    {
				ExecuteCommand();
				i = getint(current);
				if (i == 0)
				{
				    current = current->next;
				}
				if (current->next != NULL)
				{
				    current = current->next;
				    if (current->cmd != NULL)
				    {
				    struct ParameterList *subpl;
					subpl = get_parameters(current, level + 1);
					for (i = 0 ; i < NUMPARAMS ; i++)
					{
					    if (subpl[i].used == 1)
					    {
						free_parameter(pl[i]);
						pl[i].arg = subpl[i].arg;
						pl[i].intval = subpl[i].intval;
						pl[i].intval2 = subpl[i].intval2;
						subpl[i].arg = NULL;
					    }
					}
					free_parameterlist(subpl);
				    }
				}
			    }
			    else
			    {
				error = SCRIPTERROR;
				traperr("<%s> requires two arguments!\n", script->arg);
			    }
			    break;

			default: /* We do only collect tags -- this is a command */
			    break;
		    }
		}
	    }
	}
	script = script->next;
    }

return pl;
}


/*
 * read <string>s in ParameterList
 * <int>s are converted, <cmd>s executed
 */
void collect_stringargs(ScriptArg *current, int level, struct ParameterList *pl)
{
char *string, *clip, **mclip = NULL;
int j = 0;

    while (current != NULL)
    {
	ExecuteCommand();
	mclip = ReAllocVec(mclip, sizeof(char *) * (j+1), MEMF_PUBLIC);
	outofmem(mclip);
	if (current->arg != NULL)
	{
	    if ((current->arg)[0] == SQUOTE || (current->arg)[0] == DQUOTE)
	    {
		/* Strip off quotes */
		string = strip_quotes(current->arg);
	    }
	    else
	    {
		clip = get_var_arg(current->arg);
		if (clip != NULL)
		{
		    string = StrDup(clip);
		    outofmem(string);
		}
		else
		{
		    clip = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
		    outofmem(clip);
		    sprintf(clip, "%ld", get_var_int(current->arg));
		    string = StrDup(clip);
		    outofmem(string);
		    FreeVec(clip);
		}
	    }
	}
	else
	{
	    clip = AllocVec(MAXARGSIZE, MEMF_PUBLIC);
	    outofmem(clip);
	    sprintf(clip, "%ld", current->intval);
	    string = StrDup(clip);
	    outofmem(string);
	    FreeVec(clip);
	}
	mclip[j] = string;
	j++;
	current = current->next;
    }
    pl->arg = mclip;
    pl->intval = j;
}


/*
 * Read in one line of a file
 */
char * get_file_line(BPTR file)
{
char *out;
char buf[1];
int i=0, cnt;

    do
    {
	cnt = Read(file, buf, 1);
	i += cnt;
    } while (cnt && buf[0] != LINEFEED);
    if (i == 0)
    {
	return NULL;
    }
    Seek(file, -i, OFFSET_CURRENT);
    out = AllocVec(i * sizeof(char), MEMF_PUBLIC);
    outofmem(out);
    Read(file, out, i);
    out[i-1] = 0;

return out;
}


/*
 * Routine for modifying S:User-Startup
 */
void modify_userstartup(char *string, struct ParameterList *parameter)
{
BPTR userstartup;
BPTR tmpuserstartup;
char *line;
int i, changed = 0, cont = 0;

    userstartup = Open("S:User-Startup", MODE_OLDFILE);
    tmpuserstartup = Open("S:User-Startup.tmp", MODE_NEWFILE);
    if (!tmpuserstartup)
    {
#warning TODO: Complain more smoothly...
	fprintf(stderr, "Could not open S:User-Startup for writing!");
	exit(-1);
    }
    if (userstartup)
    {
	while ((line = get_file_line(userstartup)) && !changed)
	{
	    if (strncasecmp(line, ";BEGIN ", 7) == 0)
	    {
		if (strcmp(&(line[7]), string) == 0)
		{
		    changed = 1;
		}
	    }
	    if (!changed)
	    {
		Write(tmpuserstartup, line, strlen(line));
		Write(tmpuserstartup, "\n", 1);
	    }
	    FreeVec(line);
	}
    }

    Write(tmpuserstartup, ";BEGIN ", 7);
    Write(tmpuserstartup, string, strlen(string));
    Write(tmpuserstartup, "\n", 1);
    for (i = 0 ; i < GetPL(parameter, _COMMAND).intval ; i++)
    {
	Write(tmpuserstartup, GetPL(parameter, _COMMAND).arg[i], strlen(GetPL(parameter, _COMMAND).arg[i]));
      }
    Write(tmpuserstartup, ";END ", 5);
    Write(tmpuserstartup, string, strlen(string));
    Write(tmpuserstartup, "\n", 1);

    if (userstartup)
    {
	while ((line = get_file_line(userstartup)))
	{
	    if (!cont)
	    {
		if (strncasecmp(line, ";END ", 5) == 0)
		{
		    if (strcmp(&(line[5]), string) == 0)
		    {
			cont = 1;
		    }
		}
	    }
	    else
	    {
		Write(tmpuserstartup, line, strlen(line));
		Write(tmpuserstartup, "\n", 1);
	    }
	    FreeVec(line);
	}
    }

    Close(tmpuserstartup);
    Close(userstartup);

    DeleteFile("S:User-Startup");
#warning FIXME: Check correctness of Rename()
/*
    IMO both arguments to Rename() should contain S:, check again if
    Rename() is proven to work as expected
*/
    if (Rename("S:User-Startup.tmp", "User-Startup") == DOSFALSE)
    {
	printf("Rename failed because of %s\n", DosGetString(IoErr()));
    }

}


/*
 * Execute "(traperr)" from preferences
 */
void traperr(char * msg, char * name)
{
char *outmsg;
int i, j;

    if (!doing_abort)
    {
	doing_abort = TRUE;

	i = (msg != NULL) ? strlen(msg) : 0 ;
	j = (name != NULL) ? strlen(name) : 0 ;
	outmsg = AllocVec(i + j + 1, MEMF_PUBLIC);
	sprintf(outmsg, msg, name);
	display_text(outmsg);

	if (preferences.trap[ error - 1 ].cmd != NULL)
	{
	    /* execute trap */
	    execute_script(preferences.trap[ error - 1 ].cmd, -99);
	}
	else
	{
	    /* execute onerrors */
	    if (preferences.onerror.cmd != NULL)
	    {
		execute_script(preferences.onerror.cmd, -99);
	    }
	}
    }

#ifdef DEBUG
    dump_varlist();
#endif /* DEBUG */

    cleanup();
    if (grace_exit == TRUE)
    {
	exit(0);
    }
    else
    {
	exit(-1);
    }
}


