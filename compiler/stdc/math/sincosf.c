
#include <aros/debug.h>

#include "math.h"
#include "math_private.h"

void sincos(double x, double *sin, double *cos);

void sincosf(float x, float *sin, float *cos)
{
	double s, c;
	sincos(x, &s, &c);
	*sin = s;
	*cos = c;
}
