/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
  #define XHIGH 4
  #define XMIDDLE 2
  #define XLOW 0
  #define AHIGH 4
  #define AMIDDLE 2
  #define ALOW 0
#else
  #define XHIGH 2
  #define XMIDDLE 4
  #define XLOW 6
  #define AHIGH 0
  #define AMIDDLE 2
  #define ALOW 4
#endif
  *(unsigned short *)&__Xrand[XHIGH]   = param[2];
  *(unsigned short *)&__Xrand[XMIDDLE] = param[1];
  *(unsigned short *)&__Xrand[XLOW]    = param[0];

  *(unsigned short *)&__arand[AHIGH]   = param[5];
  *(unsigned short *)&__arand[AMIDDLE] = param[4];
  *(unsigned short *)&__arand[ALOW]    = param[3];

  __crand = param[6];
} /* lcong48 */
