/*
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <math.h>
#include <intuition/imageclass.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "mui.h"
#include "imspec_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/*
   sizeof(LONG)*8 gives the size in bits of the LONG type,
   then 8+1 bits (a color component size in bits, plus one bit
   to account for the sign of the delta) are subtracted from it,
   the result is the number of bits which can be used for the fractional
   part to do fixed point math with the maximum precision.

   Using all the remaining bits for the fractional part, IN THIS SPECIAL CASE,
   does not incurr in overflow problems for the integer part, for the way
   we use fractional numbers in this algorithm. Basically, we first scale up
   a number A, and then we divide A by the number B. Successively,
   the only operation we do on the number A is adding it to the number C,
   initially equal to 0, _at maximum_ B times, which means that overflow
   never happens.

   UPDATE: I've come up with a solution which makes this algorithm
           twice as fast as before, by taking advantage of the fact that
           a + b/c = (ac + b)/c, which means that now the number C is set
           to an initial value different than 0. There's no need to diminish
           the number of fractional bits as, for the way the algorithm
           works, there's still no overflow.

*/

#define SHIFT (sizeof(ULONG)*8 - (8 + 1))

STATIC VOID TrueDitherV
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD oy1, WORD oy2,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG max_delta_y = (oy2 - oy1 > 0) ? oy2 - oy1 : 1;
    LONG width       = x2  - x1 + 1;

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG step_r = (delta_r << SHIFT)/max_delta_y;
    LONG step_g = (delta_g << SHIFT)/max_delta_y;
    LONG step_b = (delta_b << SHIFT)/max_delta_y;

    LONG y, offset_y = y1 - oy1;

    LONG red   = ((1 << SHIFT) >> 1) + (start_rgb[0] << SHIFT) + offset_y*step_r;
    LONG green = ((1 << SHIFT) >> 1) + (start_rgb[1] << SHIFT) + offset_y*step_g;
    LONG blue  = ((1 << SHIFT) >> 1) + (start_rgb[2] << SHIFT) + offset_y*step_b;

    for(y = y1; y <= y2; y++)
    {
        FillPixelArray(rp, x1, y, width, 1,
                       ((red >> SHIFT) << 16) + ((green >> SHIFT) << 8) + (blue >> SHIFT));

        red   += step_r;
        green += step_g;
        blue  += step_b;
    }
}

STATIC VOID TrueDitherH
(
    struct RastPort *rp,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD ox1, WORD ox2,
    ULONG *start_rgb, ULONG *end_rgb
)
{
    LONG max_delta_x = (ox2 - ox1 > 0) ? ox2 - ox1 : 1;
    LONG height      = y2  - y1 + 1;

    LONG delta_r = end_rgb[0] - start_rgb[0];
    LONG delta_g = end_rgb[1] - start_rgb[1];
    LONG delta_b = end_rgb[2] - start_rgb[2];

    LONG step_r = (delta_r << SHIFT)/max_delta_x;
    LONG step_g = (delta_g << SHIFT)/max_delta_x;
    LONG step_b = (delta_b << SHIFT)/max_delta_x;

    LONG x, offset_x = x1 - ox1;

    /* 1 << (SHIFT - 1) is 0.5 in fixed point math. We add it to the variable
       so that, at the moment in which the variable is converted to integer,
       rounding is done properly. That is, a+x, with 0 < x < 0.5, is rounded
       down to a, and a+x, with 0.5 <= x < 1, is rounded up to a+1. */

    LONG red   = ((1 << SHIFT) >> 1) + (start_rgb[0] << SHIFT) + offset_x*step_r;
    LONG green = ((1 << SHIFT) >> 1) + (start_rgb[1] << SHIFT) + offset_x*step_g;
    LONG blue  = ((1 << SHIFT) >> 1) + (start_rgb[2] << SHIFT) + offset_x*step_b;

    for(x = x1; x <= x2; x++)
    {
        FillPixelArray(rp, x, y1, 1, height,
                       ((red >> SHIFT) << 16) + ((green >> SHIFT) << 8) + (blue >> SHIFT));

        red   += step_r;
        green += step_g;
        blue  += step_b;
    }
}

struct myrgb
{
    int red,green,blue;
};

/*****************************************************************
 Fill the given rectangle with a angle oriented gradient. The
 unit angle uses are degrees
******************************************************************/
STATIC int FillPixelArrayGradient(struct RastPort *rp, int xt, int yt, int xb, int yb, ULONG *start_rgb, ULONG *end_rgb, int angle)
{
    /* The basic idea of this algorithm is to calc the intersection between the
     * diagonal of the rectangle (xs,ys) with dimension (xw,yw) a with the line starting
     * at (x,y) (every pixel inside the rectangle) and angle angle with direction vector (vx,vy).
     * 
     * Having the intersection point we then know the color of the pixel.
     * 
     * TODO: Turn the algorithm into a incremental one
     *       Remove the use of floating point variables
     */

    double rad = angle*M_PI/180;
    double cosarc = cos(rad);
    double sinarc = sin(rad);

    struct myrgb startRGB,endRGB;
    int diffR, diffG, diffB;

    int r,t; /* some helper variables to short the code */
    int l,y,c,x;
    int y1; /* The intersection point */
    int incr_y1; /* increment of y1 */
    int xs,ys,xw,yw;
    int xadd,ystart,yadd;
//    double vx = -cosarc;
//    double vy = sinarc;
    int vx = (int)(-cosarc*0xff);
    int vy = (int)(sinarc*0xff);

    int width = xb - xt + 1;
    int height = yb - yt + 1;

    UBYTE *buf = (UBYTE*)AllocVec(width*3,0);
    if (!buf) return 0;

    startRGB.red = start_rgb[0];
    startRGB.green = start_rgb[1];
    startRGB.blue = start_rgb[2];

    endRGB.red = end_rgb[0];
    endRGB.green = end_rgb[1];
    endRGB.blue = end_rgb[2];

    diffR = endRGB.red - startRGB.red;
    diffG = endRGB.green - startRGB.green;
    diffB = endRGB.blue - startRGB.blue;

    /* Normalize the angle */
    if (angle < 0) angle = 360 - ((-angle)%360);
    if (angle >= 0) angle = angle % 360;

    if (angle <= 90 || (angle > 180 && angle <= 270))
    {
	/* The to be intersected diagonal goes from the top left edge to the bottom right edge */
	xs = 0;
	ys = 0;
	xw = width;
	yw = height;
    } else
    {
	/* The to be intersected diagonal goes from the bottom left edge to the top right edge */
	xs = 0;
	ys = height;
	xw = width;
	yw = -height;
    }
		
    if (angle > 90 && angle <= 270)
    {
	/* for these angle we have y1 = height - y1. Instead of
	 * 
	 *  y1 = height - (-vy*(yw*  xs -xw*  ys)         + yw*(vy*  x -vx*  y))        /(-yw*vx + xw*vy);
	 * 
	 * we can write
	 * 
         *  y1 =          (-vy*(yw*(-xs)-xw*(-ys+height)) + yw*(vy*(-x)-vx*(-y+height)))/(-yw*vx + xw*vy);
         * 
         * so height - y1 can be expressed with the normal formular adapting some parameters.
	 * 
	 * Note that if one would exchanging startRGB/endRGB the values would only work
	 * for linear color gradients
	 */
	xadd = -1;
	yadd = -1;
	ystart = height;

	xs = -xs;
	ys = -ys + height;
    } else
    {
	xadd = 1;
	yadd = 1;
	ystart = 0;
    }

    r = -vy*(yw*xs-xw*ys); 
    t = -yw*vx + xw*vy;

    /* The formular as shown above is
     * 
     * 	 y1 = ((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));
     * 
     * We see that only yw*(vy*x-vx*y) changes during the loop.
     * 
     * We write
     *   
     *   Current Pixel: y1(x,y) = (r + yw*(vy*x-vx*y))/t = r/t + yw*(vy*x-vx*y)/t
     *   Next Pixel:    y1(x+xadd,y) = (r + vw*(vy*(x+xadd)-vx*y))/t 
     *
     *   t*(y1(x+xadd,y) - y1(x,y)) = yw*(vy*(x+xadd)-vx*y) - yw*(vy*x-vx*y) = yw*vy*xadd;
     * 
     */

    incr_y1 = yw*vy*xadd;

    for (l = 0, y = ystart; l < height; l++, y+=yadd)
    {
	UBYTE *bufptr = buf;

	/* Calculate initial y1 accu, can be brought out of the loop as well (x=0). It's probably a
         * a good idea to add here also a value of (t-1)/2 to ensure the correct rounding
	 * This (and for r) is also a place were actually a overflow can happen |yw|=16 |y|=16. So for
         * vx nothing is left, currently 9 bits are used for vx or vy */
	int y1_mul_t_accu = r - yw*vx*y;

	for (c = 0, x = 0; c < width; c++, x+=xadd)
	{
	    int red,green,blue;

	    /* Calculate the intersection of two lines, this is not the fastet way to do but
	     * it is intuitive. Note: very slow! Will be optimzed later (remove FFP usage
             * and making it incremental)...update: it's now incremental and no FFP is used
             * but it probably can be optimized more by removing some more of the divisions and
             * further specialize the stuff here (use of three accus). */
/*	    y1 = (int)((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));*/
	    y1 = y1_mul_t_accu / t;
					
	    red = startRGB.red + (int)(diffR*y1/height);
	    green = startRGB.green + (int)(diffG*y1/height);
	    blue = startRGB.blue + (int)(diffB*y1/height);

	    /* By using full 32 bits this can be made faster as well */
	    *bufptr++ = red;
	    *bufptr++ = green;
	    *bufptr++ = blue;

	    y1_mul_t_accu += incr_y1;
	}
	/* By bringing building the gradient array in the same format as the RastPort BitMap a call
         * to WritePixelArray() can be made also faster */
	WritePixelArray(buf,0,0,width*3 /* srcMod */,
			rp,xt,yt+l,width,1,RECTFMT_RGB);
    }
    FreeVec(buf);
    return 1;
}

/***************************************************************************************************/

VOID zune_gradient_draw
(
    struct MUI_ImageSpec_intern *spec,
    struct MUI_RenderInfo *mri,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD xoff, WORD yoff
)
{
    ULONG *start_rgb = spec->u.gradient.start_rgb;
    ULONG *end_rgb   = spec->u.gradient.end_rgb;

    if (!(CyberGfxBase && (GetBitMapAttr(mri->mri_RastPort->BitMap, BMA_DEPTH) >= 15)))
        return;

    if (spec->u.gradient.obj == NULL)
	return;

    switch(spec->u.gradient.angle)
    {
        case 0:
        {
            LONG oy1 = _top(spec->u.gradient.obj), oy2 = _bottom(spec->u.gradient.obj);
            LONG delta_oy = (oy2 - oy1 > 0) ? oy2 - oy1 : 1;
            LONG hh = (delta_oy + 1)*2;
            LONG mid_y;

            yoff %= hh;

            if (yoff < 0)
                yoff += hh;

            oy1 -= yoff; oy2 -= yoff;

            if (y2 > oy2)
            {
                mid_y = y1 + delta_oy - yoff;

                if (yoff > delta_oy)
                {
                    ULONG *tmp = start_rgb;
                    start_rgb  = end_rgb;
                    end_rgb    = tmp;

                    mid_y += delta_oy;
                    oy1   += delta_oy;
                    oy2   += delta_oy;
                }
            }
            else
            {
                mid_y = y2;
            }

            TrueDitherV
            (
                mri->mri_RastPort,
                x1, y1, x2, mid_y,
                oy1, oy2,
                start_rgb, end_rgb
            );

            if (mid_y < y2)
            {
                TrueDitherV
                (
                    mri->mri_RastPort,
                    x1, mid_y+1, x2, y2,
                    oy1+delta_oy, oy2+delta_oy,
                    end_rgb, start_rgb
                );
            }

            break;
        }
        case 90:
        {
            LONG ox1 = _left(spec->u.gradient.obj), ox2 = _right(spec->u.gradient.obj);
            LONG delta_ox = (ox2 - ox1 > 0) ? ox2 - ox1 : 1;
            LONG ww = (delta_ox + 1)*2;
            LONG mid_x;


            xoff %= ww;
            if (xoff < 0)
                xoff += ww;

            ox1 -= xoff; ox2 -= xoff;

            if (x2 > ox2)
            {
                mid_x = x1 + delta_ox - xoff;

                if (xoff > delta_ox)
                {
                    ULONG *tmp = start_rgb;
                    start_rgb  = end_rgb;
                    end_rgb    = tmp;

                    mid_x += delta_ox;
                    ox1   += delta_ox;
                    ox2   += delta_ox;
                }
            }
            else
            {
                mid_x = x2;
            }

            TrueDitherH
            (
                mri->mri_RastPort,
                x1, y1, mid_x, y2,
                ox1, ox2,
                start_rgb, end_rgb
            );

            if (mid_x < x2)
            {
                TrueDitherH
                (
                    mri->mri_RastPort,
                    mid_x+1, y1, x2, y2,
                    ox1+delta_ox, ox2+delta_ox,
                    end_rgb, start_rgb
                );
            }

            break;
        }
	default:
	    FillPixelArrayGradient(mri->mri_RastPort, x1, y1, x2, y2, start_rgb, end_rgb, spec->u.gradient.angle);
	    break;
    } /* switch(angle) */
}

/*************************************************************************
 Converts a gradient string to a internal image spec

 The format of the string is:
  (h|H|v|V|<angle>),<start_r>,<start_g>,<start_b>-<end_r>,<end_g>,<end_b>
 where <angle> is given decimal. The rest represents hexadecimal 32 bit
 numbers.
**************************************************************************/
BOOL zune_gradient_string_to_intern(CONST_STRPTR str,
                                     struct MUI_ImageSpec_intern *spec)
{
    LONG converted;
    ULONG angle;
    ULONG start_r, start_g, start_b;
    ULONG end_r, end_g, end_b;

    if (!str || !spec) return FALSE;

    /* Find out about the gradient angle */
    switch (str[0])
    {
	case    'H':
	case    'h':
	        angle = 90;
	        converted = 1;
		break;

	case 'V':
	case 'v':
		angle = 0;
		converted = 1;
		break;

	default:
		converted = StrToLong((STRPTR)str,(LONG*)&angle);
		if (converted == -1) return FALSE;
		break;
    }

    str += converted;

    /* convert the color information */
    if (*str != ',') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&start_r);
    if (converted == -1) return FALSE;
    str += converted;

    if (*str != ',') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&start_g);
    if (converted == -1) return FALSE;
    str += converted;

    if (*str != ',') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&start_b);
    if (converted == -1) return FALSE;
    str += converted;

    if (*str != '-') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&end_r);
    if (converted == -1) return FALSE;
    str += converted;

    if (*str != ',') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&end_g);
    if (converted == -1) return FALSE;
    str += converted;

    if (*str != ',') return FALSE;
    str++;
    converted = HexToLong((STRPTR)str,&end_b);
    if (converted == -1) return FALSE;

    /* Fill in the spec */
    spec->u.gradient.angle = angle;

    spec->u.gradient.start_rgb[0] = start_r>>24;
    spec->u.gradient.start_rgb[1] = start_g>>24;
    spec->u.gradient.start_rgb[2] = start_b>>24;

    spec->u.gradient.end_rgb[0] = end_r>>24;
    spec->u.gradient.end_rgb[1] = end_g>>24;
    spec->u.gradient.end_rgb[2] = end_b>>24;

    return TRUE;
}

VOID zune_scaled_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
                                    STRPTR buf)
{
    sprintf(buf, "7:%ld,%08lx,%08lx,%08lx-%08lx,%08lx,%08lx",
                 (LONG)spec->u.gradient.angle,
                 spec->u.gradient.start_rgb[0]*0x01010101,
                 spec->u.gradient.start_rgb[1]*0x01010101,
                 spec->u.gradient.start_rgb[2]*0x01010101,
                 spec->u.gradient.end_rgb[0]*0x01010101,
                 spec->u.gradient.end_rgb[1]*0x01010101,
                 spec->u.gradient.end_rgb[2]*0x01010101);
}

VOID zune_tiled_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
                                    STRPTR buf)
{
    sprintf(buf, "8:%ld,%08lx,%08lx,%08lx-%08lx,%08lx,%08lx",
                 spec->u.gradient.angle,
                 spec->u.gradient.start_rgb[0]*0x01010101,
                 spec->u.gradient.start_rgb[1]*0x01010101,
                 spec->u.gradient.start_rgb[2]*0x01010101,
                 spec->u.gradient.end_rgb[0]*0x01010101,
                 spec->u.gradient.end_rgb[1]*0x01010101,
                 spec->u.gradient.end_rgb[2]*0x01010101);
}
