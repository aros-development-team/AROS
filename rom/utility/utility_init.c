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

extern const UBYTE name[];
extern const UBYTE version[];
extern const APTR inittabl[4];
extern void *const Utility_functable[];
struct UtilityBase *AROS_SLIB_ENTRY(init,Utility)();
extern const char Utility_end;

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
    41,
    NT_LIBRARY,
    103,
    (STRPTR)name,
    (STRPTR)&version[6],
    (ULONG *)inittabl
};

const UBYTE name[]="utility.library";

const UBYTE version[]="$VER: AROS utility 41.10 (10.2.97)";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntUtilityBase),
    (APTR)Utility_functable,
    NULL,
    &AROS_SLIB_ENTRY(init,Utility)
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
#define SysBase GetIntUtilityBase(UtilityBase)->ub_SysBase
#else
struct ExecBase *SysBase = 0L;
#endif

#define SetFunc(a,b) SetFunction((struct Library *)UtilityBase, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,Utility))

AROS_LH2(struct UtilityBase *, init,
    AROS_LHA(struct UtilityBase *, UtilityBase, D0),
    AROS_LHA(BPTR,               segList,   A0),
    struct ExecBase *, sysBase, 0, Utility)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    GetIntUtilityBase(UtilityBase)->ub_SysBase=sysBase;
    GetIntUtilityBase(UtilityBase)->ub_SegList=segList;
#ifdef _DCC
    SysBase = sysBase;
#endif

    /* Set up UtilityBase */
    UtilityBase->ub_LibNode.lib_Node.ln_Pri = 0;
    UtilityBase->ub_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    (const)UtilityBase->ub_LibNode.lib_Node.ln_Name = name;
    UtilityBase->ub_LibNode.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    UtilityBase->ub_LibNode.lib_Version = 41;
    UtilityBase->ub_LibNode.lib_Revision = 8;
    (const)UtilityBase->ub_LibNode.lib_IdString = &version[6];

    GetIntUtilityBase(UtilityBase)->ub_LastID = 0;

    GetIntUtilityBase(UtilityBase)->ub_GlobalNameSpace =
        AllocNamedObjectA("utility global name space", nstags);

    if(GetIntUtilityBase(UtilityBase)->ub_GlobalNameSpace == NULL)
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
    return UtilityBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct UtilityBase *, open,
 AROS_LHA(ULONG, version, D0),
           struct UtilityBase *, UtilityBase, 1, Utility)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    UtilityBase->ub_LibNode.lib_OpenCnt++;
    UtilityBase->ub_LibNode.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return UtilityBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close,
           struct UtilityBase *, UtilityBase, 2, Utility)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    if(!--UtilityBase->ub_LibNode.lib_OpenCnt)
    {
#ifdef DISK_BASED
        /* Delayed expunge pending? */
        if(UtilityBase->ub_LibNode.lib_Flags&LIBF_DELEXP)
            /* Then expunge the library */
            return expunge();
#endif
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge,
           struct UtilityBase *, UtilityBase, 3, Utility)
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    /* Test for openers. */
    if(UtilityBase->ub_LibNode.lib_OpenCnt)
    {
        /* Set the delayed expunge flag and return. */
        UtilityBase->ub_LibNode.lib_Flags|=LIBF_DELEXP;
        return 0;
    }
#ifdef DISK_BASED
    FreeNamedObject(GetIntUtilityBase(UtilityBase)->ub_GlobalNameSpace);

    /* Get rid of the library. Remove it from the list. */
    Remove(&UtilityBase->ub_LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=GetIntUtilityBase(UtilityBase)->ub_SegList;

    /* Free the memory. */
    FreeMem((char *)UtilityBase-UtilityBase->ub_LibNode.lib_NegSize,
            UtilityBase->ub_LibNode.lib_NegSize+UtilityBase->ub_LibNode.lib_PosSize);
#else
    ret = 0;
#endif

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null,
            struct UtilityBase *, UtilityBase, 4, Utility)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
