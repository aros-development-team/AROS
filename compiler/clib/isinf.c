/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

#include "__math.h"
#include <math.h>

#ifdef isinf
#   undef isinf
#endif
int isinf(double val)
{
    return __isinf(val);
} /* isinf */
