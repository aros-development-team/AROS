#ifndef _AROS_TYPES_NULL_H
#define _AROS_TYPES_NULL_H

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosstdc/include/aros/types/null.h 35477 2010-11-07T22:33:25.524511Z verhaegs  $

    NULL
*/

#ifdef NULL
#undef NULL
#endif

#ifndef __cplusplus
#define NULL ((void *)0)
#else
#define NULL 0
#endif

#endif /* _AROS_TYPES_NULL_H */
