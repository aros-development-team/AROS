/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if a double is infinite.
    Lang: english
*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

/*
 * isinf(x) returns 1 is x is inf, else 0;
 * no branching!
 */

#ifndef AROS_NOFPU

#include "__math.h"
#include <math.h>

#ifdef isinf
#   undef isinf
#endif
int __isinf(double);

int isinf(double val)
{
    return __isinf(val);
} /* isinf */

#else

int isinf(void)
{
    return 0;
}

#endif /* AROS_NOFPU */
