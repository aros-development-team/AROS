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

/* FPU math functions */
extern void AROS_SLIB_ENTRY(acos881, STDC, 130)(void);
extern void AROS_SLIB_ENTRY(log2881, STDC, 152)(void);
extern void AROS_SLIB_ENTRY(log10881, STDC, 150)(void);
extern void AROS_SLIB_ENTRY(log881, STDC, 149)(void);
extern void AROS_SLIB_ENTRY(remainder881, STDC, 177)(void);
extern void AROS_SLIB_ENTRY(sinh881, STDC, 141)(void);
extern void AROS_SLIB_ENTRY(sqrt881, STDC, 161)(void);
extern void AROS_SLIB_ENTRY(atan881, STDC, 132)(void);
extern void AROS_SLIB_ENTRY(ceil881, STDC, 166)(void);
extern void AROS_SLIB_ENTRY(copysign881, STDC, 179)(void);
extern void AROS_SLIB_ENTRY(cos881, STDC, 134)(void);
extern void AROS_SLIB_ENTRY(exp2881, STDC, 144)(void);
extern void AROS_SLIB_ENTRY(fabs881, STDC, 158)(void);
extern void AROS_SLIB_ENTRY(fdim881, STDC, 183)(void);
extern void AROS_SLIB_ENTRY(floor881, STDC, 167)(void);
extern void AROS_SLIB_ENTRY(fma881, STDC, 186)(void);
extern void AROS_SLIB_ENTRY(fmax881, STDC, 184)(void);
extern void AROS_SLIB_ENTRY(fmin881, STDC, 185)(void);
extern void AROS_SLIB_ENTRY(fpclassify881, STDC, 113)(void);
extern void AROS_SLIB_ENTRY(isfinite881, STDC, 115)(void);
extern void AROS_SLIB_ENTRY(isinf881, STDC, 118)(void);
extern void AROS_SLIB_ENTRY(isnan881, STDC, 121)(void);
extern void AROS_SLIB_ENTRY(isnormal881, STDC, 124)(void);
extern void AROS_SLIB_ENTRY(lround881, STDC, 173)(void);
extern void AROS_SLIB_ENTRY(nearbyint881, STDC, 168)(void);
extern void AROS_SLIB_ENTRY(round881, STDC, 172)(void);
extern void AROS_SLIB_ENTRY(signbit881, STDC, 128)(void);
extern void AROS_SLIB_ENTRY(sin881, STDC, 135)(void);
extern void AROS_SLIB_ENTRY(tan881, STDC, 136)(void);
extern void AROS_SLIB_ENTRY(tanh881, STDC, 142)(void);
extern void AROS_SLIB_ENTRY(trunc881, STDC, 175)(void);
extern void AROS_SLIB_ENTRY(asin881, STDC, 131)(void);
extern void AROS_SLIB_ENTRY(atanh881, STDC, 139)(void);
extern void AROS_SLIB_ENTRY(cosh881, STDC, 140)(void);
extern void AROS_SLIB_ENTRY(exp881, STDC, 143)(void);
extern void AROS_SLIB_ENTRY(fmod881, STDC, 176)(void);
extern void AROS_SLIB_ENTRY(hypot881, STDC, 159)(void);

static int STDCM68KMATH_Init(struct StdCBase *StdCBase)
{
    if ((SysBase->AttnFlags & AFF_68060) && (SysBase->AttnFlags & AFF_FPU)) {
        /* Override supported MC68060+ math functions */
        __AROS_SETVECADDR(StdCBase, 159, AROS_SLIB_ENTRY(hypot881, STDC, 159));
        __AROS_SETVECADDR(StdCBase, 161, AROS_SLIB_ENTRY(sqrt881, STDC, 161));
        __AROS_SETVECADDR(StdCBase, 166, AROS_SLIB_ENTRY(ceil881, STDC, 166));
        __AROS_SETVECADDR(StdCBase, 179, AROS_SLIB_ENTRY(copysign881, STDC, 179));
        __AROS_SETVECADDR(StdCBase, 158, AROS_SLIB_ENTRY(fabs881, STDC, 158));
        __AROS_SETVECADDR(StdCBase, 183, AROS_SLIB_ENTRY(fdim881, STDC, 183));
        __AROS_SETVECADDR(StdCBase, 167, AROS_SLIB_ENTRY(floor881, STDC, 167));
        __AROS_SETVECADDR(StdCBase, 186, AROS_SLIB_ENTRY(fma881, STDC, 186));
        __AROS_SETVECADDR(StdCBase, 184, AROS_SLIB_ENTRY(fmax881, STDC, 184));
        __AROS_SETVECADDR(StdCBase, 185, AROS_SLIB_ENTRY(fmin881, STDC, 185));
        __AROS_SETVECADDR(StdCBase, 113, AROS_SLIB_ENTRY(fpclassify881, STDC, 113));
        __AROS_SETVECADDR(StdCBase, 115, AROS_SLIB_ENTRY(isfinite881, STDC, 115));
        __AROS_SETVECADDR(StdCBase, 118, AROS_SLIB_ENTRY(isinf881, STDC, 118));
        __AROS_SETVECADDR(StdCBase, 121, AROS_SLIB_ENTRY(isnan881, STDC, 121));
        __AROS_SETVECADDR(StdCBase, 124, AROS_SLIB_ENTRY(isnormal881, STDC, 124));
        __AROS_SETVECADDR(StdCBase, 173, AROS_SLIB_ENTRY(lround881, STDC, 173));
        __AROS_SETVECADDR(StdCBase, 168, AROS_SLIB_ENTRY(nearbyint881, STDC, 168));
        __AROS_SETVECADDR(StdCBase, 172, AROS_SLIB_ENTRY(round881, STDC, 172));
        __AROS_SETVECADDR(StdCBase, 128, AROS_SLIB_ENTRY(signbit881, STDC, 128));
        __AROS_SETVECADDR(StdCBase, 175, AROS_SLIB_ENTRY(trunc881, STDC, 175));
    }
   else if ((SysBase->AttnFlags & AFF_68881) || (SysBase->AttnFlags & AFF_68882)) {
        /* Override supported 68881/68882 math functions */
        __AROS_SETVECADDR(StdCBase, 130, AROS_SLIB_ENTRY(acos881, STDC, 130));
        __AROS_SETVECADDR(StdCBase, 152, AROS_SLIB_ENTRY(log2881, STDC, 152));
        __AROS_SETVECADDR(StdCBase, 150, AROS_SLIB_ENTRY(log10881, STDC, 150));
        __AROS_SETVECADDR(StdCBase, 149, AROS_SLIB_ENTRY(log881, STDC, 149));
        __AROS_SETVECADDR(StdCBase, 177, AROS_SLIB_ENTRY(remainder881, STDC, 177));
        __AROS_SETVECADDR(StdCBase, 141, AROS_SLIB_ENTRY(sinh881, STDC, 141));
        __AROS_SETVECADDR(StdCBase, 161, AROS_SLIB_ENTRY(sqrt881, STDC, 161));
        __AROS_SETVECADDR(StdCBase, 132, AROS_SLIB_ENTRY(atan881, STDC, 132));
        __AROS_SETVECADDR(StdCBase, 166, AROS_SLIB_ENTRY(ceil881, STDC, 166));
        __AROS_SETVECADDR(StdCBase, 179, AROS_SLIB_ENTRY(copysign881, STDC, 179));
        __AROS_SETVECADDR(StdCBase, 134, AROS_SLIB_ENTRY(cos881, STDC, 134));
        __AROS_SETVECADDR(StdCBase, 144, AROS_SLIB_ENTRY(exp2881, STDC, 144));
        __AROS_SETVECADDR(StdCBase, 158, AROS_SLIB_ENTRY(fabs881, STDC, 158));
        __AROS_SETVECADDR(StdCBase, 183, AROS_SLIB_ENTRY(fdim881, STDC, 183));
        __AROS_SETVECADDR(StdCBase, 167, AROS_SLIB_ENTRY(floor881, STDC, 167));
        __AROS_SETVECADDR(StdCBase, 186, AROS_SLIB_ENTRY(fma881, STDC, 186));
        __AROS_SETVECADDR(StdCBase, 184, AROS_SLIB_ENTRY(fmax881, STDC, 184));
        __AROS_SETVECADDR(StdCBase, 185, AROS_SLIB_ENTRY(fmin881, STDC, 185));
        __AROS_SETVECADDR(StdCBase, 113, AROS_SLIB_ENTRY(fpclassify881, STDC, 113));
        __AROS_SETVECADDR(StdCBase, 115, AROS_SLIB_ENTRY(isfinite881, STDC, 115));
        __AROS_SETVECADDR(StdCBase, 118, AROS_SLIB_ENTRY(isinf881, STDC, 118));
        __AROS_SETVECADDR(StdCBase, 121, AROS_SLIB_ENTRY(isnan881, STDC, 121));
        __AROS_SETVECADDR(StdCBase, 124, AROS_SLIB_ENTRY(isnormal881, STDC, 124));
        __AROS_SETVECADDR(StdCBase, 173, AROS_SLIB_ENTRY(lround881, STDC, 173));
        __AROS_SETVECADDR(StdCBase, 168, AROS_SLIB_ENTRY(nearbyint881, STDC, 168));
        __AROS_SETVECADDR(StdCBase, 172, AROS_SLIB_ENTRY(round881, STDC, 172));
        __AROS_SETVECADDR(StdCBase, 128, AROS_SLIB_ENTRY(signbit881, STDC, 128));
        __AROS_SETVECADDR(StdCBase, 135, AROS_SLIB_ENTRY(sin881, STDC, 135));
        __AROS_SETVECADDR(StdCBase, 136, AROS_SLIB_ENTRY(tan881, STDC, 136));
        __AROS_SETVECADDR(StdCBase, 142, AROS_SLIB_ENTRY(tanh881, STDC, 142));
        __AROS_SETVECADDR(StdCBase, 175, AROS_SLIB_ENTRY(trunc881, STDC, 175));
        __AROS_SETVECADDR(StdCBase, 131, AROS_SLIB_ENTRY(asin881, STDC, 131));
        __AROS_SETVECADDR(StdCBase, 139, AROS_SLIB_ENTRY(atanh881, STDC, 139));
        __AROS_SETVECADDR(StdCBase, 140, AROS_SLIB_ENTRY(cosh881, STDC, 140));
        __AROS_SETVECADDR(StdCBase, 143, AROS_SLIB_ENTRY(exp881, STDC, 143));
        __AROS_SETVECADDR(StdCBase, 176, AROS_SLIB_ENTRY(fmod881, STDC, 176));
        __AROS_SETVECADDR(StdCBase, 159, AROS_SLIB_ENTRY(hypot881, STDC, 159));
   }

   return TRUE;
}

ADD2INITLIB(STDCM68KMATH_Init, 0)
#endif
