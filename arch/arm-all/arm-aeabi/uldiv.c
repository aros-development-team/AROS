/*
 * uldiv.c
 *
 *  Created on: Aug 12, 2009
 *      Author: misc
 */

#include <inttypes.h>

void __attribute__((noreturn)) __aeabi_ldiv0(uint64_t);

uint64_t __uldiv(uint64_t a, uint64_t b)
{
	uint64_t ret = 0;
	if (b == 0)
	{
		__aeabi_ldiv0(a);
	}
	else if (b > a)
	{
		return 0;
	}
	else
	{
		int first_bit_a = __builtin_clz(a >> 32);
		int first_bit_b = __builtin_clz(b >> 32);
		
		if (first_bit_a == 32)
		    first_bit_a += __builtin_clz(a & 0xffffffff);
		if (first_bit_b == 32)
		    first_bit_b += __builtin_clz(b & 0xffffffff);
		    
		uint64_t mask = 0x00000001ULL << (first_bit_b-first_bit_a);
		b <<= (first_bit_b - first_bit_a);

		do
		{
			if (a >= b)
			{
				ret |= mask;
				a -= b;
			}

			mask >>=1;
			b >>=1;
		} while(mask);
	}
	return ret;
}

uint64_t __uldivmod_helper(uint64_t a, uint64_t b, uint64_t *remainder)
{
	uint64_t quotient;

	quotient = __uldiv(a, b);

	*remainder = a - b * quotient;

	return quotient;
}
