/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColorWheel function ConvertHSBToRGB()
    Lang: english
*/
#include <gadgets/colorwheel.h>
#include <math.h>
#include "colorwheel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/colorwheel.h>

        AROS_LH2(void, ConvertHSBToRGB,

/*  SYNOPSIS */
        AROS_LHA(struct ColorWheelHSB *, hsb, A0),
        AROS_LHA(struct ColorWheelRGB *, rgb, A1),

/*  LOCATION */
        struct Library *, ColorWheelBase, 5, ColorWheel)

/*  FUNCTION
        Converts a color from an HSB representation to an
        RGB representation

    INPUTS
        hsb - filled-in ColorWheelHSB structure containing
              the values to convert
        rgb - structure to receive the converted values

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if FIXED_MATH
    ULONG	 H, S, I, R, G, B;
    UWORD	 f, w, q, t;

    H = hsb->cw_Hue >> 16;
    S = hsb->cw_Saturation >> 16;
    I = hsb->cw_Brightness >> 16;

    if (S == 0) S = 1;

    if (H == 0xffff) H = 0;
    
    H = H * 6;    
    f = H;
    /*
    if( I == 0xffff )
    {    	
	w = ~S;
	q = ~((S * f)>>16);
    	t = ~((S * (0xffff-f))>>16);
    }
    else*/
    {
    	w = ( I * (0xffff - S) ) >> 16;
	q = ( I * (0xffff - ((S * f)>>16)) ) >> 16;
    	t = ( I * (0xffff - ((S * (0xffff - f))>>16)))>>16;
    }	

    switch (FIXED_TO_INT(H) % 6)
    {
	case 0:
            R = I;
            G = t;
            B = w;
            break;
	    
	case 1:
            R = q;
            G = I;
            B = w;
            break;
	    
	case 2:
            R = w;
            G = I;
            B = t;
            break;
	    
	case 3:
            R = w;
            G = q;
            B = I;
            break;
	    
	case 4:
            R = t;
            G = w;
            B = I;
            break;
	    
	case 5:
            R = I;
            G = w;
            B = q;
            break;
    } /* switch (i) */
	
	rgb->cw_Red   = R | ( R << 16 );
	rgb->cw_Green = G | ( G << 16 );
	rgb->cw_Blue  = B | ( B << 16 );

#else /* FIXED_MATH */

#if 1

    DOUBLE H, S, I, R = 0.0, G = 0.0, B = 0.0, f, w, q, t;
    LONG   i;

    H = ((DOUBLE) hsb->cw_Hue) / ((DOUBLE) 0xFFFFFFFF);
    S = ((DOUBLE) hsb->cw_Saturation) / ((DOUBLE) 0xFFFFFFFF);
    I = ((DOUBLE) hsb->cw_Brightness) / ((DOUBLE) 0xFFFFFFFF);

    if (S == 0.0)
    {
        S = 0.000001;
    }
    
    if (H == 1.0) H = 0.0;
    
    H = H * 6.0;
    i = (LONG)H;
    f = H - i;
    w = I * (1.0 - S);
    q = I * (1.0 - (S * f));
    t = I * (1.0 - (S * (1.0 - f)));

    switch (i)
    {
	case 0:
            R = I;
            G = t;
            B = w;
            break;
	    
	case 1:
            R = q;
            G = I;
            B = w;
            break;
	    
	case 2:
            R = w;
            G = I;
            B = t;
            break;
	    
	case 3:
            R = w;
            G = q;
            B = I;
            break;
	    
	case 4:
            R = t;
            G = w;
            B = I;
            break;
	    
	case 5:
            R = I;
            G = w;
            B = q;
            break;
	    
    } /* switch (i) */

    rgb->cw_Red   = (ULONG) rint (R * 0xFFFFFFFF);
    rgb->cw_Green = (ULONG) rint (G * 0xFFFFFFFF);
    rgb->cw_Blue  = (ULONG) rint (B * 0xFFFFFFFF);
                                       
#else

    DOUBLE H, S, I, R, G, B;

    H = ((DOUBLE) hsb->cw_Hue) / ((DOUBLE) 0xFFFFFFFF);
    S = ((DOUBLE) hsb->cw_Saturation) / ((DOUBLE) 0xFFFFFFFF);
    I = ((DOUBLE) hsb->cw_Brightness) / ((DOUBLE) 0xFFFFFFFF);

    H *= PI2;

    if (H < PI2 * 1.0/3.0)
    {
	B = (1.0/3.0) * (1.0 - S);
	R = (1.0/3.0) * (1.0 + (S*cos(H)) / (cos(PI2/6.0 - H)));
	G = 1.0 - (B+R);
    }
    else if (H < PI2 * 2.0/3.0)
    {
	H = H - PI2 * 1.0/3.0;
	R = (1.0/3.0) * (1.0 - S);
	G = (1.0/3.0) * (1.0 + (S*cos(H)) / (cos(PI2/6.0 - H)));
	B = 1.0 - (R+G);	
    }
    else  /* if (H < PI2) */
    {
	H = H - PI2 * 2.0/3.0;
	G = (1.0/3.0) * (1.0 - S);
	B = (1.0/3.0) * (1.0 + (S*cos(H)) / (cos(PI2/6.0 - H)));
	R = 1.0 - (G+B);		
    }

    rgb->cw_Red = (ULONG) rint (R * 0xFFFFFFFF);
    rgb->cw_Green = (ULONG) rint (G * 0xFFFFFFFF);
    rgb->cw_Blue = (ULONG) rint (B * 0xFFFFFFFF);

#endif

#endif /* FIXED_MATH */

    AROS_LIBFUNC_EXIT


} /* ConvertHSBToRGB */
