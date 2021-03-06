/*
    Copyright (C) 2018-2020, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <libraries/stdc.h>

#include <defines/stdc_LVO.h>

#include <proto/exec.h>

#ifndef STDC_STATIC

/* FPU math functions */
extern void AROS_SLIB_ENTRY(acos881, STDC, LVOacos)(void);
extern void AROS_SLIB_ENTRY(log2881, STDC, LVOlog2)(void);
extern void AROS_SLIB_ENTRY(log10881, STDC, LVOlog10)(void);
extern void AROS_SLIB_ENTRY(log881, STDC, LVOlog)(void);
extern void AROS_SLIB_ENTRY(remainder881, STDC, LVOremainder)(void);
extern void AROS_SLIB_ENTRY(sinh881, STDC, LVOsinh)(void);
extern void AROS_SLIB_ENTRY(sqrt881, STDC, LVOsqrt)(void);
extern void AROS_SLIB_ENTRY(atan881, STDC, LVOatan)(void);
extern void AROS_SLIB_ENTRY(ceil881, STDC, LVOceil)(void);
extern void AROS_SLIB_ENTRY(copysign881, STDC, LVOcopysign)(void);
extern void AROS_SLIB_ENTRY(cos881, STDC, LVOcos)(void);
extern void AROS_SLIB_ENTRY(exp2881, STDC, LVOexp2)(void);
extern void AROS_SLIB_ENTRY(fabs881, STDC, LVOfabs)(void);
extern void AROS_SLIB_ENTRY(fdim881, STDC, LVOfdim)(void);
extern void AROS_SLIB_ENTRY(floor881, STDC, LVOfloor)(void);
extern void AROS_SLIB_ENTRY(fma881, STDC, LVOfma)(void);
extern void AROS_SLIB_ENTRY(fmax881, STDC, LVOfmax)(void);
extern void AROS_SLIB_ENTRY(fmin881, STDC, LVOfmin)(void);
extern void AROS_SLIB_ENTRY(fpclassify881, STDC, LVO__fpclassifyf)(void);
extern void AROS_SLIB_ENTRY(isfinite881, STDC, LVO__isfinite)(void);
extern void AROS_SLIB_ENTRY(isinf881, STDC, LVO__isinf)(void);
extern void AROS_SLIB_ENTRY(isnan881, STDC, LVO__isnan)(void);
extern void AROS_SLIB_ENTRY(isnormal881, STDC, LVO__isnormal)(void);
extern void AROS_SLIB_ENTRY(lround881, STDC, LVOlround)(void);
extern void AROS_SLIB_ENTRY(nearbyint881, STDC, LVOnearbyint)(void);
extern void AROS_SLIB_ENTRY(round881, STDC, LVOround)(void);
extern void AROS_SLIB_ENTRY(signbit881, STDC, LVO__signbitf)(void);
extern void AROS_SLIB_ENTRY(sin881, STDC, LVOsin)(void);
extern void AROS_SLIB_ENTRY(tan881, STDC, LVOtan)(void);
extern void AROS_SLIB_ENTRY(tanh881, STDC, LVOtanh)(void);
extern void AROS_SLIB_ENTRY(trunc881, STDC, LVOtrunc)(void);
extern void AROS_SLIB_ENTRY(asin881, STDC, LVOasin)(void);
extern void AROS_SLIB_ENTRY(atanh881, STDC, LVOatanh)(void);
extern void AROS_SLIB_ENTRY(cosh881, STDC, LVOcosh)(void);
extern void AROS_SLIB_ENTRY(exp881, STDC, LVOexp)(void);
extern void AROS_SLIB_ENTRY(fmod881, STDC, LVOfmod)(void);
extern void AROS_SLIB_ENTRY(hypot881, STDC, LVOhypot)(void);

static int STDCM68KMATH_Init(struct StdCBase *StdCBase)
{
    if ((SysBase->AttnFlags & AFF_68060) && (SysBase->AttnFlags & AFF_FPU)) {
        /* Override supported MC68060+ math functions */
        __AROS_SETVECADDR(StdCBase, LVOhypot, AROS_SLIB_ENTRY(hypot881, STDC, LVOhypot));
        __AROS_SETVECADDR(StdCBase, LVOsqrt, AROS_SLIB_ENTRY(sqrt881, STDC, LVOsqrt));
        __AROS_SETVECADDR(StdCBase, LVOceil, AROS_SLIB_ENTRY(ceil881, STDC, LVOceil));
        __AROS_SETVECADDR(StdCBase, LVOcopysign, AROS_SLIB_ENTRY(copysign881, STDC, LVOcopysign));
        __AROS_SETVECADDR(StdCBase, LVOfabs, AROS_SLIB_ENTRY(fabs881, STDC, LVOfabs));
        __AROS_SETVECADDR(StdCBase, LVOfdim, AROS_SLIB_ENTRY(fdim881, STDC, LVOfdim));
        __AROS_SETVECADDR(StdCBase, LVOfloor, AROS_SLIB_ENTRY(floor881, STDC, LVOfloor));
        __AROS_SETVECADDR(StdCBase, LVOfma, AROS_SLIB_ENTRY(fma881, STDC, LVOfma));
        __AROS_SETVECADDR(StdCBase, LVOfmax, AROS_SLIB_ENTRY(fmax881, STDC, LVOfmax));
        __AROS_SETVECADDR(StdCBase, LVOfmin, AROS_SLIB_ENTRY(fmin881, STDC, LVOfmin));
        __AROS_SETVECADDR(StdCBase, LVO__fpclassifyf, AROS_SLIB_ENTRY(fpclassify881, STDC, LVO__fpclassifyf));
        __AROS_SETVECADDR(StdCBase, LVO__isfinite, AROS_SLIB_ENTRY(isfinite881, STDC, LVO__isfinite));
        __AROS_SETVECADDR(StdCBase, LVO__isinf, AROS_SLIB_ENTRY(isinf881, STDC, LVO__isinf));
        __AROS_SETVECADDR(StdCBase, LVO__isnan, AROS_SLIB_ENTRY(isnan881, STDC, LVO__isnan));
        __AROS_SETVECADDR(StdCBase, LVO__isnormal, AROS_SLIB_ENTRY(isnormal881, STDC, LVO__isnormal));
        __AROS_SETVECADDR(StdCBase, LVOlround, AROS_SLIB_ENTRY(lround881, STDC, LVOlround));
        __AROS_SETVECADDR(StdCBase, LVOnearbyint, AROS_SLIB_ENTRY(nearbyint881, STDC, LVOnearbyint));
        __AROS_SETVECADDR(StdCBase, LVOround, AROS_SLIB_ENTRY(round881, STDC, LVOround));
        __AROS_SETVECADDR(StdCBase, LVO__signbitf, AROS_SLIB_ENTRY(signbit881, STDC, LVO__signbitf));
        __AROS_SETVECADDR(StdCBase, LVOtrunc, AROS_SLIB_ENTRY(trunc881, STDC, LVOtrunc));
    }
   else if ((SysBase->AttnFlags & AFF_68881) || (SysBase->AttnFlags & AFF_68882)) {
        /* Override supported 68881/68882 math functions */
        __AROS_SETVECADDR(StdCBase, LVOacos, AROS_SLIB_ENTRY(acos881, STDC, LVOacos));
        __AROS_SETVECADDR(StdCBase, LVOlog2, AROS_SLIB_ENTRY(log2881, STDC, LVOlog2));
        __AROS_SETVECADDR(StdCBase, LVOlog10, AROS_SLIB_ENTRY(log10881, STDC, LVOlog10));
        __AROS_SETVECADDR(StdCBase, LVOlog, AROS_SLIB_ENTRY(log881, STDC, LVOlog));
        __AROS_SETVECADDR(StdCBase, LVOremainder, AROS_SLIB_ENTRY(remainder881, STDC, LVOremainder));
        __AROS_SETVECADDR(StdCBase, LVOsinh, AROS_SLIB_ENTRY(sinh881, STDC, LVOsinh));
        __AROS_SETVECADDR(StdCBase, LVOsqrt, AROS_SLIB_ENTRY(sqrt881, STDC, LVOsqrt));
        __AROS_SETVECADDR(StdCBase, LVOatan, AROS_SLIB_ENTRY(atan881, STDC, LVOatan));
        __AROS_SETVECADDR(StdCBase, LVOceil, AROS_SLIB_ENTRY(ceil881, STDC, LVOceil));
        __AROS_SETVECADDR(StdCBase, LVOcopysign, AROS_SLIB_ENTRY(copysign881, STDC, LVOcopysign));
        __AROS_SETVECADDR(StdCBase, LVOcos, AROS_SLIB_ENTRY(cos881, STDC, LVOcos));
        __AROS_SETVECADDR(StdCBase, LVOexp2, AROS_SLIB_ENTRY(exp2881, STDC, LVOexp2));
        __AROS_SETVECADDR(StdCBase, LVOfabs, AROS_SLIB_ENTRY(fabs881, STDC, LVOfabs));
        __AROS_SETVECADDR(StdCBase, LVOfdim, AROS_SLIB_ENTRY(fdim881, STDC, LVOfdim));
        __AROS_SETVECADDR(StdCBase, LVOfloor, AROS_SLIB_ENTRY(floor881, STDC, LVOfloor));
        __AROS_SETVECADDR(StdCBase, LVOfma, AROS_SLIB_ENTRY(fma881, STDC, LVOfma));
        __AROS_SETVECADDR(StdCBase, LVOfmax, AROS_SLIB_ENTRY(fmax881, STDC, LVOfmax));
        __AROS_SETVECADDR(StdCBase, LVOfmin, AROS_SLIB_ENTRY(fmin881, STDC, LVOfmin));
        __AROS_SETVECADDR(StdCBase, LVO__fpclassifyf, AROS_SLIB_ENTRY(fpclassify881, STDC, LVO__fpclassifyf));
        __AROS_SETVECADDR(StdCBase, LVO__isfinite, AROS_SLIB_ENTRY(isfinite881, STDC, LVO__isfinite));
        __AROS_SETVECADDR(StdCBase, LVO__isinf, AROS_SLIB_ENTRY(isinf881, STDC, LVO__isinf));
        __AROS_SETVECADDR(StdCBase, LVO__isnan, AROS_SLIB_ENTRY(isnan881, STDC, LVO__isnan));
        __AROS_SETVECADDR(StdCBase, LVO__isnormal, AROS_SLIB_ENTRY(isnormal881, STDC, LVO__isnormal));
        __AROS_SETVECADDR(StdCBase, LVOlround, AROS_SLIB_ENTRY(lround881, STDC, LVOlround));
        __AROS_SETVECADDR(StdCBase, LVOnearbyint, AROS_SLIB_ENTRY(nearbyint881, STDC, LVOnearbyint));
        __AROS_SETVECADDR(StdCBase, LVOround, AROS_SLIB_ENTRY(round881, STDC, LVOround));
        __AROS_SETVECADDR(StdCBase, LVO__signbitf, AROS_SLIB_ENTRY(signbit881, STDC, LVO__signbitf));
        __AROS_SETVECADDR(StdCBase, LVOsin, AROS_SLIB_ENTRY(sin881, STDC, LVOsin));
        __AROS_SETVECADDR(StdCBase, LVOtan, AROS_SLIB_ENTRY(tan881, STDC, LVOtan));
        __AROS_SETVECADDR(StdCBase, LVOtanh, AROS_SLIB_ENTRY(tanh881, STDC, LVOtanh));
        __AROS_SETVECADDR(StdCBase, LVOtrunc, AROS_SLIB_ENTRY(trunc881, STDC, LVOtrunc));
        __AROS_SETVECADDR(StdCBase, LVOasin, AROS_SLIB_ENTRY(asin881, STDC, LVOasin));
        __AROS_SETVECADDR(StdCBase, LVOatanh, AROS_SLIB_ENTRY(atanh881, STDC, LVOatanh));
        __AROS_SETVECADDR(StdCBase, LVOcosh, AROS_SLIB_ENTRY(cosh881, STDC, LVOcosh));
        __AROS_SETVECADDR(StdCBase, LVOexp, AROS_SLIB_ENTRY(exp881, STDC, LVOexp));
        __AROS_SETVECADDR(StdCBase, LVOfmod, AROS_SLIB_ENTRY(fmod881, STDC, LVOfmod));
        __AROS_SETVECADDR(StdCBase, LVOhypot, AROS_SLIB_ENTRY(hypot881, STDC, LVOhypot));
   }

   return TRUE;
}

ADD2INITLIB(STDCM68KMATH_Init, 0)
#endif
