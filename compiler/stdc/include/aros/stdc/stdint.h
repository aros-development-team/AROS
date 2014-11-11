#ifndef _STDC_STDINT_H_
#define _STDC_STDINT_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99 header file stdint.h
          Standard fixed sized integral types.
*/

#include <aros/cpu.h>

#include <aros/types/int_t.h>

#include <aros/types/intptr_t.h>
#include <aros/types/uintptr_t.h>


#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

#define INT8_MIN    (-128)
#define INT16_MIN   (-32767-1)
#define INT32_MIN   (-2147483647-1)
#define INT64_MIN   (-AROS_MAKE_INT64(9223372036854775807)-1)

#define INT8_MAX    (127)
#define INT16_MAX   (32767)
#define INT32_MAX   (2147483647)
#define INT64_MAX   (AROS_MAKE_INT64(9223372036854775807))

#define UINT8_MAX   (255)
#define UINT16_MAX  (65535)
#define UINT32_MAX  (4294967295U)
#define UINT64_MAX  (AROS_MAKE_UINT64(18446744073709551615))

#define SIG_ATOMIC_MIN  AROS_SIG_ATOMIC_MIN
#define SIG_ATOMIC_MAX  AROS_SIG_ATOMIC_MAX

#define INT_LEAST8_MIN      (-128)
#define INT_LEAST16_MIN     (-32767-1)
#define INT_LEAST32_MIN     (-2147483647-1)
#define INT_LEAST64_MIN     (-AROS_MAKE_INT64(9223372036854775807)-1)

#define INT_LEAST8_MAX      (127)
#define INT_LEAST16_MAX     (32767)
#define INT_LEAST32_MAX     (2147483647)
#define INT_LEAST64_MAX     (AROS_MAKE_INT64(9223372036854775807))

#define UINT_LEAST8_MAX     (255)
#define UINT_LEAST16_MAX    (65535)
#define UINT_LEAST32_MAX    (4294967295U)
#define UINT_LEAST64_MAX    (AROS_MAKE_UINT64(18446744073709551615))


#define INT_FAST8_MIN  (-128)
#define INT_FAST16_MIN (-32767-1)
#define INT_FAST32_MIN (-2147483647-1)
#define INT_FAST64_MIN (-AROS_MAKE_INT64(9223372036854775807)-1)

#define INT_FAST8_MAX  (127)
#define INT_FAST16_MAX (32767)
#define INT_FAST32_MAX (2147483647)
#define INT_FAST64_MAX (AROS_MAKE_INT64(9223372036854775807))

#define UINT_FAST8_MAX  (255)
#define UINT_FAST16_MAX (65535)
#define UINT_FAST32_MAX (4294967295U)
#define UINT_FAST64_MAX (AROS_MAKE_UINT64(18446744073709551615))


#if __WORDSIZE == 64
#define INTPTR_MIN  (-AROS_MAKE_INT64(9223372036854775807)-1)
#define INTPTR_MAX  (AROS_MAKE_INT64(9223372036854775807))
#define UINTPTR_MAX (AROS_MAKE_UINT64(18446744073709551615))
#else
#define INTPTR_MIN  (-2147483647-1)
#define INTPTR_MAX  (2147483647)
#define UINTPTR_MAX (4294967295U)
#endif

/* TODO:
	    PTRDIFF_MIN, _MAX
	    SIZE_MAX
	    WCHAR_MIN, _MAX (must be <= -127 or >= 127)
	    WINT_MIN, _MAX (must be <= -65535 or >= 65535)
*/

#endif


#if !defined __cplusplus || defined __STDC_CONSTANT_MACROS

/* Signed.  */
# define INT8_C(c)	c
# define INT16_C(c)	c
# define INT32_C(c)	c
# if __WORDSIZE == 64
#  define INT64_C(c)	c ## L
# else
#  define INT64_C(c)	c ## LL
# endif

/* Unsigned.  */
# define UINT8_C(c)	c
# define UINT16_C(c)	c
# define UINT32_C(c)	c ## U
# if __WORDSIZE == 64
#  define UINT64_C(c)	c ## UL
# else
#  define UINT64_C(c)	c ## ULL
# endif

/* Maximal type.  */
# if __WORDSIZE == 64
#  define INTMAX_C(c)	c ## L
#  define UINTMAX_C(c)	c ## UL
# else
#  define INTMAX_C(c)	c ## LL
#  define UINTMAX_C(c)	c ## ULL
# endif

/* Maximal integer (long long) size */
# define INTMAX_MIN     INT64_MIN
# define INTMAX_MAX     INT64_MAX

/* Maximal unsigned integer (unsigned long long) size */
# define UINTMAX_MAX     UINT64_MAX

#endif /* !__cplusplus || __STDC_CONSTANT_MACROS */

#endif /* _STDC_STDINT_H_ */
