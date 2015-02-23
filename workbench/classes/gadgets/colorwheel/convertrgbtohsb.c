/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColorWheel function ConvertRGBToHSB()
    Lang: english
*/
#include <gadgets/colorwheel.h>
#include <math.h>
#include "colorwheel_intern.h"

#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/*****************************************************************************

    NAME */
#include <proto/colorwheel.h>

        AROS_LH2(void, ConvertRGBToHSB,

/*  SYNOPSIS */
        AROS_LHA(struct ColorWheelRGB *, rgb, A0),
        AROS_LHA(struct ColorWheelHSB *, hsb, A1),

/*  LOCATION */
        struct Library *, ColorWheelBase, 6, ColorWheel)

/*  FUNCTION
        Converts a color from an RGB representation to an 
        HSB representation

    INPUTS
        rgb - filled-in ColorWheelRGB structure containing
              the values to convert
        hsb - structure to receive the converted values

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
    Fixed32 R, G, B, H = 0, S, I, max, min, delta;
    
    R = rgb->cw_Red >> 16;
    G = rgb->cw_Green >> 16;
    B = rgb->cw_Blue >> 16;

    max = MAX(MAX(R, G), B);
    min = MIN(MIN(R, G), B);
    
    I = max;
    
    S = (max != 0) ? ( (ULONG) ( ( max - min ) << 16 ) / (ULONG) max ) : 0;
    if( S >= (FIXED_ONE-1) ) S = FIXED_ONE-1;
    
    if (S == 0)
    {
        H = 0; /* -1.0; */
    }
    else
    {
	delta = max - min;

	if (R == max)
	{
            H = FixDiv( (G - B), delta );
	}
	else if (G == max)
	{
            H = INT_TO_FIXED( 2 ) + FixDiv( (B - R), delta );
	}
	else if (B == max)
	{
            H = INT_TO_FIXED( 4 ) + FixDiv( (R - G), delta );
        }
	
	H *= 60;

	if (H < 0 )
	{
            H += INT_TO_FIXED( 360 );
	}
	
	H /= 360;
    }	

    hsb->cw_Hue 	= H | ( H << 16 );
    hsb->cw_Saturation  = S | ( S << 16 );
    hsb->cw_Brightness  = I | ( I << 16 );

#else /* FIXED_MATH */

#if 1

    DOUBLE R, G, B, H = 0.0, S, I, max, min, delta;

    R = (DOUBLE) rgb->cw_Red / (DOUBLE) 0xFFFFFFFF;
    G = (DOUBLE) rgb->cw_Green / (DOUBLE) 0xFFFFFFFF;
    B = (DOUBLE) rgb->cw_Blue / (DOUBLE) 0xFFFFFFFF;

    max = MAX(MAX(R, G), B);
    min = MIN(MIN(R, G), B);
    
    I = max;
    
    if (max != 0.0)
    {
        S = (max - min) / max;
    }
    else
    {
        S = 0.0;
    }
    
    if (S == 0.0)
    {
        H = 0.0; /* -1.0; */
    }
    else
    {
	delta = max - min;

	if (R == max)
	{
            H = (G - B) / delta;
	}
	else if (G == max)
	{
            H = 2.0 + (B - R) / delta;
	}
	else if (B == max)
	{
            H = 4.0 + (R - G) / delta;
        }
	
	H = H * 60.0;

	if (H < 0.0)
	{
            H = H + 360;
	}
	
	H = H / 360.0;
    }
    
    hsb->cw_Hue 	= (ULONG) rint (H * 0xFFFFFFFF);
    hsb->cw_Saturation 	= (ULONG) rint (S * 0xFFFFFFFF);
    hsb->cw_Brightness 	= (ULONG) rint (I * 0xFFFFFFFF);
      
#else      
    DOUBLE R, G, B, H, S, I, a;

    R = (DOUBLE) rgb->cw_Red / (DOUBLE) 0xFFFFFFFF;
    G = (DOUBLE) rgb->cw_Green / (DOUBLE) 0xFFFFFFFF;
    B = (DOUBLE) rgb->cw_Blue / (DOUBLE) 0xFFFFFFFF;

    I = (R + G + B) / 3.0;
    a = MIN (MIN(R, G), B);
    S = 1.0 - (a/I);
    H = acos ((0.5*(R-G)+(R-B))/sqrt(pow(R-G, 2.0) + (R-B)*(G-B)));
    if (B/I > G/I)
	H = PI2 - H;
    H /= PI2;

    hsb->cw_Hue = (ULONG) rint (H * 0xFFFFFFFF);
    hsb->cw_Saturation = (ULONG) rint (S * 0xFFFFFFFF);
    hsb->cw_Brightness = (ULONG) rint (I * 0xFFFFFFFF);

#endif

#endif /* FIXED_MATH */

    AROS_LIBFUNC_EXIT
    
} /* ConvertRGBToHSB */
