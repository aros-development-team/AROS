/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 15:32:25  digulla
    Externel reference to DOSBase was missing

    Revision 1.2  1996/08/01 17:40:37  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

extern struct DosBase * DOSBase;

int main(void)
{
    BPTR segs;

    segs=LoadSeg("c/shell");
    if(segs)
    {
	RunCommand(segs,4096,"FROM S:Startup-Sequence",23);
	UnLoadSeg(segs);
    }
    return 0;
}
