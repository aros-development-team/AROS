/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function seed48()
    Lang: english
*/

#include <aros/machine.h>
extern unsigned char __Xrand[8];
extern unsigned char __Xrand_buffer[6];
extern void __set_standardvalues(void);
extern void __copy_x_to_buffer();

/*****************************************************************************

    NAME */
#include <stdlib.h>

	unsigned short int * seed48 (

/*  SYNOPSIS */
	unsigned short int seed16v[3])

/*  FUNCTION
        Initialize the random number generator

    INPUTS
        seedval

    RESULT
        None

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	dran48()

    INTERNALS

    HISTORY

******************************************************************************/
{
  __copy_x_to_buffer();

#if (AROS_BIG_ENDIAN == 0)
  #define HIGH 4
  #define MIDDLE 2
  #define LOW 0
#else
  #define HIGH 2
  #define MIDDLE 4
  #define LOW 6
#endif
  *(unsigned short *)&__Xrand[HIGH]   = seed16v[2];
  *(unsigned short *)&__Xrand[MIDDLE] = seed16v[1];
  *(unsigned short *)&__Xrand[LOW]    = seed16v[0];

  __set_standardvalues();
  
  return (unsigned short int *)__Xrand_buffer;
} /* srand48 */
