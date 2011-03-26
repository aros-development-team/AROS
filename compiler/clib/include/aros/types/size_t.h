#ifndef _AROS_TYPES_SIZE_T_H
#define _AROS_TYPES_SIZE_T_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosstdc/include/aros/types/size_t.h 35477 2010-11-07T22:33:25.524511Z verhaegs  $

    size_t
*/

#include <aros/cpu.h>

#ifndef __GNUC__
typedef unsigned AROS_INTPTR_TYPE size_t;
#else
#define __need_size_t
#include <stddef.h>
#endif

#endif /* _AROS_TYPES_SIZE_T_H */
