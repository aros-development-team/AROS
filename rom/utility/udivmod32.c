/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UDivMod32 - divide two 32 bit numbers.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(ULONG, UDivMod32,

/*  SYNOPSIS */
	AROS_LHA(ULONG, dividend, D0),
	AROS_LHA(ULONG, divisor, D1),

/*  LOCATION */
	struct Library *, UtilityBase, 26, Utility)

/*  FUNCTION
	Perform the 32 bit unsigned division and modulus of dividend by
	divisor, that is dividend / divisor. Will return both the
	quotient and the remainder.

    INPUTS
	dividend	-   The number to divide into (numerator).
	divisor 	-   The number to divide by (denominator).

    RESULT
	For m68k assembly programmers,
	    D0: quotient
	    D1: remainder

	For HLL programmers,
	    the quotient

    NOTES
	The utility.library math functions are unlike all other utility
	functions in that they don't require the library base to be
	loaded in register A6, and they also save the values of the
	address registers A0/A1.

	This function is mainly to support assembly programers, and is
	probably of limited use to higher-level language programmers.

    EXAMPLE

    BUGS
	It is impossible for C programmers to obtain the value of
	remainder.

    SEE ALSO
	SDivMod32(), SMult32(), SMult64(), UMult32(), UMult64()

    INTERNALS
	May actually be handled by code that is in config/$(KERNEL)
	This is the case for m68k-native.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return dividend / divisor;

#if 0
#error The UDivMod32() emulation code does NOT work!

    /* This does _NOT_ work, so do not use it */

    UWORD a,b,c,d;
    UQUAD result;
    LONG quo, rem;

    a = dividend >> 16;
    b = dividend & 0xFFFF;
    c = divisor >> 16;
    d = divisor & 0xFFFF;

    /* See if the numerator is 32 bits or 16... */
    if(a == 0)
    {
	/* 16 bits */
	if(c != 0)
	{
	    /* 16/32 -> quo = 0; rem = dividend */
	    quo = 0;
	    rem = dividend;
	}
	else
	{
	    /* 16/16 -> can be done in native div */
	    quo = b / d;
	    rem = b % d;
	}
    }
    else
    {
	/* 32 bit numerator */
	if(c != 0)
	{
	    /* 32 bit denominator, quo ~= a/c */
	    quo = a/c;
	}
	else
	{
	    /* 16 bit denominator, quo ~= (a/d) * 65536 */
	    quo = (a / d) << 16;
	}
	/* Get the error */
	rem = dividend - UMult32(quo,divisor);

	/* Take the remainder down to zero */
	while(rem > 0)
	{
	    quo++;
	    rem -= divisor;
	}

	/* However a -ve remainder is silly,
	   this also catches the case when the remainder is < 0 from the
	   guess
	*/
	while(rem < 0)
	{
	    quo--;
	    rem += divisor;
	}
    }
    SET_HIGH32OF64(result, quo);
    SET_LOW32OF64(result, rem);

    return result;

#endif
    AROS_LIBFUNC_EXIT

} /* UDivMod32 */
