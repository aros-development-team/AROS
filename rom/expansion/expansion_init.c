/*
    Copyright (C) 1996 AROS - The Amiga Replacement OS
    $Id$
    
    Desc: Expansion Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "expansion_intern.h"
#include "libdefs.h"

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR initabl[4];
extern void *const FUNCTABLE[];
struct LIBBASETYPE *INIT();
extern const char END;

int Expansion_entry()
{
    return -1;
}

const struct Resident Expansion_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Expansion_resident,
    (APTR)&END,
    RTF_AUTOINIT|RTF_SINGLETASK,
    LIBVERSION,
    NT_LIBRARY,
    110,
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)initabl
};

const UBYTE name[]=LIBNAME;
const UBYTE version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntExpansionBase),
    (APTR)FUNCTABLE,
    NULL,
    &INIT
};

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *,LIBBASE, D0),
    AROS_LHA(BPTR, segList, A0),
    struct ExecBase *, sysBase, 0, Expansion)
{
    AROS_LIBFUNC_INIT
    
    /* Store arguments */
    IntExpBase(LIBBASE)->eb_SegList = (ULONG)segList;
    IntExpBase(LIBBASE)->eb_SysBase = sysBase;
    
    LIBBASE->LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Node.ln_Name = (STRPTR)name;
    LIBBASE->LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->LibNode.lib_Version = LIBVERSION;
    LIBBASE->LibNode.lib_Revision = LIBREVISION;
    LIBBASE->LibNode.lib_IdString = (STRPTR)&version[6];
    
#if 0
    /* See what expansion hardware we can detect. */
    ConfigChain();
#endif
    
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}
