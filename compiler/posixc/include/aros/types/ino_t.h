#ifndef _AROS_TYPES_INO_T_H
#define _AROS_TYPES_INO_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    type definition(s)
        ino_t - the type used for file serial numbers
        ino64_t - the type used for large file access serial numbers
*/

#include <aros/cpu.h>

#if (__WORDSIZE==64)
typedef signed AROS_64BIT_TYPE __ino_t;
#else
typedef signed AROS_32BIT_TYPE __ino_t;
#endif
typedef signed AROS_64BIT_TYPE __ino64_t;

#if defined(__USE_XOPEN)
# if !defined(__ino_t_defined)
#  if !defined(__USE_FILE_OFFSET64)
typedef __ino_t ino_t;
#  else
typedef __ino64_t ino_t;
#  endif
#  define __ino_t_defined
# endif
# if defined(__USE_LARGEFILE64) && !defined(__ino64_t_defined)
typedef __ino64_t ino64_t;
#  define __ino64_t_defined
# endif
#endif

#endif /* _AROS_TYPES_INO_T_H */
