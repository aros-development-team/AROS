/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Set CLI command.
*/

/*****************************************************************************

    NAME

        Set

    SYNOPSIS

        NAME,STRING/F

    LOCATION

        Workbench:c

    FUNCTION

        Set a local environment variable in the current shell. If any global
        variables have the same name the local variable will be used instead.

        This instance the variable is only accessible from within the shell
        it was defined.

        If no parameters are specified, the current list of local variables
        are displayed.

    INPUTS

        NAME    - The name of the local variable to set.

        STRING  - The value of the local variable NAME.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Set Jump 5

            Sets a local variable called "Jump" to the value of "5".

    BUGS

    SEE ALSO

        Get, Unset

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

static void GetNewString(STRPTR, STRPTR, LONG);

AROS_SH2(Set, 41.0,
AROS_SHA(STRPTR, ,NAME,    ,  NULL),
AROS_SHA(STRPTR, ,STRING, /F, NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process  * SetProc;
    struct LocalVar * SetNode;
    IPTR              OutArgs[3];
    LONG              VarLength;
    char              Buffer1[BUFFER_SIZE];
    char              Buffer2[BUFFER_SIZE];


    if (SHArg(NAME) != NULL || SHArg(STRING) != NULL)
    {
        /* Make sure we get to here is either arguments are
         * provided on the command line.
         */
        if (SHArg(NAME) != NULL && SHArg(STRING) != NULL)
        {
            /* Add the new local variable to the list.
             */
	    if
	    (
	        !SetVar(SHArg(NAME),
                        SHArg(STRING),
                        -1,
                        GVF_LOCAL_ONLY)
            )
	    {
	        return RETURN_ERROR;
            }
	}
    }
    else
    {
	SetProc = (struct Process *)FindTask(NULL);

	ForeachNode(&(SetProc->pr_LocalVars), SetNode)
        {
            if (SetNode->lv_Node.ln_Type == LV_VAR)
            {
                /* Get a clean variable with no excess
                 * characters.
                 */
                VarLength = -1;
                VarLength = GetVar(SetNode->lv_Node.ln_Name,
                                   &Buffer1[0],
                                   BUFFER_SIZE,
                                   GVF_LOCAL_ONLY
                );
                if (VarLength != -1)
                {
                    GetNewString(&Buffer1[0],
                                 &Buffer2[0],
                                 VarLength
                    );

                    Buffer2[VarLength] = NULL;

		    OutArgs[0] = (IPTR)SetNode->lv_Node.ln_Name;
                    OutArgs[1] = (IPTR)&Buffer2[0];
                    OutArgs[2] = (IPTR)NULL;
                    VPrintf("%-20s\t%-20s\n", &OutArgs[0]);
                }
            }
        }
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
} /* main */

static void GetNewString(STRPTR s, STRPTR d, LONG l)
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
