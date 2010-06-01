/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

ULONG count_bits_set(ULONG x) {
    ULONG c, y = x;

    c = 0x55555555;
    y = ((y>>1) & c) + (y & c);
    c = 0x33333333;
    y = ((y>>2) & c) + (y & c);
    y = (y>>4) + y;
    c = 0x0f0f0f0f;
    y &= c;
    y = (y>>8) + y;
    y = (y>>16) + y;
    return y & 0x1f;
}
