/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <proto/dos.h>
#include <proto/perid.h>

int main (int argc, char ** argv)
{
    FPuts(Output(), (STRPTR)"\nTesting perid.library in child\n");

    FPrintf(Output(), (STRPTR)"base=%lx, parent=%lx\n", PeridBase,
            GetParentBase()
    );
    
    return 0;
}
