/*
    Copyright © 2008-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/peropener.h>
#include <proto/pertask.h>

int main (int argc, char ** argv)
{
    struct Library *base1, *base2;
    BPTR seglist;
    
    FPuts(Output(), (STRPTR)"Testing peropener.library\n");
    
    base1=OpenLibrary((STRPTR)"peropener.library",0);
    base2=OpenLibrary((STRPTR)"peropener.library",0);

    FPrintf(Output(), (STRPTR)"base1=%lx, base2=%lx\n", base1, base2);
    
    if (base1 != NULL)
        CloseLibrary(base1);
    if (base2 != NULL)
        CloseLibrary(base2);

    FPuts(Output(), (STRPTR)"\nTesting pertask.library\n");

    base1=OpenLibrary((STRPTR)"pertask.library",0);
    base2=OpenLibrary((STRPTR)"pertask.library",0);
    
    FPrintf(Output(), (STRPTR)"base1=%lx, base2=%lx\n", base1, base2);

    seglist = LoadSeg((CONST_STRPTR)"peropenertest_child");
    if (seglist != (BPTR)NULL)
    {
        SetProgramName("peropenertest_child");
        RunCommand(seglist, 10*1024, "\n", 1);
        UnLoadSeg(seglist);
    }
    else
    {
        FPrintf(Output(), (STRPTR)"Failed to load peropenertest_child\n");
    }
    
    if (base1 != NULL)
        CloseLibrary(base1);
    if (base2 != NULL)
        CloseLibrary(base2);

    Flush (Output ());
    
    return 0;
}
