#ifndef _AROS_TYPES_NULL_H
#define _AROS_TYPES_NULL_H

/*
    Copyright © 2010-2012, The AROS Development Team. All rights reserved.
    $Id$

    NULL
*/

#ifdef NULL
#undef NULL
#endif

#ifndef __cplusplus
#define NULL ((void *)0)
#elif defined(__GNUG__)
/* Pointer sized: NULL also ends C++ varargs lists */
#define NULL __null
#else
#define NULL 0L
#endif

#endif /* _AROS_TYPES_NULL_H */
