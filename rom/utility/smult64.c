/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Signed 64 bit multiplication function.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(QUAD, SMult64,

/*  SYNOPSIS */
	AROS_LHA(LONG, arg1, D0),
	AROS_LHA(LONG, arg2, D1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 33, Utility)

/*  FUNCTION
	Compute the signed 64-bit product of arg1 * arg2.

    INPUTS
	arg1, arg2  -	32 bit signed numbers.

    RESULT
	arg1 * arg2

    NOTES
	For m68k assembly programmers, QUADs are returned in D0:D1 (with
	the high 32 bits in D0).

	The utility.library math functions are unlike all other utility
	functions in that they don't require the library base to be
	loaded in register A6, and they also save the values of the
	address registers A0/A1.

	This function is mainly to support assembly programers, and is
	probably of limited use to higher-level language programmers.

    EXAMPLE

    BUGS

    SEE ALSO
	utility/SMult32(), utility/UMult32(), utility/UMult64()

    INTERNALS
	Actually handled in config/$(KERNEL)/utility_math.s

	This is essentially SMult32(), but with the code to calculate
	the product of the high 32 bits of the multiplicands.

	In fact all that is added is the 2^32 * ac term (see docs for
	    SMult32().)

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	18-08-96    iaint   Modified SMult32().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If we have native support for 32 * 32 -> 64, use that.
       The compiler will complain if it cannot support QUAD's
       natively (GNU C can)
     */

    return (QUAD)arg1 * arg2;

#if 0
    /* This is partially the algoritm that is used, however for a
       more complete version see config/m68k-native/smult64.s

       This version has problems with:
	- adding the partial products together
	- setting the value of QUADs
    */

    QUAD product;
    WORD a0, a1, b0, b1;
    LONG part_prod;
    BOOL neg;

    /* Fix everything up so that -ve signs don't vanish */
    if(arg1 < 0)
    {
	neg = 1; arg1 = -arg1;
    }
    else
	neg = 0;

    if(arg2 <= 0)
    {
	neg ^= 1; arg2 = -arg2;
    }

    a0 = arg1 & 0xFFFF;
    a1 = (arg1 >> 16) & 0xFFFF;

    b0 = arg2 & 0xFFFF;
    b1 = (arg2 >> 16) & 0xFFFF;

    part_prod = (a0 * b1) + (a1 * b0);

    /* In case numbers are small - note, does NOT compile */
    if(a1 && b1)
	SET_HIGH32OF64(product, a1 * b1 + (part_prod >> 16));
    else
	SET_HIGH32OF64(product, (part_prod >> 16));

    SET_LOW32OF64(product, (part_prod & 0xFFFF) + (a0 * b0));

    return (neg ? NEG64(product) : product);
#endif

    AROS_LIBFUNC_EXIT
} /* SMult64 */
