/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function srand48()
    Lang: english
*/

#include <aros/machine.h>
extern unsigned char __Xrand[8];
extern void __set_standardvalues(void);


/*****************************************************************************

    NAME */
#include <stdlib.h>

	void srand48 (

/*  SYNOPSIS */
	long int seedval)

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
#if (AROS_BIG_ENDIAN == 0)
  /* little endian */
  char * ptr = &__Xrand[2];
  *(long int *)ptr = seedval;
  __Xrand[0] = 0x0e;
  __Xrand[1] = 0x33;

#else
  /* big endian */
  char * ptr = &__Xrand[2];
  *(long int *)ptr = seedval;
  __Xrand[6] = 0x33;
  __Xrand[7] = 0x0e;
#endif

  __set_standardvalues();
} /* srand48 */
