/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Unsetenv CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Unsetenv

    SYNOPSIS

        NAME

    LOCATION

        Workbench:c

    FUNCTION

    INPUTS

        NAME    - The name of the global variable to unset.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <utility/tagitem.h>

#define ARG_TEMPLATE    "NAME"
#define ARG_NAME        0
#define TOTAL_ARGS      1

static const char version[] = "$VER: Unsetenv 41.0 (27.07.1997)\n";

int main(int argc, char *argv[])
{
	struct RDArgs       * rda;
	struct ExAllControl * eac;
    struct ExAllData    * ead;
    struct ExAllData    * eaData;
    IPTR                * args[TOTAL_ARGS] = { NULL };
    int                   Return_Value;
    BOOL                  Success;
    BOOL                  More;
    BPTR                  Lock;

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
    if (rda)
    {
        if (args[ARG_NAME] != NULL)
        {
            /* Add the new global variable to the list.
             */
            Success = DeleteVar((STRPTR)args[ARG_NAME],
                                GVF_GLOBAL_ONLY
            );
            if (Success == FALSE)
            {
                PrintFault(IoErr(), "Unsetenv");
                Return_Value = RETURN_ERROR;
            }
        }
        else
        {
            /* Display a list of global variables.
             */
            Lock = Lock("ENV:", ACCESS_READ);
            if (Lock)
            {
                eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
                if (eac)
                {
                    eaData = AllocMem(4096, MEMF_ANY | MEMF_CLEAR);
                    if (eaData)
                    {
                        eac->eac_LastKey = 0;
                        
                        do
                        {
                            More = ExAll(Lock, eaData, 4096, ED_TYPE, eac);
                            if ((!More) && (IoErr() != ERROR_NO_MORE_ENTRIES))
                            {
                                /* Failed abnormally
                                 */
                                break;
                            }

                            if (eac->eac_Entries == 0)
                            {
                                /* Failed normally.
                                 */
                                continue;
                            }

                            ead = (struct ExAllData *)eaData;
                            do
                            {
                                if (ead->ed_Type < 0)
                                {
                                    FPuts(Output(), ead->ed_Name);
                                    FPutC(Output(), '\n');
                                }
                                ead = ead->ed_Next;
                            }
                            while (ead);
                        }
                        while (More);
                        
                        FreeMem(ead, 4096);
                    }
                
                    FreeDosObject(DOS_EXALLCONTROL, eac);
                }
            
                UnLock(Lock);
            }
            else
            {
                PrintFault(IoErr(), "Unsetenv");

                Return_Value = RETURN_FAIL;
            }
        }
    }
    else
    {
        PrintFault(IoErr(), "Unsetenv");

        Return_Value = RETURN_ERROR;
    }

    if (rda) FreeArgs(rda);

    return (Return_Value);

} /* main */
