/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include "dummylib_gcc.h"

static const char version[] = "$VER: librarytest 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    int a=1,b=2,c=0,d=0;
    struct dummybase *dummybase;

    dummybase=(struct dummybase *)OpenLibrary("dummy.library",0);

    if(dummybase!=NULL)
    {
	ULONG vec[3];

	c=add(a,b);

	d=asl(a,b);

	vec[0]=a;
	vec[1]=b;
	vec[2]=c;
	VPrintf("%ld+%ld=%ld\n",vec);

	vec[0]=a;
	vec[1]=b;
	vec[2]=d;
	VPrintf("%ld<<%ld=%ld\n",vec);

	Flush (Output ());

	CloseLibrary((struct Library *)dummybase);
    }

    return 0;
}
