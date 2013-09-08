/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Definition of infinity and NaN.
*/

/* These definitions are used if no architecture-specific version of this file
 * is available. They assume we're dealing with IEEE 754 floating point. */

#include <aros/system.h>
#include <math.h>

#if AROS_BIG_ENDIAN
const union __infinity_un __infinity = { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 } };
#else
const union __infinity_un __infinity = { {0, 0, 0, 0, 0, 0, 0xf0, 0x7f} };
#endif

#if AROS_BIG_ENDIAN
const union __nan_un __nan = { { 0xff, 0xc0, 0, 0 } };
#else
const union __nan_un __nan = { { 0, 0, 0xc0, 0xff } };
#endif
