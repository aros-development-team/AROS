#ifndef _AROS_TYPES_FPOS_T_H
#define _AROS_TYPES_FPOS_T_H

/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$

    fpos_t type definition
*/

#include <aros/cpu.h>

typedef unsigned AROS_32BIT_TYPE __fpos_t;
typedef unsigned AROS_64BIT_TYPE __fpos64_t;

#if !defined(__fpos_t_defined)
# if !defined(__USE_FILE_OFFSET64)
typedef __fpos_t fpos_t;
# else
typedef __fpos64_t fpos_t;
# endif
# define __fpos_t_defined
#endif

#endif /* _AROS_TYPES_FPOS_T_H */
