/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/09/17 16:43:00  digulla
    Use general startup code

    Revision 1.3  1996/09/12 14:48:55  digulla
    Tests why it didn´t work

    Revision 1.2  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/aros_protos.h>
#include "dummylib_gcc.h"

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
