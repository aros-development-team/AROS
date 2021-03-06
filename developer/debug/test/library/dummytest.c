/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/dummy.h>

int main (int argc, char ** argv)
{
    ULONG a=1,b=2,c=0,d=0;
    struct Library *DummyBase;

    DummyBase=OpenLibrary((STRPTR)"dummy.library",0);

    if(DummyBase!=NULL)
    {
        c=add(a,b);

        d=asl(a,b);

        Printf((STRPTR)"%ld+%ld=%ld\n", a, b, c);

        Printf((STRPTR)"%ld<<%ld=%ld\n", a, b, d);

        CloseLibrary((struct Library *)DummyBase);
    }
    else
        FPuts(Output(),(STRPTR)"Failed to open dummy.library");

    Flush (Output ());
    
    return 0;
}
