/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dummy.h>

int main (int argc, char ** argv)
{
    ULONG a=1,b=2,c=0,d=0;

    c=add(a,b);

    d=asl(a,b);

    Printf((STRPTR)"%ld+%ld=%ld\n",a, b ,c);

    Printf((STRPTR)"%ld<<%ld=%ld\n",a, b, d);

    Flush (Output ());
    
    return 0;
}
