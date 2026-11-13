/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <softfloat.h>
#include <softfloat_types.h>

int __aeabi_dcmpun(float64_t, float64_t);

int
__aeabi_dcmpun(float64_t a, float64_t b)
{
	/*
	 * The comparison is unordered if either input is a NaN.
	 * Test for this by comparing each operand with itself.
	 * We must perform both comparisons to correctly check for
	 * signalling NaNs.
	 */
	return !f64_eq(a, a) || !f64_eq(b, b);
}