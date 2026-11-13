/*****************************************************************************

    NAME */
#include <complex.h>
#include <math.h>

        double complex cabs(

/*  SYNOPSIS */
        double complex z)

/*  FUNCTION
        Computes the magnitude (absolute value) of a complex number.

    INPUTS
        z - complex number.

    RESULT
        The magnitude sqrt(real(z)^2 + imag(z)^2).

    NOTES
        Avoids overflow/underflow by scaling.

    EXAMPLE
        double complex z = 3.0 + 4.0*I;
        double r = cabs(z);  // r = 5.0

    BUGS
        None known.

    SEE ALSO
        cabsf(), cabsl()

    INTERNALS
        Calls __ieee754_hypot(real(z), imag(z)) internally.
        Uses scaling to maintain precision.

******************************************************************************/