#ifndef _FLEXGROUP_PRIVATE_H
#define _FLEXGROUP_PRIVATE_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <utility/hooks.h>

#define CLAMP(x, low, high) \
    (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* Per-child working set for one layout pass.  "main" is the axis we
   distribute along, "cross" is the other one. */
struct FlexCalc
{
    Object *child;
    LONG min, def, max;
    LONG cmin, cdef, cmax;
    LONG basis;
    LONG size;
    LONG weight;
    BOOL frozen;
};

struct Flexgroup_DATA
{
    struct Hook hook;           /* must stay first, see flex_data() */
    struct FlexCalc *calc;      /* grown on demand, never per layout */
    LONG calcsize;
    UBYTE direction;
    UBYTE justify;
    UBYTE align;
    WORD gap;
};

#endif /* _FLEXGROUP_PRIVATE_H */
