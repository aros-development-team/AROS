/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function dran48()
    Lang: english
*/

#include <aros/machine.h>
extern void __calc_seed(unsigned short int xsubi[3]);

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void _erand48 (

/*  SYNOPSIS */
	unsigned short int xsubi[3])

/*  FUNCTION
        Compute a non-negative double-precision floating-point value
        uniformly distributed between [0.0, 1.0).

    INPUTS
        None.

    RESULT
        Double precision floating point number between [0.0, 1.0).

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	srand48(), erand48(), drand48()

    INTERNALS

    HISTORY

******************************************************************************/
{
  return;
} /* erand48 */
