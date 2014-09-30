/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/bptr.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static LONG get_default_stack_size()
{
    struct CommandLineInterface *cli = Cli();
    return cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
}

int main(int argc, char **argv)
{
    char *fname = "SYS:Utilities/Clock";
    char *full = "";
    int lastresult = RETURN_OK;
    
    if(fname) {
        BPTR seglist = LoadSeg(fname);
        if(seglist)
        {
            SetProgramName(fname);
            lastresult=RunCommand(seglist,get_default_stack_size(),
                    full,strlen(full));
            UnLoadSeg(seglist);
        }
    }
    
    exit(lastresult);
}
