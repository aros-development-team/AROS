/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Battery-backed up clock initialisation.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/battclock.h>

#include <aros/asmcall.h>
#include "battclock_intern.h"
#include LC_LIBDEFS_FILE

#ifdef SysBase
#undef SysBase
#endif

static const UBYTE name[];
static const UBYTE version[];
static const void * const LIBFUNCTABLE[];
extern const char LIBEND;

struct BattClockBase *AROS_SLIB_ENTRY(init, BASENAME)();

extern void AROS_SLIB_ENTRY(ReadBattClock,BASENAME)();
extern void AROS_SLIB_ENTRY(ResetBattClock,BASENAME)();
extern void AROS_SLIB_ENTRY(WriteBattClock,BASENAME)();

int Battclock_entry(void)
{
    return -1;
}

const struct Resident Battclock_resident __attribute__((section(".text"))) =
{
    RTC_MATCHWORD,
    (struct Resident *)&Battclock_resident,
    (APTR)&LIBEND,
    RTF_COLDSTART,
    41,
    NT_RESOURCE,
    45,
    (UBYTE *)name,
    (UBYTE *)&version[6],
    (ULONG *)&AROS_SLIB_ENTRY(init,BASENAME)
};

static const UBYTE name[] = NAME_STRING;
static const UBYTE version[] = VERSION_STRING;

static const void * const LIBFUNCTABLE[] =
{
    &AROS_SLIB_ENTRY(ResetBattClock,BASENAME),
    &AROS_SLIB_ENTRY(ReadBattClock,BASENAME),
    &AROS_SLIB_ENTRY(WriteBattClock,BASENAME),
    (void *)-1
};

AROS_UFH3(struct BattClockBase *, AROS_SLIB_ENTRY(init,BASENAME),
    AROS_UFHA(ULONG,	dummy,	D0),
    AROS_UFHA(ULONG,	slist,	A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT
    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * 3);
    struct BattClockBase * BattClockBase = NULL;
    
    BattClockBase = (struct BattClockBase *)(((UBYTE *)
	AllocMem( neg + sizeof(struct BattClockBase),
		    MEMF_CLEAR | MEMF_PUBLIC)) + neg);

    if( BattClockBase )
    {
	BattClockBase->bb_SysBase = SysBase;
	BattClockBase->bb_UtilBase = OpenLibrary("utility.library",0);
	BattClockBase->bb_Node.ln_Pri = 0;
	BattClockBase->bb_Node.ln_Type = NT_RESOURCE;
	BattClockBase->bb_Node.ln_Name = (STRPTR)name;

	MakeFunctions(BattClockBase, (APTR)LIBFUNCTABLE, NULL);
	AddResource(BattClockBase);
    }
    return BattClockBase;
    AROS_USERFUNC_EXIT
}
