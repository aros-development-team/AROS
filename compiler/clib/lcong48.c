/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Function srand48()
    Lang: english
*/

#include <aros/machine.h>
extern unsigned char __Xrand[8];
extern unsigned char __arand[8];
extern unsigned short __crand;


/*****************************************************************************

    NAME */
#include <stdlib.h>

	void lcong48 (

/*  SYNOPSIS */
	unsigned short int param[7])
/*  FUNCTION
        Initialize the random number generator

    INPUTS
        param[0-2] specify X
        param[3-5] specify a
        param[6] specifies c

    RESULT
        None

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	srand48()

    INTERNALS

    HISTORY

******************************************************************************/
{
#if (AROS_BIG_ENDIAN == 0)
  #define HIGH 4
  #define MIDDLE 2
  #define LOW 0
#else
  #define HIGH 2
  #define MIDDLE 4
  #define LOW 6
#endif
  *(unsigned short *)&__Xrand[HIGH]   = param[2];
  *(unsigned short *)&__Xrand[MIDDLE] = param[1];
  *(unsigned short *)&__Xrand[LOW]    = param[0];

  *(unsigned short *)&__arand[HIGH]   = param[5];
  *(unsigned short *)&__arand[MIDDLE] = param[4];
  *(unsigned short *)&__arand[LOW]    = param[3];

  __crand = param[6];
} /* lcong48 */
