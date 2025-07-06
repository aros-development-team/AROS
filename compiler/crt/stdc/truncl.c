/*****************************************************************************

    NAME */
#include <math.h>

        long double truncl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Truncates fractional part of x (long double) toward zero.

    INPUTS
        x - input long double

    RESULT
        Integral part without fraction.

    NOTES
        Same behavior as trunc() for long double.

    EXAMPLE
        long double r = truncl(3.9L);  // 3.0L
        long double r2 = truncl(-3.9L); // -3.0L

    BUGS
        None known.

    SEE ALSO
        floorl(), ceill(), roundl()

    INTERNALS
        Bitwise clearing of fractional bits in long double format.

******************************************************************************/