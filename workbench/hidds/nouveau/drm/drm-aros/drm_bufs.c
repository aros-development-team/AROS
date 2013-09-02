/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"

int drm_order(unsigned long size)
{
    int order;
    unsigned long tmp;

    for (order = 0, tmp = size >> 1; tmp; tmp >>= 1, order++) ;

    if (size & (size - 1))
        ++order;

    return order;
}

