/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function jrand48()
    Lang: english
*/

#include <aros/machine.h>
extern void __calc_seed(unsigned short int xsubi[3]);

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long int jrand48 (

/*  SYNOPSIS */
	unsigned short int xsubi[3])

/*  FUNCTION
        Compute a random integer between [0, 2^32-1]

    INPUTS
        None.

    RESULT
        Random number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	srand48(), erand48(), drand48()

    INTERNALS

    HISTORY

******************************************************************************/
{
  long int retval;
  
  __calc_seed(xsubi);

  retval = *(long int *)&xsubi[1];
  
  if (retval < 0)
    return (-retval | 0x80000000);

  return retval;
} /* jrand48 */
