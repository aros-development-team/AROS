/*****************************************************************************

    NAME */
#include <math.h>

        double exp10(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes 10 raised to the power of x (10^x).

    INPUTS
        x - the exponent to raise 10 to.

    RESULT
        The value of 10 raised to the power x.

    NOTES
        Internally implemented as exp(x * M_LN10).
        Domain: all real numbers.

    EXAMPLE
        double val = exp10(2.0); // val == 100.0

    BUGS
        Accuracy depends on the precision of the underlying exp() implementation.

    SEE ALSO
        exp(), pow()

    INTERNALS
        Uses __ieee754_exp(x * M_LN10) to compute the result.

******************************************************************************/