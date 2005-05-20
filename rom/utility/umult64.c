/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unsigned 64-bit product of two 32-bit numbers.
    Lang: english
*/
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/utility.h>

	AROS_LH2(UQUAD, UMult64,

/*  SYNOPSIS */
	AROS_LHA(ULONG, arg1, D0),
	AROS_LHA(ULONG, arg2, D1),

/*  LOCATION */
	struct UtilityBase *, UtilityBase, 34, Utility)

/*  FUNCTION
	Compute the unsigned 64-bit product of arg1 * arg2.

    INPUTS
	arg1, arg2  -	32 bit unsigned numbers.

    RESULT
	arg1 * arg2

    NOTES
	For m68k assembly programmers, UQUADs are returned in D0:D1 (with
	the high 32 bits in D0.

	This function is really only for people programming in
	assembly on real Amigas. Most compilers will be able to do this
	math for you inline.

    EXAMPLE

    BUGS

    SEE ALSO
	SMult32(), UMult32(), SMult64()

    INTERNALS
	This may or may not be handled by code in config/$(KERNEL),
	for m68k-native it is...

	This is essentially UMult32(), but with the code to calculate
	the product of the high 32 bits of the multiplicands.

    HISTORY
	29-10-95    digulla automatically created from
			    utility_lib.fd and clib/utility_protos.h
	18-08-96    iaint   Modified UMult32().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If we have native support for 32 * 32 -> 64, use that. */
    return (UQUAD)arg1 * arg2;


#if 0
    /* This is partially the algoritm that is used, however for a
       more complete version see config/m68k-native/smult64.s

       This version has problems with:
	- adding the partial products together
	- setting the value of QUADs
    */

    UQUAD product;
    UWORD a0, a1, b0, b1;
    ULONG part_prod;

    a1 = (arg1 >> 16) & 0xffff;
    a0 = arg1 & 0xffff;
    b1 = (arg2 >> 16) & 0xffff;
    b0 = arg2 & 0xffff;

    part_prod = (a0 * b1) + (a1 * b0);

    SET_HIGH32OF64(product, (part_prod >> 16) + (a1 * b1));
    SET_LOW32OF64(product, ((part_prod & 0xFFFF) << 16) + (a0 * b0));

    return product;
#endif

    AROS_LIBFUNC_EXIT
} /* UMult64 */
