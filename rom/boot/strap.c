/* 
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot AROS native
    Lang: english
*/

#define DEBUG 1

#include <devices/trackdisk.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/alerts.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <libraries/expansionbase.h>
#include <devices/newstyle.h>

#include <proto/exec.h>

#ifdef DEBUG
#include <aros/debug.h>
#endif
#include <aros/macros.h>

#define BOOT_CHECK 0

void InitKeyboard(void);
void putc(char);
void putstring(char *);

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

AROS_LH2(int, init,
    AROS_LHA(ULONG, dummy, D0),
    AROS_LHA(ULONG, seglist, A0),
    struct ExecBase *, SysBase, 0, boot)
{
    AROS_LIBFUNC_INIT
    
    struct Resident *DOSResident;

    DOSResident = FindResident( "dos.library" );
					
    if( DOSResident == NULL )
    {
        Alert( AT_DeadEnd | AG_OpenLib | AN_BootStrap | AO_DOSLib );
    }
	
    InitResident( DOSResident, NULL );
    				
    return 0;
    
    AROS_LIBFUNC_EXIT
}

static const char boot_end = 0;
