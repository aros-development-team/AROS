/*
 * $Id$
 *
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2009 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _QUAD_MATH_H
#include "quad_math.h"
#endif /* _QUAD_MATH_H */

/****************************************************************************/

/* Divide a 64 bit integer by a 32 bit integer, filling in a 64 bit quotient
   and returning a 32 bit remainder. */
ULONG
divide_64_by_32(QUAD * dividend,ULONG divisor,QUAD * quotient)
{
	QUAD dividend_cdef = (*dividend);
	ULONG dividend_ab = 0;
	LONG i;

	quotient->High = quotient->Low = 0;

	for(i = 0 ; i < 64 ; i++)
	{
		/* Shift the quotient left by one bit. */
		quotient->High = (quotient->High << 1);

		if((quotient->Low & 0x80000000UL) != 0)
			quotient->High |= 1;

		quotient->Low = (quotient->Low << 1);

		/* Shift the dividend left by one bit. We start
		 * with the most significant 32 bit portion.
		 */
		dividend_ab = (dividend_ab << 1);

		if((dividend_cdef.High & 0x80000000UL) != 0)
			dividend_ab |= 1;

		/* Now for the middle 32 bit portion. */
		dividend_cdef.High = (dividend_cdef.High << 1);

		if((dividend_cdef.Low & 0x80000000UL) != 0)
			dividend_cdef.High |= 1;

		/* Finally, the least significant portion. */
		dividend_cdef.Low = (dividend_cdef.Low << 1);

		/* Does the divisor actually divide the dividend? */
		if(dividend_ab >= divisor)
		{
			dividend_ab -= divisor;

			/* We could divide the divisor. Keep track of
			 * this and take care of an overflow condition.
			 */
			quotient->Low++;
			if(quotient->Low == 0)
				quotient->High++;
		}
	}

	return(dividend_ab);
}

/****************************************************************************/

/* Subtract a 64 bit integer from another 64 bit integer, producing a
   64 bit integer difference, returning a 32 bit integer that indicates
   whether or not an underflow occured. */
ULONG
subtract_64_from_64_to_64(const QUAD * const minuend,const QUAD * const subtrahend,QUAD * difference)
{
	QUAD extended_minuend;

	/* We may have to borrow if the minuend is less than the
	   subtrahend, so we set up a local variable to track
	   any underflow this might produce. */
	extended_minuend.High	= 0;
	extended_minuend.Low	= minuend->High;

	/* First step: take care of the least significant word. If
	   that produces a local underflow, borrow from the most
	   significant word. */
	if(minuend->Low < subtrahend->Low)
	{
		/* Borrow, and if there's nothing to be borrowed,
		   remember that we had an underflow. */
		if(extended_minuend.Low-- == 0)
			extended_minuend.High--;
	}

	difference->Low = minuend->Low - subtrahend->Low;

	/* Second step: take care of the most significant word. If
	   that produces a local underflow, remember that. */
	if(extended_minuend.Low < subtrahend->High)
		extended_minuend.High--;

	difference->High = extended_minuend.Low - subtrahend->High;

	/* Return the underflow, if any. */
	return(extended_minuend.High);
}
