/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Alias CLI command.
*/

/*****************************************************************************

    NAME

        Alias

    SYNOPSIS

        NAME,STRING/F

    LOCATION

        Workbench:c

    FUNCTION

        Alias allows you to create an alternate name for other DOS commands.
        If Alias is used with no parameters, it will display the current
        list of Aliases defined within the current shell.

        Using a pair of square brackets within an alias allows you to
        provide the 'new' dos command with parameters.

        If no parameters are specified, the current list of aliases are
        displayed.

    INPUTS

        NAME    - The name of the alias to set.

        STRING  - The value of the alias NAME.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Alias DF "Type [] number"

            By typing "DF S:Shell-Startup" in the shell, you are actually
            executing the command "Type S:Shell-Startup number". This will
            display the contents of the S:Shell-Startup file in the shell
            with line numbers on the left hand side.

    BUGS

    SEE ALSO

        Unalias

    INTERNALS

******************************************************************************/


#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include <aros/shcommands.h>

#define BUFFER_SIZE     160

void GetNewString(STRPTR, STRPTR, LONG);

AROS_SH2(Alias, 41.0,
AROS_SHA(STRPTR, ,NAME, ,NULL),
AROS_SHA(STRPTR, ,STRING,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process  *AliasProc;
    struct LocalVar *AliasNode;
    char             Buffer[BUFFER_SIZE];
    IPTR             OutArgs[3];
    int              Return_Value = RETURN_OK;
    BOOL             Success;
    LONG             VarLength;
    char             Buffer1[BUFFER_SIZE];
    char             Buffer2[BUFFER_SIZE];


    if (SHArg(NAME) != NULL || SHArg(STRING) != NULL)
    {
        /* Make sure we get to here is either arguments are
         * provided on the command line.
         */
        if (SHArg(NAME) != NULL && SHArg(STRING) == NULL)
        {
            Success = GetVar(SHArg(NAME), &Buffer[0],
		             BUFFER_SIZE, GVF_LOCAL_ONLY | LV_ALIAS);

            if (Success == FALSE)
            {
                Return_Value = RETURN_WARN;
                PrintFault(IoErr(), "Alias");
            }
            else
            {
                OutArgs[0] = (IPTR)&Buffer[0];
                OutArgs[1] = (IPTR)NULL;
                VPrintf("%s\n", &OutArgs[0]);
            }
        }
        else
        {
            /* Add the new local variable to the list. */
            Success = SetVar(SHArg(NAME), SHArg(STRING),
                             -1, GVF_LOCAL_ONLY | LV_ALIAS);

            if (Success == FALSE)
            {
                PrintFault(IoErr(), "Alias");
                Return_Value = RETURN_ERROR;
            }
        }
    }
    else
    {
        /* Display a list of aliases. */
        AliasProc = (struct Process *)FindTask(NULL);

	ForeachNode(&(AliasProc->pr_LocalVars), AliasNode)
        {
	    if (AliasNode->lv_Node.ln_Type == LV_ALIAS)
	    {
		/* Get a clean variable with no excess
		 * characters.
		 */
		VarLength = GetVar(AliasNode->lv_Node.ln_Name,
		 	            &Buffer1[0],
				    BUFFER_SIZE,
				    GVF_LOCAL_ONLY | LV_ALIAS);
		if (VarLength != -1)
		{
		     GetNewString(&Buffer1[0], &Buffer2[0], VarLength);

		     Buffer2[VarLength] = NULL;

		     OutArgs[0] = (IPTR)AliasNode->lv_Node.ln_Name;
		     OutArgs[1] = (IPTR)&Buffer2[0];
		     OutArgs[2] = (IPTR)NULL;
		     VPrintf("%-20s\t%-20s\n", &OutArgs[0]);
		}
	    }
	}
    }

    return Return_Value;

    AROS_SHCOMMAND_EXIT
} /* main */


void GetNewString(STRPTR s, STRPTR d, LONG l)
{
    int i;
    int j;

    i = j = 0;

    while (i < l)
    {
        if (s[i] == '*' || s[i] == '\e')
        {
            d[j] = '*';

            i++;
            j++;
        }
        else
        {
            d[j] = s[i];

            i++;
            j++;
        }
    }
} /* GetNewString */
