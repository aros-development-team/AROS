#ifndef CLIB_MACROS_H
#define CLIB_MACROS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C macros
    Lang: english
*/

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define ABS(x) (((x)<0)?(-(x)):(x))

#endif /* CLIB_MACROS_H */
