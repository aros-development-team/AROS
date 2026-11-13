/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 functions j0, j1, jn, y0, y1 and yn
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/* j0, j1, y0, y1 (Bessel functions) */

/*****************************************************************************

    NAME */
#include <math.h>

        double j0(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the Bessel function of the first kind of order zero, J0(x).

    INPUTS
        x - input value.

    RESULT
        The value of J0(x).

    NOTES
        Oscillatory function, used in many physics and engineering problems.
        Domain: all real numbers.

    EXAMPLE
        double val = j0(0.0); // val == 1.0

    BUGS
        May lose precision for very large x.

    SEE ALSO
        j1(), y0(), y1()

    INTERNALS
        Uses __ieee754_j0() implementation based on polynomial approximations.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(j0, x);
    return __ieee754_j0(x);
}

/*****************************************************************************

    NAME */

        double j1(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the Bessel function of the first kind of order one, J1(x).

    INPUTS
        x - input value.

    RESULT
        The value of J1(x).

    NOTES
        Oscillatory function, important in wave propagation.
        Domain: all real numbers.

    EXAMPLE
        double val = j1(0.0); // val == 0.0

    BUGS
        May lose precision for very large x.

    SEE ALSO
        j0(), y0(), y1()

    INTERNALS
        Uses __ieee754_j1() with polynomial approximations.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(j1, x);
    return __ieee754_j1(x);
}

/*****************************************************************************

    NAME */
        double jn(

/*  SYNOPSIS */
        int n, double x)

/*  FUNCTION
        Computes the Bessel function of the first kind of order `n` at `x`.

    INPUTS
        n - order of the Bessel function (integer).
        x - the point at which to evaluate.

    RESULT
        Returns the value of the Bessel function \( J_n(x) \).
        If `x` is infinite or NaN, forwards to __ieee754_jn.

    NOTES
        Commonly appears in solutions to wave and diffusion equations
        with cylindrical symmetry.

    EXAMPLE
        double r = jn(0, 1.0);  // J0(1.0)

    BUGS
        None known.

    SEE ALSO
        yn(), j0(), j1()

    INTERNALS
        Forwards to __ieee754_jn() with minimal input checks.

******************************************************************************/
{
    if (!isfinite(x)) {
        return __ieee754_jn(n, x);
    }
    return __ieee754_jn(n, x);
}


/*****************************************************************************

    NAME */

        double y0(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the Bessel function of the second kind of order zero, Y0(x).

    INPUTS
        x - input value, must be positive.

    RESULT
        The value of Y0(x).

    NOTES
        Domain: x > 0.
        Returns -inf or NaN for invalid domain inputs.

    EXAMPLE
        double val = y0(1.0);

    BUGS
        Undefined for x <= 0.

    SEE ALSO
        y1(), j0(), j1()

    INTERNALS
        Uses __ieee754_y0() with domain checks.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(y0, x);
    if (x <= 0.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_y0(x);
}

/*****************************************************************************

    NAME */

        double y1(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the Bessel function of the second kind of order one, Y1(x).

    INPUTS
        x - input value, must be positive.

    RESULT
        The value of Y1(x).

    NOTES
        Domain: x > 0.
        Returns -inf or NaN for invalid domain inputs.

    EXAMPLE
        double val = y1(1.0);

    BUGS
        Undefined for x <= 0.

    SEE ALSO
        y0(), j0(), j1()

    INTERNALS
        Uses __ieee754_y1() with domain checks.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(y1, x);
    if (x <= 0.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_y1(x);
}

/*****************************************************************************

    NAME */
        double yn(

/*  SYNOPSIS */
        int n, double x)

/*  FUNCTION
        Computes the Bessel function of the second kind (Neumann function)
        of order `n` at `x`.

    INPUTS
        n - order of the Bessel function (integer).
        x - the point at which to evaluate. Must be positive.

    RESULT
        Returns the value of the Bessel function \( Y_n(x) \).
        Returns NaN and sets errno to EDOM if `x` < 0.
        Returns -infinity and sets errno to ERANGE if `x` == 0.

    NOTES
        Commonly used in physics for problems with cylindrical symmetry
        involving singularities.

    EXAMPLE
        double r = yn(1, 2.0);  // Y1(2.0)

    BUGS
        None known.

    SEE ALSO
        jn(), y0(), y1()

    INTERNALS
        Forwards to __ieee754_yn() with domain checks.

******************************************************************************/
{
    if (!isfinite(x)) {
        return __ieee754_yn(n, x);
    }

    if (x < 0.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    } else if (x == 0.0) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return -INFINITY;
    }

    return __ieee754_yn(n, x);
}
