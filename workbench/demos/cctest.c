#include <aros/debug.h>

#include <stdio.h>
#include <string.h>

static char s[256];

void arrgh(double a, double b, double shouldbe_a, double shouldbe_b)
{
    sprintf(s,"\n\n\n**************** FPU CONDITION CODES TERRIBLY WRONG ************ %f %f %f %f \n\n\n", a, b, shouldbe_a, shouldbe_b);
    bug(s);
}

int main(void)
{
    double a, b;
    
    for(;;)
    {
	a = 1.0; b = 1.0;
	if (!(a == b)) arrgh(a,b,1.0,1.0);
	a = 2.0; b = 1.0;
	if (!(a > b)) arrgh(a,b,2.0,1.0);
	if (!(b < a)) arrgh(a,b,2.0,1.0);
	if (!(b != a)) arrgh(a,b,2.0,1.0);
	if (a == b) arrgh(a,b,2.0,1.0);
	if (a <= b) arrgh(a,b,2.0,1.0);
    }
    
    return 0;
}
