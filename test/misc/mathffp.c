/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Because fload is defined to "int" later below,
   and we still need the real float in some places */
   
typedef float realfloat;

union kludge
{
    realfloat f;
    int i;
};

/* !!!! Where you see "float" later below, "int" will be
   used instead by the compiler !!!! */
   
#define float int

/* Because of the define above, the math protos/defines are
   changed in such a way that all parameters are assumed to
   be integers and also the return value is assumed to be
   integer !!!! */
   
#include <proto/exec.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <libraries/mathffp.h>
#include <stdio.h>

struct MathBase *MathBase;
struct MathTransBase *MathTransBase;

realfloat converttofloat(float ffpfloat)
{
    union kludge n;

    n.i = ffpfloat;
    n.i = SPTieee(n.i);
    
    return n.f;
}

void domul(realfloat a, realfloat b)
{
    union kludge x, y, res;
    
    x.f = a;
    x.i = SPFieee(x.i);
    
    y.f = b;
    y.i = SPFieee(y.i);
    
    res.i = SPMul(x.i, y.i);
    
    puts("");
    printf("mathffp  : %f x %f = %f (hex %x)\n",
	   converttofloat(x.i), converttofloat(y.i), converttofloat(res.i), res.i);
    printf("realfloat: %f x %f = %f\n",   
	   a, b, a * b);
    
}

int main(void)
{
    MathBase = (struct MathBase *)OpenLibrary("mathffp.library", 0);
    if (!MathBase) return;

    /* mathtrans.library is needed for SPFieee() function to convert
       a float to IEEE floating point format */
       
    MathTransBase = (struct MathTransBase *)OpenLibrary("mathtrans.library", 0);
    if (!MathTransBase) return;
    
    domul(1.0, 0.017812);
    domul(2.0, 0.017812);
    domul(3.0, 0.017812);
    domul(4.0, 0.017812);
    domul(5.0, 0.017812);
    domul(6.0, 0.017812);
    domul(7.0, 0.017812);
    domul(8.0, 0.017812);
    domul(9.0, 0.017812);
        
    CloseLibrary((struct Library *)MathTransBase);
    CloseLibrary((struct Library *)MathBase);
}
