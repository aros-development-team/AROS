#ifndef _AROS_TYPES_NULL_H
#define _AROS_TYPES_NULL_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosstdc/include/aros/types/null.h 35477 2010-11-07T22:33:25.524511Z verhaegs  $

    NULL
*/

#ifndef __cplusplus

#ifndef __GNUC__
#define NULL ((void *)0)
#else
#define __need_NULL
#include <stddef.h>
#endif

#else
#define NULL 0
#endif

#endif /* _AROS_TYPES_NULL_H */
