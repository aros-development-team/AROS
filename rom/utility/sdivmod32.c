/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SDivMod32 - Divide two 32 bit numbers.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(QUAD, SDivMod32,

/*  SYNOPSIS */
	AROS_LHA(LONG, dividend, D0),
	AROS_LHA(LONG, divisor, D1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 25, Utility)

/*  FUNCTION
	Calculates the 32-bit signed division of dividend by divisor. That
	is dividend / divisor. Will return both the quotient and the
	remainder.

    INPUTS
	dividend    -	The number to divide.
	divisor     -	The to divide by.

    RESULT
	For m68k assembly programmers:
	    D0: quotient
	    D1: remainder
	Others:
	    The quotient is returned in the high 32 bits of the result.
	    The remainder in the low 32 bits.

    NOTES
	The utility.library math functions are unlike all other utility
	functions in that they don't require the library base to be
	loaded in register A6, and they also save the values of the
	address registers A0/A1.

	This function is mainly to support assembly programers, and is
	probably of limited use to higher-level language programmers.

    EXAMPLE

    BUGS
	It is very hard for a C programmer to obtain the value of the
	remainder. In fact, its pretty near impossible.

    SEE ALSO
	SMult32(), SMult64(), UDivMod32(), UMult32(), UMult64()

    INTERNALS
	This may be handled by code in config/$(KERNEL).

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return dividend / divisor;

#if 0
#error Sorry, but the SDivMod32() emulation code does NOT work...
    /*
	This does NOT work. Do not even try and use this code...
    */

    UWORD a,b,c,d;
    QUAD result;
    LONG quo, rem;
    BOOL neg;

    /* Fix everything up so that -ve signs don't vanish */
    if(dividend < 0)
    {
	neg = 1; dividend = -dividend;
    }
    else
	neg = 0;

    if(divisor < 0)
    {
	neg ^= 1; divisor = -divisor;
    }

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

    return result;

#endif

    AROS_LIBFUNC_EXIT
} /* SDivMod32 */
