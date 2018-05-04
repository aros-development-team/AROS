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
extern void AROS_SLIB_ENTRY(acos881, STDC, 120)(void);
extern void AROS_SLIB_ENTRY(log2881, STDC, 142)(void);
extern void AROS_SLIB_ENTRY(log10881, STDC, 140)(void);
extern void AROS_SLIB_ENTRY(log881, STDC, 139)(void);
extern void AROS_SLIB_ENTRY(remainder881, STDC, 167)(void);
extern void AROS_SLIB_ENTRY(sinh881, STDC, 131)(void);
extern void AROS_SLIB_ENTRY(sqrt881, STDC, 151)(void);
extern void AROS_SLIB_ENTRY(atan881, STDC, 122)(void);
extern void AROS_SLIB_ENTRY(ceil881, STDC, 156)(void);
extern void AROS_SLIB_ENTRY(copysign881, STDC, 169)(void);
extern void AROS_SLIB_ENTRY(cos881, STDC, 124)(void);
extern void AROS_SLIB_ENTRY(exp2881, STDC, 134)(void);
extern void AROS_SLIB_ENTRY(fabs881, STDC, 148)(void);
extern void AROS_SLIB_ENTRY(fdim881, STDC, 173)(void);
extern void AROS_SLIB_ENTRY(floor881, STDC, 157)(void);
extern void AROS_SLIB_ENTRY(fma881, STDC, 176)(void);
extern void AROS_SLIB_ENTRY(fmax881, STDC, 174)(void);
extern void AROS_SLIB_ENTRY(fmin881, STDC, 175)(void);
extern void AROS_SLIB_ENTRY(fpclassify881, STDC, 103)(void);
extern void AROS_SLIB_ENTRY(isfinite881, STDC, 105)(void);
extern void AROS_SLIB_ENTRY(isinf881, STDC, 108)(void);
extern void AROS_SLIB_ENTRY(isnan881, STDC, 111)(void);
extern void AROS_SLIB_ENTRY(isnormal881, STDC, 114)(void);
extern void AROS_SLIB_ENTRY(lround881, STDC, 163)(void);
extern void AROS_SLIB_ENTRY(nearbyint881, STDC, 158)(void);
extern void AROS_SLIB_ENTRY(round881, STDC, 162)(void);
extern void AROS_SLIB_ENTRY(signbit881, STDC, 118)(void);
extern void AROS_SLIB_ENTRY(sin881, STDC, 125)(void);
extern void AROS_SLIB_ENTRY(tan881, STDC, 126)(void);
extern void AROS_SLIB_ENTRY(tanh881, STDC, 132)(void);
extern void AROS_SLIB_ENTRY(trunc881, STDC, 165)(void);
extern void AROS_SLIB_ENTRY(asin881, STDC, 121)(void);
extern void AROS_SLIB_ENTRY(atanh881, STDC, 129)(void);
extern void AROS_SLIB_ENTRY(cosh881, STDC, 130)(void);
extern void AROS_SLIB_ENTRY(exp881, STDC, 133)(void);
extern void AROS_SLIB_ENTRY(fmod881, STDC, 166)(void);
extern void AROS_SLIB_ENTRY(hypot881, STDC, 149)(void);

static int STDCM68KMATH_Init(struct StdCBase *StdCBase)
{
    if (SysBase->AttnFlags & AFF_68060) {
        /* Override supported MC68060+ math functions */
        __AROS_SETVECADDR(StdCBase, 149, AROS_SLIB_ENTRY(hypot881, STDC, 149));
        __AROS_SETVECADDR(StdCBase, 151, AROS_SLIB_ENTRY(sqrt881, STDC, 151));
        __AROS_SETVECADDR(StdCBase, 156, AROS_SLIB_ENTRY(ceil881, STDC, 156));
        __AROS_SETVECADDR(StdCBase, 169, AROS_SLIB_ENTRY(copysign881, STDC, 169));
        __AROS_SETVECADDR(StdCBase, 148, AROS_SLIB_ENTRY(fabs881, STDC, 148));
        __AROS_SETVECADDR(StdCBase, 173, AROS_SLIB_ENTRY(fdim881, STDC, 173));
        __AROS_SETVECADDR(StdCBase, 157, AROS_SLIB_ENTRY(floor881, STDC, 157));
        __AROS_SETVECADDR(StdCBase, 176, AROS_SLIB_ENTRY(fma881, STDC, 176));
        __AROS_SETVECADDR(StdCBase, 174, AROS_SLIB_ENTRY(fmax881, STDC, 174));
        __AROS_SETVECADDR(StdCBase, 175, AROS_SLIB_ENTRY(fmin881, STDC, 175));
        __AROS_SETVECADDR(StdCBase, 103, AROS_SLIB_ENTRY(fpclassify881, STDC, 103));
        __AROS_SETVECADDR(StdCBase, 105, AROS_SLIB_ENTRY(isfinite881, STDC, 105));
        __AROS_SETVECADDR(StdCBase, 108, AROS_SLIB_ENTRY(isinf881, STDC, 108));
        __AROS_SETVECADDR(StdCBase, 111, AROS_SLIB_ENTRY(isnan881, STDC, 111));
        __AROS_SETVECADDR(StdCBase, 114, AROS_SLIB_ENTRY(isnormal881, STDC, 114));
        __AROS_SETVECADDR(StdCBase, 163, AROS_SLIB_ENTRY(lround881, STDC, 163));
        __AROS_SETVECADDR(StdCBase, 158, AROS_SLIB_ENTRY(nearbyint881, STDC, 158));
        __AROS_SETVECADDR(StdCBase, 162, AROS_SLIB_ENTRY(round881, STDC, 162));
        __AROS_SETVECADDR(StdCBase, 118, AROS_SLIB_ENTRY(signbit881, STDC, 118));
        __AROS_SETVECADDR(StdCBase, 165, AROS_SLIB_ENTRY(trunc881, STDC, 165));
    }
   else if (SysBase->AttnFlags & AFF_68881 || SysBase->AttnFlags & AFF_68882) {
        /* Override supported 68881/68882 math functions */
        __AROS_SETVECADDR(StdCBase, 120, AROS_SLIB_ENTRY(acos881, STDC, 120));
        __AROS_SETVECADDR(StdCBase, 142, AROS_SLIB_ENTRY(log2881, STDC, 142));
        __AROS_SETVECADDR(StdCBase, 140, AROS_SLIB_ENTRY(log10881, STDC, 140));
        __AROS_SETVECADDR(StdCBase, 139, AROS_SLIB_ENTRY(log881, STDC, 139));
        __AROS_SETVECADDR(StdCBase, 167, AROS_SLIB_ENTRY(remainder881, STDC, 167));
        __AROS_SETVECADDR(StdCBase, 131, AROS_SLIB_ENTRY(sinh881, STDC, 131));
        __AROS_SETVECADDR(StdCBase, 151, AROS_SLIB_ENTRY(sqrt881, STDC, 151));
        __AROS_SETVECADDR(StdCBase, 122, AROS_SLIB_ENTRY(atan881, STDC, 122));
        __AROS_SETVECADDR(StdCBase, 156, AROS_SLIB_ENTRY(ceil881, STDC, 156));
        __AROS_SETVECADDR(StdCBase, 169, AROS_SLIB_ENTRY(copysign881, STDC, 169));
        __AROS_SETVECADDR(StdCBase, 124, AROS_SLIB_ENTRY(cos881, STDC, 124));
        __AROS_SETVECADDR(StdCBase, 134, AROS_SLIB_ENTRY(exp2881, STDC, 134));
        __AROS_SETVECADDR(StdCBase, 148, AROS_SLIB_ENTRY(fabs881, STDC, 148));
        __AROS_SETVECADDR(StdCBase, 173, AROS_SLIB_ENTRY(fdim881, STDC, 173));
        __AROS_SETVECADDR(StdCBase, 157, AROS_SLIB_ENTRY(floor881, STDC, 157));
        __AROS_SETVECADDR(StdCBase, 176, AROS_SLIB_ENTRY(fma881, STDC, 176));
        __AROS_SETVECADDR(StdCBase, 174, AROS_SLIB_ENTRY(fmax881, STDC, 174));
        __AROS_SETVECADDR(StdCBase, 175, AROS_SLIB_ENTRY(fmin881, STDC, 175));
        __AROS_SETVECADDR(StdCBase, 103, AROS_SLIB_ENTRY(fpclassify881, STDC, 103));
        __AROS_SETVECADDR(StdCBase, 105, AROS_SLIB_ENTRY(isfinite881, STDC, 105));
        __AROS_SETVECADDR(StdCBase, 108, AROS_SLIB_ENTRY(isinf881, STDC, 108));
        __AROS_SETVECADDR(StdCBase, 111, AROS_SLIB_ENTRY(isnan881, STDC, 111));
        __AROS_SETVECADDR(StdCBase, 114, AROS_SLIB_ENTRY(isnormal881, STDC, 114));
        __AROS_SETVECADDR(StdCBase, 163, AROS_SLIB_ENTRY(lround881, STDC, 163));
        __AROS_SETVECADDR(StdCBase, 158, AROS_SLIB_ENTRY(nearbyint881, STDC, 158));
        __AROS_SETVECADDR(StdCBase, 162, AROS_SLIB_ENTRY(round881, STDC, 162));
        __AROS_SETVECADDR(StdCBase, 118, AROS_SLIB_ENTRY(signbit881, STDC, 118));
        __AROS_SETVECADDR(StdCBase, 125, AROS_SLIB_ENTRY(sin881, STDC, 125));
        __AROS_SETVECADDR(StdCBase, 126, AROS_SLIB_ENTRY(tan881, STDC, 126));
        __AROS_SETVECADDR(StdCBase, 132, AROS_SLIB_ENTRY(tanh881, STDC, 132));
        __AROS_SETVECADDR(StdCBase, 165, AROS_SLIB_ENTRY(trunc881, STDC, 165));
        __AROS_SETVECADDR(StdCBase, 121, AROS_SLIB_ENTRY(asin881, STDC, 121));
        __AROS_SETVECADDR(StdCBase, 129, AROS_SLIB_ENTRY(atanh881, STDC, 129));
        __AROS_SETVECADDR(StdCBase, 130, AROS_SLIB_ENTRY(cosh881, STDC, 130));
        __AROS_SETVECADDR(StdCBase, 133, AROS_SLIB_ENTRY(exp881, STDC, 133));
        __AROS_SETVECADDR(StdCBase, 166, AROS_SLIB_ENTRY(fmod881, STDC, 166));
        __AROS_SETVECADDR(StdCBase, 149, AROS_SLIB_ENTRY(hypot881, STDC, 149));
   }

   return TRUE;
}

ADD2INITLIB(STDCM68KMATH_Init, 0)
#endif
