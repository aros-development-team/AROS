/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Unalias CLI command.
*/

/*****************************************************************************

    NAME

        Unalias

    SYNOPSIS

        NAME

    LOCATION

        Workbench:c

    FUNCTION

        Removes a previously defined shell alias.

        If no parameters are specified, the current list of aliases are
        displayed.

    INPUTS

        NAME    - The name of the alias to unset.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Unalias DF

    BUGS

    SEE ALSO

        Alias

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
#include <strings.h>

#include <aros/shcommands.h>

#define BUFFER_SIZE 160

void GetNewString(STRPTR, STRPTR, LONG);

AROS_SH1(Unalias, 41.0,
AROS_SHA(STRPTR, ,NAME, ,NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process  * UnaliasProc;
    struct LocalVar * UnaliasNode;
    IPTR              OutArgs[4];
    BOOL              Success;
    LONG              VarLength;
    char              Buffer1[BUFFER_SIZE];
    char              Buffer2[BUFFER_SIZE];


    if (SHArg(NAME) != NULL)
    {
        Success = DeleteVar(SHArg(NAME), GVF_LOCAL_ONLY | LV_ALIAS);

        if (Success == FALSE)
        {
            PrintFault(IoErr(), "Unalias");

	    return RETURN_WARN;
        }
    }
    else
    {
        /* Display a list of aliases.
         */
        UnaliasProc = (struct Process *)FindTask(NULL);

        if (UnaliasProc != NULL)
        {
            ForeachNode(&(UnaliasProc->pr_LocalVars), UnaliasNode)
            {
                if (UnaliasNode->lv_Node.ln_Type == LV_ALIAS)
                {
                    /* Get a clean variable with no excess
                     * characters.
                     */
                    VarLength = -1;
                    VarLength = GetVar(UnaliasNode->lv_Node.ln_Name,
                                       &Buffer1[0],
                                       BUFFER_SIZE,
                                       GVF_LOCAL_ONLY | LV_ALIAS);

                    if (VarLength != -1)
                    {
                        GetNewString(&Buffer1[0],
                                     &Buffer2[0],
                                     VarLength);

                        Buffer2[VarLength] = NULL;
			
                        OutArgs[0] = (IPTR)UnaliasNode->lv_Node.ln_Name;
                        OutArgs[1] = (IPTR)&Buffer2[0];
                        OutArgs[2] = (IPTR)NULL;
                        VPrintf("%-20s\t%-20s\n", &OutArgs[0]);
                    }
                }
            }
        }
    }

    return RETURN_OK;

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
