/*
    Copyright (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Utility Resident and initialization.
    Lang: english
*/
#include <exec/types.h>
#include <aros/system.h>
#include <aros/libcall.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "utility_intern.h"
#include "libdefs.h"

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR inittabl[4];
extern void *const FUNCTABLE[];
struct LIBBASETYPE *INIT();
extern const char END;

extern ULONG AROS_SLIB_ENTRY(SMult32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UMult32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(SMult64_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UMult64_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(SDivMod32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UDivMod32_020,Utility)();

int Utility_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident Utility_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Utility_resident,
    (APTR)&Utility_end,
    RTF_AUTOINIT|RTF_COLDSTART,
    LIBVERSION,
    NT_LIBRARY,
    103,
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

const UBYTE name[]=LIBNAME;

const UBYTE version[]=VERSION;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntUtilityBase),
    (APTR)FUNCTABLE,
    NULL,
    &INIT
};

static struct TagItem nstags[] =
{
    { ANO_NameSpace, TRUE },
    { ANO_Flags, NSF_NODUPS },
    { TAG_DONE, 0 }
};

#ifdef SysBase
#undef SysBase
#endif

/* iaint:
    Sigh, I require this to compile this with DICE, I will
    remove it at a later date...or at least change it :)
*/

#ifdef __GNUC__
#define SysBase GetIntUtilityBase(LIBBASE)->ub_SysBase
#else
struct ExecBase *SysBase = 0L;
#endif

#define SetFunc(a,b) SetFunction((struct Library *)LIBBASETYPE, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,Utility))

AROS_LH2(struct LIBBASETYPE *, init,
    AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    GetIntUtilityBase(LIBBASE)->ub_SysBase=sysBase;
    GetIntUtilityBase(LIBBASE)->ub_SegList=segList;
#ifdef _DCC
    SysBase = sysBase;
#endif

    /* Set up UtilityBase */
    LIBBASE->ub_LibNode.lib_Node.ln_Pri = 0;
    LIBBASE->ub_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    (const)UtilityBase->ub_LibNode.lib_Node.ln_Name = name;
    LIBBASE->ub_LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    LIBBASE->ub_LibNode.lib_Version = LIBVERSION;
    LIBBASE->ub_LibNode.lib_Revision = LIBREVISION;
    (const)LIBBASE->ub_LibNode.lib_IdString = &version[6];

    GetIntUtilityBase(LIBBASE)->ub_LastID = 0;

    GetIntUtilityBase(LIBBASE)->ub_GlobalNameSpace =
        AllocNamedObjectA("utility global name space", nstags);

    if(GetIntUtilityBase(LIBBASE)->ub_GlobalNameSpace == NULL)
    {
        Alert(AG_NoMemory | AO_UtilityLib);
    }

#if defined(__mc68000__)
    /* Are we running on a m68020 or higher?
       If so we should setfunction all the relevant functions to use
       native code.
    */
    if(SysBase->AttnFlags & AFF_68020)
    {
        SetFunc(23, SMult32_020);
        SetFunc(24, UMult32_020);
        SetFunc(25, SDivMod32_020);
        SetFunc(26, UDivMod32_020);

#if 0
        /* The 060 doesn't have some of the instructions I use... */
        if((SysBase->AttnFlags & AFF_68060) == 0)
        {
            SetFunc(33, SMult64_020);
            SetFunc(34, UMult64_020);
        }
#endif
    }
#endif

    /* You would return NULL if the init failed */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct LIBBASETYPE *, open,
 AROS_LHA(ULONG, version, D0),
           struct LIBBASETYPE *, LIBBASE, 1, BASENAME)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    LIBBASE->ub_LibNode.lib_OpenCnt++;
    LIBBASE->ub_LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return LIBBASE;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
           struct LIBBASETYPE *, LIBBASE, 2, Utility)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--LIBBASE->ub_LibNode.lib_OpenCnt)
    {
#ifdef DISK_BASED
        /* Delayed expunge pending? */
        if(LIBBASE->ub_LibNode.lib_Flags&LIBF_DELEXP)
            /* Then expunge the library */
            return expunge();
#endif
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
           struct LIBBASETYPE *, LIBBASE, 3, Utility)
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    /* Test for openers. */
    if(LIBBASE->ub_LibNode.lib_OpenCnt)
    {
        /* Set the delayed expunge flag and return. */
        LIBBASE->ub_LibNode.lib_Flags|=LIBF_DELEXP;
        return 0;
    }
#ifdef DISK_BASED
    FreeNamedObject(GetIntUtilityBase(LIBBASE)->ub_GlobalNameSpace);

    /* Get rid of the library. Remove it from the list. */
    Remove(&LIBBASE->ub_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=GetIntUtilityBase(LIBBASE)->ub_SegList;

    /* Free the memory. */
    FreeMem((char *)LIBBASE-LIBBASE->ub_LibNode.lib_NegSize,
            LIBBASE->ub_LibNode.lib_NegSize+LIBBASE->ub_LibNode.lib_PosSize);
#else
    ret = 0;
#endif

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
            struct LIBBASETYPE *, LIBBASE, 4, Utility)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
