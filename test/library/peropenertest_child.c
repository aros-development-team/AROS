/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <proto/dos.h>
#include <proto/pertask.h>

int main (int argc, char ** argv)
{
    FPuts(Output(), (STRPTR)"\nTesting pertask.library in child\n");

    FPrintf(Output(), (STRPTR)"base=%lx, parent=%lx, parent2=%lx\n", PertaskBase,
            GetParentBase(), GetParentBase2()
    );
    
    return 0;
}
