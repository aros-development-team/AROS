/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility Resident and initialization.
    Lang: english
*/
#include "intern.h"
#include LC_LIBDEFS_FILE

#ifdef SysBase
#   undef SysBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntUtilityBase *)(lib))->ub_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntUtilityBase *)(lib))->ub_SegList)
#define LC_RESIDENTNAME 	Utility_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		103
#define LC_LIBBASESIZE		sizeof (struct IntUtilityBase)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntUtilityBase *)(lib))->UBase.ub_LibNode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

extern ULONG AROS_SLIB_ENTRY(SMult32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UMult32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(SMult64_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UMult64_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(SDivMod32_020,Utility)();
extern ULONG AROS_SLIB_ENTRY(UDivMod32_020,Utility)();

/* iaint:
    Sigh, I require this to compile this with DICE, I will
    remove it at a later date...or at least change it :)
*/

#ifndef _DCC
#undef SysBase
#define SysBase GetIntUtilityBase(LIBBASE)->ub_SysBase
#else
struct ExecBase *SysBase = 0L;
#endif

#define SetFunc(a,b) SetFunction((struct Library *)LIBBASETYPE, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,Utility))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{
#ifdef _DCC
    SysBase = GetIntUtilityBase(LIBBASE)->ub_SysBase;
#endif

    GetIntUtilityBase(LIBBASE)->ub_LastID = 0;

    /*
	I no longer allocate memory here for the global namespace, since
	that is not quite legal. (AllocMem is not Forbid() protected).

	Also makes this a little bit shorter. (In time and length).
    */
    InitSemaphore(&GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_Lock);
    NEWLIST((struct List *)&GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_List);
    GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_Flags = NSF_NODUPS;

#if defined(__mc68000__)
    /* Are we running on a m68020 or higher?
       If so we should setfunction all the relevant functions to use
       native code.
    */
    if(SysBase->AttnFlags & AFF_68020)
    {
/*
	SetFunc(23, SMult32_020);
	SetFunc(24, UMult32_020);
	SetFunc(25, SDivMod32_020);
	SetFunc(26, UDivMod32_020);
*/

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
    return TRUE;
}

