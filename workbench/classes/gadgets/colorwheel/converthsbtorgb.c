/*
    (C) 2000 AROS - The Amiga Research OS
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
        rgb - structure to recieive the converted values

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

    AROS_LIBFUNC_EXIT
} /* ConvertHSBToRGB */
