#include <dos/dostags.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>

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