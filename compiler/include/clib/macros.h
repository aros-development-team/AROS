#ifndef CLIB_MACROS_H
#define CLIB_MACROS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: C macros
    Lang: english
*/

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define ABS(x) (((x)<0)?(-(x)):(x))

#endif /* CLIB_MACROS_H */
