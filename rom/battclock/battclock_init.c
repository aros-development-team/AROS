/*
    Copyright (C) 1995-1998 AROS
    $Id$

    Desc: Battery-backed up clock initialisation.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/battclock.h>

#include <aros/asmcall.h>
#include "battclock_intern.h"

#ifdef SysBase
#undef SysBase
#endif

static const UBYTE name[];
static const UBYTE idstring[];
static const void * const functable[];
extern const char AROS_SLIB_ENTRY(end,Battclock);

struct BattClockBase *AROS_SLIB_ENTRY(init, Battclock)();

extern void AROS_SLIB_ENTRY(ReadBattClock,Battclock)();
extern void AROS_SLIB_ENTRY(ResetBattClock,Battclock)();
extern void AROS_SLIB_ENTRY(WriteBattClock,Battclock)();

static int battclock_entry(void)
{
    return -1;
}

const struct Resident Battclock_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Battclock_resident,
    (APTR)&AROS_SLIB_ENTRY(end,Battclock),
    RTF_COLDSTART,
    41,
    NT_RESOURCE,
    45,
    (UBYTE *)name,
    (UBYTE *)&idstring[6],
    (ULONG *)&AROS_SLIB_ENTRY(init,Battclock)
};

static const UBYTE name[] = "battclock.resource";
static const UBYTE idstring[] = "battclock 41.1 (23.1.1998)";

static const void * const functable[] =
{
    &AROS_SLIB_ENTRY(ResetBattClock,Battclock),
    &AROS_SLIB_ENTRY(ReadBattClock,Battclock),
    &AROS_SLIB_ENTRY(WriteBattClock,Battclock),
    (void *)-1
};

AROS_UFH3(struct BattClockBase *, AROS_SLIB_ENTRY(init,Battclock),
    AROS_UFHA(ULONG,	dummy,	D0),
    AROS_UFHA(ULONG,	slist,	A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * 3);
    struct BattClockBase * BattClockBase = NULL;
    
    BattClockBase = (struct BattClockBase *)(((UBYTE *)
	AllocMem( neg + sizeof(struct BattClockBase),
		    MEMF_CLEAR | MEMF_PUBLIC)) + neg);

    if( BattClockBase )
    {
	BattClockBase->bb_SysBase = SysBase;
	BattClockBase->bb_Node.ln_Pri = 0;
	BattClockBase->bb_Node.ln_Type = NT_RESOURCE;
	BattClockBase->bb_Node.ln_Name = (STRPTR)name;

	MakeFunctions(BattClockBase, (APTR)functable, NULL);
	AddResource(BattClockBase);
    }
    return BattClockBase;
}
