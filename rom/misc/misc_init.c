/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/*  HISTORY:  23.7.98  SDuvan  Implemented based on BattClock code. */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/misc.h>
#include <resources/misc.h>

#include <aros/asmcall.h>
#include "misc_intern.h"

#include "libdefs.h"

static const UBYTE name[];
static const UBYTE idstring[];
static const void * const functable[];
extern const char LIBEND;

struct MiscBase *AROS_SLIB_ENTRY(init, Misc)();

extern void AROS_SLIB_ENTRY(AllocMiscResource ,Misc)();
extern void AROS_SLIB_ENTRY(FreeMiscResource, Misc)();

int misc_entry(void)
{
    return -1;
}

const struct Resident Misc_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Misc_resident,
    (APTR)&LIBEND,
    RTF_COLDSTART,
    41,
    NT_RESOURCE,
    45,
    (UBYTE *)name,
    (UBYTE *)&idstring[6],
    (ULONG *)&AROS_SLIB_ENTRY(init,Misc)
};

static const UBYTE name[] = MISCNAME;
static const UBYTE idstring[] = "misc 41.0 (23.7.1998)";

static const void * const functable[] =
{
    &AROS_SLIB_ENTRY(AllocMiscResource, Misc),
    &AROS_SLIB_ENTRY(FreeMiscResource,  Misc),
    (void *)-1
};


#ifdef SysBase
#undef SysBase
#endif
//#define SysBase (*(APTR*)4L)

AROS_UFH3(struct MiscBase *, AROS_SLIB_ENTRY(init, Misc),
    AROS_UFHA(ULONG,	dummy,	D0),
    AROS_UFHA(ULONG,	slist,	A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    UWORD neg = AROS_ALIGN(LIB_VECTSIZE * 3);
    struct MiscBase *MiscBase = NULL;
    
    MiscBase = (struct MiscBase *)(((UBYTE *)
	AllocMem( neg + sizeof(struct MiscBase),
		 MEMF_CLEAR | MEMF_PUBLIC)) + neg);

    if( MiscBase )
    {
	MiscBase->mb_SysBase = SysBase;
	MiscBase->mb_Node.ln_Pri = 0;
	MiscBase->mb_Node.ln_Type = NT_RESOURCE;
	MiscBase->mb_Node.ln_Name = (STRPTR)name;

	InitSemaphore(&MiscBase->mb_Lock);

	MakeFunctions(MiscBase, (APTR)functable, NULL);
	AddResource(MiscBase);
    }

    return MiscBase;
}
