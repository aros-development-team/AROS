/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS
    Lang: english
*/

#define DEBUG 1

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <aros/asmcall.h>

#include <proto/exec.h>

#ifdef DEBUG
#include <aros/debug.h>
#endif
#include <aros/macros.h>

#define BOOT_CHECK 0

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
	"Boot Strap",
	"AROS Boot Strap 41.0\r\n",
	(APTR)&boot_init
};

AROS_UFH3(int, AROS_SLIB_ENTRY(init,boot),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(ULONG, seglist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Resident *DOSResident;

    DOSResident = FindResident( "dos.library" );

    if( DOSResident == NULL )
    {
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
    }

    InitResident( DOSResident, NULL );

    return 0;

    AROS_USERFUNC_EXIT
}

static const char boot_end = 0;
