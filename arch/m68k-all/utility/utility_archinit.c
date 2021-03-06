/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <defines/utility_LVO.h>

extern ULONG AROS_SLIB_ENTRY(SMult32_020,Utility,LVOSMult32)();
extern ULONG AROS_SLIB_ENTRY(UMult32_020,Utility,LVOUMult32)();
extern ULONG AROS_SLIB_ENTRY(SDivMod32_020,Utility,LVOSDivMod32)();
extern ULONG AROS_SLIB_ENTRY(UDivMod32_020,Utility,LVOUDivMod32)();
extern ULONG AROS_SLIB_ENTRY(SMult64_020,Utility,LVOSMult64)();
extern ULONG AROS_SLIB_ENTRY(UMult64_020,Utility,LVOUMult64)();

#define SetFunc(a,b) SetFunction(UtilityBase, -(a) * LIB_VECTSIZE, AROS_SLIB_ENTRY(b,Utility,a))

static int UtilityM68K_ArchInit(struct Library *UtilityBase)
{
    /* Are we running on a m68020 or higher?
       If so we should setfunction all the relevant functions to use
       native code.
    */
    if(SysBase->AttnFlags & AFF_68020)
    {
        SetFunc(LVOSMult32, SMult32_020);
        SetFunc(LVOUMult32, UMult32_020);
        SetFunc(LVOSDivMod32, SDivMod32_020);
        SetFunc(LVOUDivMod32, UDivMod32_020);

        /* The 060 doesn't have some of the instructions I use... */
        if((SysBase->AttnFlags & AFF_68060) == 0)
        {
            SetFunc(LVOSMult64, SMult64_020);
            SetFunc(LVOUMult64, UMult64_020);
        }
    }

    return TRUE;
}

ADD2INITLIB(UtilityM68K_ArchInit, 0);
