/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.

    C99 floating-point environment
*/

#ifndef _STDC_FLOAT_H_
#define	_STDC_FLOAT_H_

#include <aros/system.h>
#include <aros/types/int_t.h>

#define FLT_MANT_DIG        24
#define FLT_EPSILON         1.19209290E-07F
#define FLT_DIG             6
#define FLT_MIN_EXP         (-125)
#define FLT_MIN             1.17549435E-38F
#define FLT_MIN_10_EXP      (-37)
#define FLT_MAX_EXP         128
#define FLT_MAX             3.40282347E+38F
#define FLT_MAX_10_EXP      38

#define	DBL_MANT_DIG        53
#define	DBL_EPSILON         2.2204460492503131E-16
#define	DBL_DIG             15
#define	DBL_MIN_EXP         (-1021)
#define	DBL_MIN             2.2250738585072014E-308
#define	DBL_MIN_10_EXP      (-307)
#define	DBL_MAX_EXP         1024
#define	DBL_MAX             1.7976931348623157E+308
#define	DBL_MAX_10_EXP      308

#if (__ISO_C_VISIBLE >= 2011)
# define FLT_TRUE_MIN       1.40129846E-45F
# define FLT_DECIMAL_DIG    9
# define FLT_HAS_SUBNORM    1

# define DBL_TRUE_MIN       4.9406564584124654E-324
# define DBL_DECIMAL_DIG    17
# define DBL_HAS_SUBNORM    1
#endif

/* Use architecture-specific implementation if available */
#if defined __riscv64 || defined __riscv || defined __aarch64__
  /* 128-bit quad precision (e.g. IEEE binary128) */
  #define LDBL_MANT_DIG        113
  #define LDBL_EPSILON         1.925929944387235853055977942584927319E-34L
  #define LDBL_DIG             33
  #define LDBL_MIN_EXP         (-16381)
  #define LDBL_MIN             3.362103143112093506262677817321752603E-4932L
  #define LDBL_MIN_10_EXP      (-4931)
  #define LDBL_MAX_EXP         (+16384)
  #define LDBL_MAX             1.189731495357231765085759326628007016E+4932L
  #define LDBL_MAX_10_EXP      (+4932)
  #if __ISO_C_VISIBLE >= 2011
    #define LDBL_TRUE_MIN      6.475175119438025110924438958227646552E-4966L
    #define LDBL_DECIMAL_DIG   36
    #define LDBL_HAS_SUBNORM   1
  #endif
#elif defined __i386__ || defined __x86_64__ || defined __m68k__
  /* 80-bit extended precision */
  #define LDBL_MANT_DIG        64
  #define LDBL_EPSILON         1.084202172485504434007452800869941711E-19L
  #define LDBL_DIG             18
  #define LDBL_MIN_EXP         (-16381)
  #define LDBL_MIN             3.36210314311209350626267781732175260E-4932L
  #define LDBL_MIN_10_EXP      (-4931)
  #define LDBL_MAX_EXP         (+16384)
  #define LDBL_MAX             1.18973149535723176508575932662800702E+4932L
  #define LDBL_MAX_10_EXP      (+4932)
  #if __ISO_C_VISIBLE >= 2011
    #define LDBL_TRUE_MIN      3.64519953188247460252840593361941982E-4951L
    #define LDBL_DECIMAL_DIG   21
    #define LDBL_HAS_SUBNORM   1
  #endif
#elif defined __arm__
  /* long double is same as double */
  #define LDBL_MANT_DIG        DBL_MANT_DIG
  #define LDBL_EPSILON         ((long double)DBL_EPSILON)
  #define LDBL_DIG             DBL_DIG
  #define LDBL_MIN_EXP         DBL_MIN_EXP
  #define LDBL_MIN             ((long double)DBL_MIN)
  #define LDBL_MIN_10_EXP      DBL_MIN_10_EXP
  #define LDBL_MAX_EXP         DBL_MAX_EXP
  #define LDBL_MAX             ((long double)DBL_MAX)
  #define LDBL_MAX_10_EXP      DBL_MAX_10_EXP
  #if __ISO_C_VISIBLE >= 2011
    #define LDBL_TRUE_MIN      ((long double)DBL_TRUE_MIN)
    #define LDBL_DECIMAL_DIG   DBL_DECIMAL_DIG
    #define LDBL_HAS_SUBNORM   DBL_HAS_SUBNORM
  #endif
#else
  /* Fallback — assume 80-bit extended precision */
  #define LDBL_MANT_DIG        64
  #define LDBL_EPSILON         1.084202172485504434007452800869941711E-19L
  #define LDBL_DIG             18
  #define LDBL_MIN_EXP         (-16381)
  #define LDBL_MIN             3.36210314311209350626267781732175260E-4932L
  #define LDBL_MIN_10_EXP      (-4931)
  #define LDBL_MAX_EXP         (+16384)
  #define LDBL_MAX             1.18973149535723176508575932662800702E+4932L
  #define LDBL_MAX_10_EXP      (+4932)
  #if __ISO_C_VISIBLE >= 2011
    #define LDBL_TRUE_MIN      3.64519953188247460252840593361941982E-4951L
    #define LDBL_DECIMAL_DIG   21
    #define LDBL_HAS_SUBNORM   1
  #endif
#endif

# if (__ISO_C_VISIBLE >= 1999)
# if defined __riscv64 || defined __riscv || defined __aarch64__ || defined __arm__
#  define FLT_EVAL_METHOD    0
#  define DECIMAL_DIG        17
# else
#  if __WORDSIZE==64
#   define FLT_EVAL_METHOD   0
#  else
#   define FLT_EVAL_METHOD   (-1)
#  endif
#  define DECIMAL_DIG        21
# endif
#endif

#if !defined(STDC_NOINLINE) && !defined(STDC_NOINLINE_FLOAT) \
    && defined(_STDC_FENV_H_)
static inline int __flt_rounds(void) {
    switch (fegetround()) {
# ifdef FE_TOWARDZERO
        case FE_TOWARDZERO:     return 0;
# endif
# ifdef FE_TONEAREST
        case FE_TONEAREST:      return 1;
# endif
# ifdef FE_UPWARD
        case FE_UPWARD:         return 2;
# endif
# ifdef FE_DOWNWARD
        case FE_DOWNWARD:       return 3;
# endif
        default:                return -1;
    }
}
#else
__BEGIN_DECLS
extern int __flt_rounds(void);
__END_DECLS
#endif

#ifndef FLT_ROUNDS
#define FLT_ROUNDS (__flt_rounds())
#endif

#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif

#endif /* _STDC_FLOAT_H_ */
