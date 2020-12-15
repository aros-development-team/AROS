/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/utility.h>

extern ULONG AROS_SLIB_ENTRY(SMult32_020,Utility,23)();
extern ULONG AROS_SLIB_ENTRY(UMult32_020,Utility,24)();
extern ULONG AROS_SLIB_ENTRY(SDivMod32_020,Utility,25)();
extern ULONG AROS_SLIB_ENTRY(UDivMod32_020,Utility,26)();
extern ULONG AROS_SLIB_ENTRY(SMult64_020,Utility,33)();
extern ULONG AROS_SLIB_ENTRY(UMult64_020,Utility,34)();

static int UtilityM68K_ArchInit(struct Library *UtilityBase)
{
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

    return TRUE;
}

ADD2INITLIB(UtilityM68K_ArchInit, 0);
#endif
