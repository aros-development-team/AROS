/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
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
	IPTR vec[3];

	c=add(a,b);

	d=asl(a,b);

	vec[0]=a;
	vec[1]=b;
	vec[2]=c;
	VPrintf((STRPTR)"%ld+%ld=%ld\n",vec);

	vec[0]=a;
	vec[1]=b;
	vec[2]=d;
	VPrintf((STRPTR)"%ld<<%ld=%ld\n",vec);

	CloseLibrary((struct Library *)DummyBase);
    }
    else
        FPuts(Output(),(STRPTR)"Failed to open dummy.library");

    Flush (Output ());
    
    return 0;
}
