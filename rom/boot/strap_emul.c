/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Try and boot an emulated AROS under Unix
    Lang: english
*/

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <dos/filehandler.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>

#include <proto/exec.h>
#include <proto/expansion.h>

int boot_entry()
{
    return -1;
}

static const char boot_end;
int AROS_SLIB_ENTRY(init,boot)();

const struct Resident boot_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&boot_resident,
    (APTR)&boot_end,
    RTF_COLDSTART,
    41,
    NT_PROCESS,
    -50,
    "Boot_Strap",
    "AROS Boot Strap (Emulated) 41.1 (24.8.1997)\r\n",
    (APTR)&boot_init
};

AROS_LH2(int, init,
    AROS_LHA(ULONG, dummy, D0),
    AROS_LHA(ULONG, seglist, A0),
    struct ExecBase *, SysBase, 0, boot)
{
    AROS_LIBFUNC_INIT
    
    /*  We have been run at a low priority, after emul.handler,
        but before DOS. We have to create get DOS to boot off 
        the emul.handler FS.

	There is very little for us to do. emul.handler should have
	installed a boot node for itself, so DOS will just try and
	run from that.       

    	We can then cause the boot to continue by calling
    	InitResident(FindResident("dos.library"));
    */
    
    struct Resident *dosResident = NULL;

    dosResident = FindResident("dos.library");
    if(dosResident)
    {
	InitResident(dosResident, NULL);
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

static const char boot_end = 0;
