#ifndef _AROS_TYPES_OFF_T_H
#define _AROS_TYPES_OFF_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    definition(s) for the off_t type, used for 
    file offsets and sizes.

    provides the definitions for offset types, namely -:
        off_t - the type used for sizes/offsets accessing files

    on 32bit systems, if __USE_LARGEFILE64 is defined the following is
    also provided -:
        off64_t - the type used for sizes/offsets accessing large files
*/

#include <aros/cpu.h>

#if (__WORDSIZE==64)
typedef signed AROS_64BIT_TYPE __off_t;
#else
typedef signed AROS_32BIT_TYPE __off_t;
#endif
typedef signed AROS_64BIT_TYPE __off64_t;

#if !defined(__off_t_defined)
# if !defined(__USE_FILE_OFFSET64)
typedef __off_t off_t;
# else
typedef __off64_t off_t;
# endif
# define __off_t_defined
#endif

#if defined(__USE_LARGEFILE64) && !defined(__off64_t_defined)
# define __off64_t_defined
typedef __off64_t off64_t;
#endif

#endif /* _AROS_TYPES_OFF_T_H */
