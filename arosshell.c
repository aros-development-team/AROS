/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/23 14:09:45  aros
    Use the systems' stacksize

    Revision 1.5  1996/10/10 13:11:01  digulla
    There is no such symbol as DOSBase (Fleischer)

    Revision 1.4  1996/09/12 14:47:52  digulla
    More stack

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

extern struct DosLibrary *DOSBase;

int main(void)
{
    BPTR segs;

    segs=LoadSeg("c/shell");
    if(segs)
    {
	RunCommand(segs,AROS_STACKSIZE,"FROM S:Startup-Sequence",23);
	UnLoadSeg(segs);
    }
    return 0;
}
