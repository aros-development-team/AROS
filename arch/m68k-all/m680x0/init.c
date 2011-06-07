
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "m680x0_intern.h"

static int M680x0Init(struct M680x0Base *M680x0Base)
{
    if (!(SysBase->AttnFlags & (AFF_68040 | AFF_68060)))
    	return FALSE; /* 68040/060 only need emulation */
    if (SysBase->AttnFlags & AFF_68882)
    	return FALSE; /* we already have full support? */
    if (!(SysBase->AttnFlags & AFF_FPU40))
    	return FALSE; /* no FPU, don't bother with missing instruction emulation */

    /* initialize emulation here */

    /* emulation installed, full 68881/68882 now supported  */
    SysBase->AttnFlags |= AFF_68881 | AFF_68882;
    return TRUE;
}

ADD2INITLIB(M680x0Init, 0)


