/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc:
*/

/******************************************************************************

    NAME

        Skip

    SYNOPSIS

        LABEL, BACK/S

    LOCATION

        C:

    FUNCTION

        Jump to a new position in a script. If a label is specified, control
        goes to the first Lab command found that has the same label. If no
        label is specified, control goes to the first EndSkip command found.

        If the BACK switch is given, the search for a matching Lab or
        EndSkip command starts at the beginning of the script; otherwise the
        search starts at the Skip command. If a matching Lab/EndSkip is not
        found, an error is returned.

    INPUTS

        LABEL  --  The label to skip to.

        BACK   --  Specify this if the label appears before the Skip statement
                   in the script file.

    RESULT

    NOTES
        This command can only be used in scripts.

    EXAMPLE

    BUGS

    SEE ALSO

       Lab, EndSkip

    INTERNALS

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include "dos_commanderrors.h"
#include <dos/stdio.h>

#include <aros/shcommands.h>

AROS_SH2(Skip, 41.2,
AROS_SHA(STRPTR, , LABEL,  , NULL),
AROS_SHA(BOOL,   , BACK, /S, FALSE))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    BOOL                  labelFound = FALSE;


    if (cli == NULL || cli->cli_CurrentInput == cli->cli_StandardInput)
    {
        PrintFault(ERROR_SCRIPT_ONLY, "Skip");

        return RETURN_FAIL;
    }

    {
        char  buffer[256];
        int   a = 0;
        LONG  status;
        BOOL  quit = FALSE;

        SelectInput(cli->cli_CurrentInput);

        if (SHArg(BACK))
        {
            Flush(Input());
            Seek(Input(), 0, OFFSET_BEGINNING);
        }

        while (!quit)
        {
            status = ReadItem(buffer, sizeof(buffer), NULL);

            if (status == ITEM_ERROR)
            {
                break;
            }

            if (status != ITEM_NOTHING)
            {
                switch (FindArg("LAB,ENDSKIP", buffer))
                {
                    case 0:
                        if (SHArg(LABEL) != NULL)
                        {
                            ReadItem(buffer, sizeof(buffer), NULL);

                            if (FindArg(SHArg(LABEL), buffer) == 0)
                            {
                                quit = TRUE;
                                labelFound = TRUE;
                            }
                        }
                        break;

                    case 1:
                        quit = TRUE;
                        break;
                }
            }

            /* Skip to the next line */
            do
            {
                a = FGetC(Input());
            } while (a != '\n' && a != ENDSTREAMCH);

            if (a == ENDSTREAMCH)
                break;
        }
    }

    if (!labelFound && SHArg(LABEL) != NULL)
    {
        SetIoErr(ERROR_OBJECT_NOT_FOUND);
        PrintFault(ERROR_OBJECT_NOT_FOUND, "Skip");

        return RETURN_FAIL;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
