#ifndef _STDINT_H_
#define _STDINT_H_

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Standard fixed sized integral types.
*/

#include <exec/types.h>			/* includes <aros/system.h> */

typedef QUAD		int64_t;        /* 64-bit signed integer   */
typedef UQUAD		uint64_t;	/* 64-bit unsigned integer */
typedef LONG		int32_t;        /* 32-bit signed integer   */
typedef ULONG		uint32_t;	/* 32-bit unsigned integer */
typedef WORD		int16_t;        /* 16-bit signed integer   */
typedef UWORD		uint16_t;	/* 16-bit unsigned integer */
typedef BYTE		int8_t;         /* 8-bit signed integer    */
typedef UBYTE		uint8_t;	/* 8-bit unsigned integer  */

typedef SIPTR		intptr_t;	/* Integer or Pointer      */
typedef IPTR		uintptr_t;	/* Unsigned integer or ptr */

/* Fast versions of these types */
#if defined AROS_64BIT_TYPE
typedef QUAD		int_fast64_t;
typedef UQUAD		uint_fast64_t;
#endif
typedef LONG		int_fast32_t;
typedef ULONG		uint_fast32_t;
typedef WORD		int_fast16_t;
typedef UWORD		uint_fast16_t;
typedef BYTE		int_fast8_t;
typedef UBYTE		uint_fast8_t;

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

/*
        TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO

	    int_least_	    8, 16, 32, 64 _t
	    uint_least_	    8, 16, 32, 64 _t

#if defined __cplusplus && defined __STDC_LIMIT_MACROS
	    INT<N>_MAX, INT<N>_MIN, UINT<N>_MAX
	    INT_LEAST<N>_MIN, INT_LEAST<N>_MAX, UINT_LEAST<N>_MAX
	    INT_FAST<N>_MIN, INT_FAST<N>_MAX, UINT_FAST<N>_MAX
	    INTPTR_MIN, INTPTR_MAX, UINTPTR_MAX

	    INTMAX_MIN, INTMAX_MAX, UINTMAX_MAX

	    PTRDIFF_MIN, _MAX
	    SIG_ATOMIC_MIN, _MAX (must be <= -127 or >= 127, or unsig >= 255)
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
