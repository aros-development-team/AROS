/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 functions j0, j1, jn, y0, y1 and yn
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/* j0, j1, y0, y1 (Bessel functions) */
double j0(double x) {
    FORWARD_IF_NAN_OR_INF(j0, x);
    return __ieee754_j0(x);
}

double j1(double x) {
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

double y0(double x) {
    FORWARD_IF_NAN_OR_INF(y0, x);
    if (x <= 0.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_y0(x);
}

double y1(double x) {
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
