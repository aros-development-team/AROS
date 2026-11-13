/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pertask.h>
#include <proto/userel.h>

#include "pertaskvalue.h"

int main (int argc, char ** argv)
{
    ULONG vec[3] = {1, 2, 0};
    VOID *parent;

    vec[2] = DummyAdd(vec[0], vec[1]);

    Printf("%ld+%ld=%ld\n",vec[0], vec[1], vec[2]);

    parent = PertaskGetParentBase();

    Printf("ParentBase = %p\n",parent);

    Printf("104 205 306 407:\n");
    DummyPrint4(101, 202, 303, 404);

    pertaskvalue = 1;
    Printf("\n1 == %ld ?\n", PertaskGetValue());
    Printf("1 == %ld ?\n", GetChildValue());

    PertaskSetValue(2);
    Printf("\n2 == %ld ?\n", GetChildValue());

    Flush (Output ());
    
    return 0;
}
