  /*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Setenv CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Setenv

    SYNOPSIS

        NAME,STRING/F

    LOCATION

        Workbench:c

    FUNCTION

        Sets a global variable from the current shell. These variables can
        be accessed from any program executing at any time.

        These variables are not saved in the ENVARC: directory, hence they
        can only be used by programs during the current execution of the
        operating system.

        If no parameters are specified, the current list of global variables
        are displayed.

    INPUTS

        NAME    - The name of the global variable to set.

        STRING  - The value of the global variable NAME.

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

        Setenv EDITOR Ed

            Any program that accesses the variable "EDITOR" will be able to
            find out the name of the text-editor the user would like to use,
            by examining the contents of the variable.

    BUGS

    SEE ALSO

        Getenv, Unsetenv

    INTERNALS

    HISTORY

        30-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <exec/types.h>

#define ARG_TEMPLATE    "NAME,STRING/F"
#define ARG_NAME        0
#define ARG_STRING      1
#define TOTAL_ARGS      2

static const char version[] = "$VER: Setenv 41.0 (27.07.1997)\n";

int main(int argc, char *argv[])
{
	struct RDArgs       * rda;
	struct ExAllControl * eac;
    struct ExAllData    * ead;
    struct ExAllData    * eaData;
    IPTR                * args[TOTAL_ARGS] = { NULL, NULL };
    int                   Return_Value;
    BOOL                  Success;
    BOOL                  More;
    BPTR                  Lock;

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
    if (rda)
    {
        if (args[ARG_NAME] != NULL || args[ARG_STRING] != NULL)
        {
            /* Make sure we get to here is either arguments are
             * provided on the command line.
             */
            if (args[ARG_NAME] != NULL && args[ARG_STRING] != NULL)
            {
                /* Add the new global variable to the list.
                 */
                Success = SetVar((STRPTR)args[ARG_NAME],
                                 (STRPTR)args[ARG_STRING],
                                 -1,
                                 GVF_GLOBAL_ONLY
                );
                if (Success == FALSE)
                {
                    PrintFault(IoErr(), "Setenv");
                    Return_Value = RETURN_ERROR;
                }
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
                PrintFault(IoErr(), "Setenv");

                Return_Value = RETURN_FAIL;
            }
        }
    }
    else
    {
        PrintFault(IoErr(), "Setenv");

        Return_Value = RETURN_ERROR;
    }

    FreeArgs(rda);

    return (Return_Value);

} /* main */
