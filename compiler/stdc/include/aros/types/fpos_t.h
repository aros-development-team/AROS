#ifndef _AROS_TYPES_FPOS_T_H
#define _AROS_TYPES_FPOS_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    fpos_t type definition
*/

#include <aros/cpu.h>

#if (__WORDSIZE==64)
typedef unsigned AROS_64BIT_TYPE __fpos_t;
#else
typedef unsigned AROS_32BIT_TYPE __fpos_t;
#endif
typedef unsigned AROS_64BIT_TYPE __fpos64_t;

#if !defined(__fpos_t_defined)
# if !defined(__USE_FILE_OFFSET64)
typedef __fpos_t fpos_t;
# else
typedef __fpos64_t fpos_t;
# endif
# define __fpos_t_defined
#endif

#if defined(__USE_LARGEFILE64) && !defined(__fpos64_t_defined)
# define __fpos64_t_defined
typedef __fpos64_t fpos64_t;
#endif

#endif /* _AROS_TYPES_FPOS_T_H */
