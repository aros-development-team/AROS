#ifndef _AROS_TYPES_TIME_T_H
#define _AROS_TYPES_TIME_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    time_t type definition
*/

#include <aros/cpu.h>

typedef signed AROS_32BIT_TYPE __time_t;

#if !defined(__time_t_defined)
typedef __time_t time_t;
# define __time_t_defined
#endif

#endif /* _AROS_TYPES_TIME_T_H */
