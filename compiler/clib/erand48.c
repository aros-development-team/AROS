/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function dran48()
    Lang: english
*/

#ifndef AROS_NOFPU
#include <aros/machine.h>
extern void __calc_seed(unsigned short int xsubi[3]);

/*****************************************************************************

    NAME */
#include <stdlib.h>

	double erand48 (

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
  unsigned char array[8];
  unsigned short val;
  double * retval = (double *)&array[0];
  unsigned char * cxsubi = (unsigned char *)xsubi;


  int shift;
  int i;

  __calc_seed(xsubi);

#if (AROS_BIG_ENDIAN == 0)  

  array[7] = 0x3f;

  val = (cxsubi[5] << 8) | cxsubi[4];
  
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
    array[0] = cxsubi[i-1] << (8+shift);
  }
  else
  {
    i = 6;
    array[0] = 0;
  }
  
  while (i >= 2)
  {
    if (shift > 0)
      array[i] = cxsubi[i-1] << shift | cxsubi[i-2] >> (8-shift);
    else
      array[i-1] = cxsubi[i-1] << (8+shift) | cxsubi[i-2] >> (-shift);

    i--;
    
  }

  array[6] = (array[6] & 0x0f) | ((11 - shift) << 4);

#else

#warning Missing implementation for big endian CPUs

#endif
  return *retval;
} /* erand48 */

#else

void erand48(void)
{
	return;
}

#endif /* AROS_NOFPU */
