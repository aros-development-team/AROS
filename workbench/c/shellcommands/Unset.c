/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Unset CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Unset

    SYNOPSIS

        NAME

    LOCATION

        Workbench:c

    FUNCTION

    INPUTS

        NAME    - The name of the local variable to unset.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define AROS_ALMOST_COMPATIBLE

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include "shcommands.h"

#define BUFFER_SIZE     160

static void GetNewString(STRPTR, STRPTR, LONG);

AROS_SH1(Unset, 41.0, 27.07.1997,
AROS_SHA(,NAME, ,NULL))
{
    AROS_SHCOMMAND_INIT

    struct Process  * UnsetProc;
    struct LocalVar * UnsetNode;
    IPTR              OutArgs[3];
    LONG              VarLength;
    char              Buffer1[BUFFER_SIZE];
    char              Buffer2[BUFFER_SIZE];

    if (SHArg(NAME) != NULL)
    {
        /* Delete the local Var from the list.
         */

	 if (!DeleteVar((STRPTR)SHArg(NAME), GVF_LOCAL_ONLY))
             SHReturn(RETURN_FAIL);

    }
    else
    {
        /* Display a list of local variables.
        */
        UnsetProc = (struct Process *)FindTask(NULL);

        ForeachNode((struct List *)&(UnsetProc->pr_LocalVars),
                    (struct Node *)UnsetNode
        )
        {
            if (UnsetNode->lv_Node.ln_Type == LV_VAR)
            {
                /* Get a clean variable with no excess
                 * characters.
                 */
                 VarLength = -1;
                 VarLength = GetVar(UnsetNode->lv_Node.ln_Name,
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

                     OutArgs[0] = (IPTR)UnsetNode->lv_Node.ln_Name;
                     OutArgs[1] = (IPTR)&Buffer2[0];
                     OutArgs[2] = (IPTR)NULL;
                     VPrintf("%-20s\t%-20s\n", &OutArgs[0]);
                 }
            }
       }
    }

    SHReturn(RETURN_OK);

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
