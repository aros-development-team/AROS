/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/peropener.h>
#include <proto/perid.h>

int main (int argc, char ** argv)
{
    struct Library *base1, *base2;

    FPuts(Output(), (STRPTR)"Testing peropener.library\n");
    
    base1=OpenLibrary((STRPTR)"peropener.library",0);
    base2=OpenLibrary((STRPTR)"peropener.library",0);

    FPrintf(Output(), (STRPTR)"base1=%lx, base2=%lx\n", base1, base2);
    
    if (base1 != NULL)
        CloseLibrary(base1);
    if (base2 != NULL)
        CloseLibrary(base2);

    FPuts(Output(), (STRPTR)"\nTesting perid.library\n");

    base1=OpenLibrary((STRPTR)"perid.library",0);
    base2=OpenLibrary((STRPTR)"perid.library",0);

    FPrintf(Output(), (STRPTR)"base1=%lx, base2=%lx\n", base1, base2);
    
    if (base1 != NULL)
        CloseLibrary(base1);
    if (base2 != NULL)
        CloseLibrary(base2);

    Flush (Output ());
    
    return 0;
}
