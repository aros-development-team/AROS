/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Signed 32 bit multiplication function.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(LONG, SMult32,

/*  SYNOPSIS */
	AROS_LHA(LONG, arg1, D0),
	AROS_LHA(LONG, arg2, D1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 23, Utility)

/*  FUNCTION
	Performs the signed 32-bit multiplication of arg1 * arg2 and
	returns a signed 32 bit value.

    INPUTS
	arg1, arg2  -	32 bit signed longs

    RESULT
	arg1 * arg2

    NOTES
	This can perform the multiplication either using the machines
	native instructions (if they exist), or in software using a
	simple algorithm based on expanding algebraic products.

	The utility.library math functions are unlike all other utility
	functions in that they don't require the library base to be
	loaded in register A6, and they also save the values of the
	address registers A0/A1.

	This function is mainly to support assembly programers, and is
	probably of limited use to higher-level language programmers.

    EXAMPLE

	LONG a = 352543;
	LONG b = -52464;
	LONG c = SMult32(a,b);
	c == -1315946768

    BUGS
	Of limited use to C programmers.

    SEE ALSO
	UMult32(), UMult64(), SMult64()

    INTERNALS
	May be handled by code in config/$(KERNEL), may not be...
	It is for m68k-native.

	For emulation we are performing the operation:

	    (2^16 * a + b) * (2^16 * c + d)
	  = 2^32 * ab + 2^16 * ad + 2^16 * bc + bd
	  = 2^32 * ab + 2^16 ( ad + bc ) + bd

	Now since the result is a 32-bit number, the 2^32 term will have
	no effect. (Since 2^32 > max (32-bit number)), as will the
	high part of ad + bc.

	Therefore:
	product = 2^16( ad + bc ) + bd

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	18-08-96    iaint   Implemented as described above.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If we have native support for 32 * 32 -> 32, use that. */

    return arg1 * arg2;

#if 0
    /* This is effectively what the emulation does, see also
	config/m68k-native/sumult32.s
    */

    UWORD a1, b1, a0, b0;

    a1 = (arg1 >> 16) & 0xffff;
    a0 = arg1 & 0xffff;
    b1 = (arg2 >> 16) & 0xffff;
    b0 = arg2 & 0xffff;

    return ((a0 * b1) + (a1 * b0)) << 16 + (b0 * a0);
#endif

    AROS_LIBFUNC_EXIT
} /* SMult32 */
