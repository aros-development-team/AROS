/*
 * uldiv.c
 *
 *  Created on: Aug 12, 2009
 *      Author: misc
 */

#include <inttypes.h>

void __attribute__((noreturn)) __aeabi_ldiv0(uint64_t);

uint64_t __aeabi_uldiv(uint64_t a, uint64_t b)
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
		int first_bit_a = __builtin_clz(a);
		int first_bit_b = __builtin_clz(b);
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
