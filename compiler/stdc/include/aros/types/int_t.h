#ifndef _AROS_TYPES_INT_T_H
#define _AROS_TYPES_INT_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    all int types
*/

#include <aros/cpu.h>

typedef signed AROS_64BIT_TYPE              int64_t;
typedef unsigned AROS_64BIT_TYPE            uint64_t;
typedef signed AROS_32BIT_TYPE              int32_t;
typedef unsigned AROS_32BIT_TYPE            uint32_t;
typedef signed AROS_16BIT_TYPE              int16_t;
typedef unsigned AROS_16BIT_TYPE            uint16_t;
typedef signed AROS_8BIT_TYPE               int8_t;
typedef unsigned AROS_8BIT_TYPE             uint8_t;

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


#endif /* _AROS_TYPES_INT_T_H */
