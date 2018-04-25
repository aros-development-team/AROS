/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <libraries/stdc.h>

#include <proto/exec.h>

#ifndef STDC_STATIC

#if (0)
/* 68881/68882 functions */
#endif
/* MC68060+ functions */
extern void AROS_SLIB_ENTRY(hypot060, STDC, 149)(void);
extern void AROS_SLIB_ENTRY(sqrt060, STDC, 151)(void);
extern void AROS_SLIB_ENTRY(ceil060, STDC, 156)(void);
extern void AROS_SLIB_ENTRY(copysign060, STDC, 169)(void);
extern void AROS_SLIB_ENTRY(fabs060, STDC, 148)(void);
extern void AROS_SLIB_ENTRY(fdim060, STDC, 173)(void);
extern void AROS_SLIB_ENTRY(floor060, STDC, 157)(void);
extern void AROS_SLIB_ENTRY(fma060, STDC, 176)(void);
extern void AROS_SLIB_ENTRY(fmax060, STDC, 174)(void);
extern void AROS_SLIB_ENTRY(fmin060, STDC, 175)(void);
extern void AROS_SLIB_ENTRY(fpclassify060, STDC, 103)(void);
extern void AROS_SLIB_ENTRY(isfinite060, STDC, 105)(void);
extern void AROS_SLIB_ENTRY(isinf060, STDC, 108)(void);
extern void AROS_SLIB_ENTRY(isnan060, STDC, 111)(void);
extern void AROS_SLIB_ENTRY(isnormal060, STDC, 114)(void);
extern void AROS_SLIB_ENTRY(lround060, STDC, 163)(void);
extern void AROS_SLIB_ENTRY(nearbyint060, STDC, 158)(void);
extern void AROS_SLIB_ENTRY(round060, STDC, 162)(void);
extern void AROS_SLIB_ENTRY(signbit060, STDC, 118)(void);
extern void AROS_SLIB_ENTRY(trunc060, STDC, 165)(void);

static int STDCM68KMATH_Init(struct StdCBase *StdCBase)
{
#if (0)
	/* TODO: Implement */
   if (SysBase->AttnFlags & AFF_68881 || SysBase->AttnFlags & AFF_68882) {
        /* Override supported 68881/68882 math functions */
   }
#endif

    if (SysBase->AttnFlags & AFF_68060) {
        /* Override supported MC68060+ math functions */
    	__AROS_SETVECADDR(StdCBase, -149*LIB_VECTSIZE, AROS_SLIB_ENTRY(hypot060, STDC, 149));
    	__AROS_SETVECADDR(StdCBase, -151*LIB_VECTSIZE, AROS_SLIB_ENTRY(sqrt060, STDC, 151));
    	__AROS_SETVECADDR(StdCBase, -156*LIB_VECTSIZE, AROS_SLIB_ENTRY(ceil060, STDC, 156));
    	__AROS_SETVECADDR(StdCBase, -169*LIB_VECTSIZE, AROS_SLIB_ENTRY(copysign060, STDC, 169));
    	__AROS_SETVECADDR(StdCBase, -148*LIB_VECTSIZE, AROS_SLIB_ENTRY(fabs060, STDC, 148));
    	__AROS_SETVECADDR(StdCBase, -173*LIB_VECTSIZE, AROS_SLIB_ENTRY(fdim060, STDC, 173));
    	__AROS_SETVECADDR(StdCBase, -157*LIB_VECTSIZE, AROS_SLIB_ENTRY(floor060, STDC, 157));
    	__AROS_SETVECADDR(StdCBase, -176*LIB_VECTSIZE, AROS_SLIB_ENTRY(fma060, STDC, 176));
    	__AROS_SETVECADDR(StdCBase, -174*LIB_VECTSIZE, AROS_SLIB_ENTRY(fmax060, STDC, 174));
    	__AROS_SETVECADDR(StdCBase, -175*LIB_VECTSIZE, AROS_SLIB_ENTRY(fmin060, STDC, 175));
    	__AROS_SETVECADDR(StdCBase, -103*LIB_VECTSIZE, AROS_SLIB_ENTRY(fpclassify060, STDC, 103));
    	__AROS_SETVECADDR(StdCBase, -105*LIB_VECTSIZE, AROS_SLIB_ENTRY(isfinite060, STDC, 105));
    	__AROS_SETVECADDR(StdCBase, -108*LIB_VECTSIZE, AROS_SLIB_ENTRY(isinf060, STDC, 108));
    	__AROS_SETVECADDR(StdCBase, -111*LIB_VECTSIZE, AROS_SLIB_ENTRY(isnan060, STDC, 111));
    	__AROS_SETVECADDR(StdCBase, -114*LIB_VECTSIZE, AROS_SLIB_ENTRY(isnormal060, STDC, 114));
    	__AROS_SETVECADDR(StdCBase, -163*LIB_VECTSIZE, AROS_SLIB_ENTRY(lround060, STDC, 163));
    	__AROS_SETVECADDR(StdCBase, -158*LIB_VECTSIZE, AROS_SLIB_ENTRY(nearbyint060, STDC, 158));
    	__AROS_SETVECADDR(StdCBase, -162*LIB_VECTSIZE, AROS_SLIB_ENTRY(round060, STDC, 162));
    	__AROS_SETVECADDR(StdCBase, -118*LIB_VECTSIZE, AROS_SLIB_ENTRY(signbit060, STDC, 118));
    	__AROS_SETVECADDR(StdCBase, -165*LIB_VECTSIZE, AROS_SLIB_ENTRY(trunc060, STDC, 165));
    }
    return TRUE;
}

ADD2INITLIB(STDCM68KMATH_Init, 0)
#endif
