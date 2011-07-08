/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility Resident and initialization.
    Lang: english
*/

#include <aros/symbolsets.h>

#include "intern.h"
#include LC_LIBDEFS_FILE

extern ULONG AROS_SLIB_ENTRY(SMult32_020,Utility,23)();
extern ULONG AROS_SLIB_ENTRY(UMult32_020,Utility,24)();
extern ULONG AROS_SLIB_ENTRY(SDivMod32_020,Utility,25)();
extern ULONG AROS_SLIB_ENTRY(UDivMod32_020,Utility,26)();
extern ULONG AROS_SLIB_ENTRY(SMult64_020,Utility,33)();
extern ULONG AROS_SLIB_ENTRY(UMult64_020,Utility,34)();

#define SetFunc(a,b) SetFunction((struct Library *)LIBBASE, a * -LIB_VECTSIZE, AROS_SLIB_ENTRY(b,Utility,a))

static int UtilityInit(LIBBASETYPEPTR LIBBASE)
{
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
	SetFunc(23, SMult32_020);
	SetFunc(24, UMult32_020);
	SetFunc(25, SDivMod32_020);
	SetFunc(26, UDivMod32_020);

	/* The 060 doesn't have some of the instructions I use... */
	if((SysBase->AttnFlags & AFF_68060) == 0)
	{
	    SetFunc(33, SMult64_020);
	    SetFunc(34, UMult64_020);
	}
    }
#endif

    return TRUE;
}

ADD2INITLIB(UtilityInit, 0);
