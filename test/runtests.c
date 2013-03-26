/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/******************************************************************************

    NAME

        runtests

    SYNOPSIS

        scriptname

    LOCATION

        SYS:Tests

    FUNCTION

        Executes all commands given by an input script. Reports memory loss
        and summarizes the return codes. The commands shouldn't do any
        output on success to avoid increasing of the shell's buffer.
        The result is printed to the debugging console.
        
    INPUTS

        scriptname -- script with the programs to be executed. Defaults to
        "testscript".

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>

BPTR scripthandle;
CONST_STRPTR scriptname;
UBYTE command[200];
LONG error;
LONG failcnt;
LONG errorcnt;
LONG warncnt;
LONG okcnt;
LONG noshellcnt;
LONG rubbishcnt;

static ULONG checkmem(void)
{
    FreeVec(AllocVec((ULONG)(~0ul/2), MEMF_ANY)); // trigger expunges
    return AvailMem(MEMF_ANY);
}

int main(int argc, char **argv)
{
    ULONG mem_old, mem;

    if (argc == 1)
    {
        scriptname = "testscript";
    }
    else if (argc == 2)
    {
        scriptname = argv[1];
    }
    else
    {
        PutStr("Usage runtest [scriptfile]\n");
    }

    scripthandle = Open(scriptname, MODE_OLDFILE);
    if (!scripthandle)
    {
        PutStr("Can't open file\n");
        return 0;
    }

    PutStr("Reading commands from file ");
    PutStr(scriptname);
    PutStr("\nOutput will be sent to the debugging console\n\n");

    mem_old = checkmem();

    while (FGets(scripthandle, command, sizeof command))
    {
        if (command[0] != '#' && command[0] != '\n')
        {
            bug("====================================\n");
            bug("Running command: %s", command);
            error = SystemTagList(command, NULL);
            bug("returns: %d\n", error);

            mem = checkmem();
            if (mem != mem_old)
            {
                bug("Memory loss %ul Bytes\n", mem_old - mem);
                mem_old = mem;
            }

            if (error == -1)
                noshellcnt++;
            else if (error > 100 || error < 0)
                rubbishcnt++;
            else if (error >= RETURN_FAIL)
                failcnt++;
            else if (error >= RETURN_ERROR)
                errorcnt++;
            else if (error >= RETURN_WARN)
                warncnt++;
            else
                okcnt++;
        }
    }
    bug("====================================\n");
    bug("Summary: ok %d, warn %d, error %d, fail %d, no Shell %d, rubbish %d\n",
        okcnt, warncnt, errorcnt, failcnt, noshellcnt, rubbishcnt);

    Close(scripthandle);

    return 0;
}
