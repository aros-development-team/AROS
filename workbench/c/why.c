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

static const char version[] = "$VER: Why 1.0 (28.2.97)\n";

int main()
{
    struct Process * proc;
    struct CommandLineInterface * cli;
    LONG lasterror;
    int error = RETURN_OK;

    proc = (struct Process *)FindTask(NULL);
    cli = Cli();
    if (cli != NULL)
    {
        lasterror = cli->cli_Result2;
        if (lasterror == 0)
            printf("The last command did not set a return-value\n");
        else
            PrintFault(lasterror, "The last command failed, reason");
    } else
        error = RETURN_FAIL;
    return(error);
}
