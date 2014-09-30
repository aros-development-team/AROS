/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <aros/debug.h>

#include <proto/version.h>
#include <proto/exec.h>

#include <stdlib.h>

int main(int argc, char **argv)
{
    LONG retval = RETURN_ERROR;
    ULONG version = 0;
    
    if (argc == 2)
    {
        version = atoi(argv[1]);
    }

    struct Library *VersionBase = OpenLibrary("version.library", version);
    if (VersionBase)
    {
        if (VersionBase->lib_Version == 40)
        {
            retval = RETURN_OK;
        }
        else
        {
            bug("lib_Version != 40\n");
        }
        CloseLibrary(VersionBase);
    }
    else
    {
        bug("Can't open version.library v%u\n", version);
    }
    return retval;
}
