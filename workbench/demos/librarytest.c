/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include "dummylib_gcc.h"

CALLENTRY /* Before the first symbol */

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

LONG entry(struct ExecBase *sysbase)
{
    SysBase=sysbase;

    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39);
    if(DOSBase!=NULL)
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

            CloseLibrary((struct Library *)dummybase);
        }
        CloseLibrary((struct Library *)DOSBase);
    }
    return 0;
}
