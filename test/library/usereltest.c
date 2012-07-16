/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/userel.h>

int main (int argc, char ** argv)
{
    IPTR vec[3] = {1, 2, 0};

    vec[2] = DummyAdd(vec[0], vec[1]);

    VPrintf((STRPTR)"%ld+%ld=%ld\n",vec);

    vec[0] = (IPTR)PertaskGetParentBase();

    VPrintf((STRPTR)"ParentBase = %ld\n",vec);

    Flush (Output ());
    
    return 0;
}
