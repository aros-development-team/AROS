/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: ColorWheel function ConvertRGBToHSB()
    Lang: english
*/
#include <gadgets/colorwheel.h>
#include <math.h>
#include "colorwheel_intern.h"

#define MIN(a, b) ((a) < (b) ? a : b)

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
        hsb - structure to recieive the converted values

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        27-04-2000  lbischoff  implemented

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, ColorWheelBase)

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

    AROS_LIBFUNC_EXIT
} /* ConvertRGBToHSB */
