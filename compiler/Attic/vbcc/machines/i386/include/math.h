/*  Die Library ist auch noch unvollstaendig    */

#ifndef __MATH_H
#define __MATH_H 1

#ifndef PI
#define PI            3.14159265358979323846
#endif
#ifndef PI_2
#define PI_2          1.57079632679489661923
#endif

double sin(double);
double cos(double);
double tan(double);
double sinh(double);
double cosh(double);
double tanh(double);
double asin(double);
double acos(double);
double atan(double);
double exp(double);
double log(double);
double log10(double);
double pow(double,double);
double sqrt(double);
double ceil(double);
double floor(double);
double fabs(double);

double atan2(double,double);
double ldexp(double,int);
double frexp(double,int *);
double modf(double,double *);
double fmod(double,double);

#endif

