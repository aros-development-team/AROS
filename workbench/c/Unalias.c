/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Unalias CLI command
    Lang: english
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

    HISTORY

        30-Jul-1997     laguest     Initial inclusion into the AROS tree

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

#include <strings.h>

#define ARG_TEMPLATE    "NAME"
#define ARG_NAME        0
#define TOTAL_ARGS      1

#define BUFFER_SIZE     160

static const char version[] = "$VER: Unalias 41.0 (27.07.1997)\n";

void GetNewString(STRPTR, STRPTR, LONG);

int __nocommandline;

int main(void)
{
    struct RDArgs   * rda;
    struct Process  * UnaliasProc;
    struct LocalVar * UnaliasNode;
    IPTR            * args[TOTAL_ARGS] = { NULL };
    IPTR              OutArgs[4];
    int               Return_Value;
    BOOL              Success;
    LONG              VarLength;
    char              Buffer1[BUFFER_SIZE];
    char              Buffer2[BUFFER_SIZE];

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);
    if (rda)
    {
        if (args[ARG_NAME] != NULL)
        {
            Success = DeleteVar((STRPTR)args[ARG_NAME],
                                GVF_LOCAL_ONLY | LV_ALIAS
            );
            if (Success == FALSE)
            {
                Return_Value = RETURN_WARN;
                PrintFault(IoErr(), "Unalias");
            }
        }
        else
        {
            /* Display a list of aliases.
             */
            Forbid();
            UnaliasProc = (struct Process *)FindTask(NULL);
            Permit();

            if (UnaliasProc != NULL)
            {
                ForeachNode((struct List *)&(UnaliasProc->pr_LocalVars),
                            (struct Node *)UnaliasNode
                )
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
                                           GVF_LOCAL_ONLY | LV_ALIAS
                        );
                        if (VarLength != -1)
                        {
                            GetNewString(&Buffer1[0],
                                         &Buffer2[0],
                                         VarLength
                            );

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
    }
    else
    {
        PrintFault(IoErr(), "Unalias");

        Return_Value = RETURN_ERROR;
    }

    if (rda)
        FreeArgs(rda);

    return (Return_Value);

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
