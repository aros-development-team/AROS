/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Why CLI command
    Lang: english
*/

#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

static const char version[] = "$VER: why 41.2 (2.3.97)\n";

int main()
{
    struct RDArgs *rda;
    IPTR args[0];
    struct CommandLineInterface *cli;
    LONG lasterror;
    int error = RETURN_OK;

    rda = ReadArgs("", args, NULL);
    if (rda != NULL)
    {
        if ((cli = Cli()) != NULL)
        {
            lasterror = cli->cli_Result2;
            if (lasterror == 0)
                VPrintf("The last command did not set a return-value\n", NULL);
            else
            {
                PrintFault(lasterror, "The last command failed, reason");
                SetIoErr(0);
            }
        } else
            error = RETURN_FAIL;
    } else
    {
        PrintFault(IoErr(), "Why");
        error = RETURN_FAIL;
    }
    return(error);
}
