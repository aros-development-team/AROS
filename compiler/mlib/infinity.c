/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Definition of the magic infinity value.
    XXX Needs to be made better CPU dependant! Basically it assumes we are
    dealing with IEEE 754 floating point.
*/

#include <aros/system.h>

#if AROS_BIG_ENDIAN
char __infinity[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
#else
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
#endif
