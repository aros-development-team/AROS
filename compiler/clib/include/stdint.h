#ifndef _STDINT_H_
#define _STDINT_H_

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Standard fixed sized integral types.
*/

#include <aros/cpu.h>

#ifndef __AROS_INT64_T_DECLARED
#define __AROS_INT64_T_DECLARED
typedef signed AROS_64BIT_TYPE              int64_t;
#endif

#ifndef __AROS_UINT64_T_DECLARED
#define __AROS_UINT64_T_DECLARED
typedef unsigned AROS_64BIT_TYPE            uint64_t;
#endif

#ifndef __AROS_INT32_T_DECLARED
#define __AROS_INT32_T_DECLARED
typedef signed AROS_32BIT_TYPE              int32_t;
#endif

#ifndef __AROS_UINT32_T_DECLARED
#define __AROS_UINT32_T_DECLARED
typedef unsigned AROS_32BIT_TYPE            uint32_t;
#endif

#ifndef __AROS_INT16_T_DECLARED
#define __AROS_INT16_T_DECLARED
typedef signed AROS_16BIT_TYPE              int16_t;
#endif

#ifndef __AROS_UINT16_T_DECLARED
#define __AROS_UINT16_T_DECLARED
typedef unsigned AROS_16BIT_TYPE            uint16_t;
#endif

#ifndef __AROS_INT8_T_DECLARED
#define __AROS_INT8_T_DECLARED
typedef signed AROS_8BIT_TYPE               int8_t;
#endif

#ifndef __AROS_UINT8_T_DECLARED
#define __AROS_UINT8_T_DECLARED
typedef unsigned AROS_8BIT_TYPE             uint8_t;
#endif

#ifndef __AROS_INTPTR_T_DECLARED
#define __AROS_INTPTR_T_DECLARED
typedef signed AROS_INTPTR_TYPE             intptr_t;
#endif

#ifndef __AROS_UINTPTR_T_DECLARED
#define __AROS_UINTPTR_T_DECLARED
typedef unsigned AROS_INTPTR_TYPE           uintptr_t;
#endif

/* Fast versions of these types */
typedef signed AROS_64BIT_FASTTYPE          int_fast64_t;
typedef unsigned AROS_64BIT_FASTTYPE        uint_fast64_t;
typedef signed AROS_32BIT_FASTTYPE          int_fast32_t;
typedef unsigned AROS_32BIT_FASTTYPE        uint_fast32_t;
typedef signed AROS_16BIT_FASTTYPE          int_fast16_t;
typedef unsigned AROS_16BIT_FASTTYPE        uint_fast16_t;
typedef signed AROS_8BIT_FASTTYPE           int_fast8_t;
typedef unsigned AROS_8BIT_FASTTYPE         uint_fast8_t;

/* Minimum sized types */
typedef signed AROS_64BIT_LEASTTYPE         int_least64_t;
typedef unsigned AROS_64BIT_LEASTTYPE       uint_least64_t;
typedef signed AROS_32BIT_LEASTTYPE         int_least32_t;
typedef unsigned AROS_32BIT_LEASTTYPE       uint_least32_t;
typedef signed AROS_16BIT_LEASTTYPE         int_least16_t;
typedef unsigned AROS_16BIT_LEASTTYPE       uint_least16_t;
typedef signed AROS_8BIT_LEASTTYPE          int_least8_t;
typedef unsigned AROS_8BIT_LEASTTYPE        uint_least8_t;


/*
    Use the AROS_LARGEST_TYPE to describe the intmax_t and uintmax_t
*/
#if defined AROS_LARGEST_TYPE
typedef signed   AROS_LARGEST_TYPE  intmax_t;
typedef unsigned AROS_LARGEST_TYPE  uintmax_t;
#else
typedef LONG	intmax_t;
typedef ULONG	uintmax_t;
#endif

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

#define SIG_ATOMIC_MIN  AROS_SIG_ATOMIC_MIN
#define SIG_ATOMIC_MAX  AROS_SIG_ATOMIC_MAX

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

#endif


/*
        TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO

#if defined __cplusplus && defined __STDC_LIMIT_MACROS
	    INT_LEAST<N>_MIN, INT_LEAST<N>_MAX, UINT_LEAST<N>_MAX
	    INT_FAST<N>_MIN, INT_FAST<N>_MAX, UINT_FAST<N>_MAX
	    INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX

	    INTMAX_MIN, INTMAX_MAX, UINTMAX_MAX

	    PTRDIFF_MIN, _MAX
	    SIZE_MAX
	    WCHAR_MIN, _MAX (must be <= -127 or >= 127)
	    WINT_MIN, _MAX (must be <= -65535 or >= 65535)
#endif

#if defined __cplusplus && defined __STD_CONSTANT_MACROS
	    INT<N>_C(value) constant to int_least<N>_t
	    UINT<N>_C(vlaue) constant to uint_least<N>_t

	    eg UINT64_C(0x1234)	=>  0x1234ULL

	    INTMAX_C(), UINTMAX_C()
#endif

*/

#endif /* _STDINT_H_ */
