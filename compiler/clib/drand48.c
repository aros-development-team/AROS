/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function drand48()
    Lang: english
*/

#ifndef AROS_NOFPU
#include <aros/machine.h>
#include <stdio.h>
extern void __calc_seed(unsigned short int xsubi[3]);
extern unsigned char __Xrand[8];

/*****************************************************************************

    NAME */
#include <stdlib.h>

	double drand48 (

/*  SYNOPSIS */
	void)

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
	srand48(), erand48()

    INTERNALS

    HISTORY

******************************************************************************/
{
  unsigned char array[8];
  unsigned short val;
  double * retval = (double *)&array[0];

  int shift;
  int i;

  __calc_seed(NULL);

#if (AROS_BIG_ENDIAN == 0)

  array[7] = 0x3f;

  val = (__Xrand[5] << 8) | __Xrand[4];
  
  shift = -3;
  if (val)
  {
    int newshift =-3;
    unsigned short mask = 0x8000;
  
    while (0 != mask)
    {
      if (0 != (val & mask))
      {
        shift = newshift;
        break;
      }
    
      mask = mask >> 1;
      newshift++;
    }
  }

  if (shift < 0) 
  {
    i = 7;
    array[0] = __Xrand[i-1] << (8+shift);
  }
  else
  {
    i = 6;
    array[0] = 0;
  }
  
  while (i >= 2)
  {
    if (shift > 0)
      array[i] = __Xrand[i-1] << shift | __Xrand[i-2] >> (8-shift);
    else
      array[i-1] = __Xrand[i-1] << (8+shift) | __Xrand[i-2] >> (-shift);

    i--;
    
  }

  array[6] = (array[6] & 0x0f) | ((11 - shift) << 4);

#else

#warning Missing implementation for big endian CPUs

#endif
  return *retval;
} /* drand48 */

#else

void drand48(void)
{
	return;
}

#endif /* AROS_NOFPU */
