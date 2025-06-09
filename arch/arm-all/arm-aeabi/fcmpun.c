/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <softfloat.h>
#include <softfloat_types.h>

int __aeabi_fcmpun(float32_t, float32_t);

int
__aeabi_fcmpun(float32_t a, float32_t b)
{
	/*
	 * The comparison is unordered if either input is a NaN.
	 * Test for this by comparing each operand with itself.
	 * We must perform both comparisons to correctly check for
	 * signalling NaNs.
	 */
	return !f32_eq(a, a) || !f32_eq(b, b);
}
