
/*****************************************************************************

    NAME */
#include <math.h>

        long double cabsl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Computes absolute value (modulus) of complex long double number.

    INPUTS
        x - complex number (real and imaginary parts)

    RESULT
        Returns magnitude sqrt(real² + imag²).

    NOTES
        Scales input to prevent overflow/underflow.

    EXAMPLE
        long double r = cabsl(3.0 + 4.0iL); // 5.0L

    BUGS
        None known.

    SEE ALSO
        cabs(), cabsf()

    INTERNALS
        Uses scaling and __ieee754_sqrtl() to compute sqrt(re² + im²),
        handling special values per IEEE rules.

******************************************************************************/