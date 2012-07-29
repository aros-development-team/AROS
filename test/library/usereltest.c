/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pertask.h>
#include <proto/userel.h>

#include "pertaskvalue.h"

int main (int argc, char ** argv)
{
    IPTR vec[3] = {1, 2, 0};

    vec[2] = DummyAdd(vec[0], vec[1]);

    VPrintf((STRPTR)"%ld+%ld=%ld\n",vec);

    vec[0] = (IPTR)PertaskGetParentBase();

    VPrintf((STRPTR)"ParentBase = %ld\n",vec);

    Printf("101 202 303 404:\n");
    DummyPrint4(101, 202, 303, 404);

    pertaskvalue = 1;
    Printf("\n1 == %ld ?\n", PertaskGetValue());
    Printf("1 == %ld ?\n", GetChildValue());

    PertaskSetValue(2);
    Printf("\n2 == %ld ?\n", GetChildValue());

    Flush (Output ());
    
    return 0;
}
