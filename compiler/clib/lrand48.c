/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function lrand48()
    Lang: english
*/

#include <aros/machine.h>
#include <stdio.h>
extern void __calc_seed(unsigned short int xsubi[3]);
extern unsigned char __Xrand[8];

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long int lrand48 (

/*  SYNOPSIS */
	void)

/*  FUNCTION
        Compute a random integer between [0, 2^32-1]

    INPUTS
        None.

    RESULT
        Random number

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
  unsigned long int retval;
  
  __calc_seed(NULL);

  retval = *(long int *)&__Xrand[2];

  return (retval>>1);
} /* lrand48 */
