/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Support functions for the gradientslider class
    Lang: English
*/
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <graphics/gfxmacros.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>

#include "gradientslider_intern.h"

/***************************************************************************************************/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/***************************************************************************************************/

#define MATRIX_HALFTONE		0
#define MATRIX_BAYER4 		1
#define MATRIX_BAYER16		0
#define MATRIX_RECTANGULAR	0


#define MATRIX 	      		Bayer4
#define MATRIX_WIDTH  		Bayer4_Width
#define MATRIX_HEIGHT 		Bayer4_Height
#define MATRIX_MAXVAL 		Bayer4_MaxVal

/***************************************************************************************************/

#if MATRIX_HALFTONE

#define Halftone_Width 		6
#define Halftone_Height 	6
#define Halftone_MaxVal 	36

const UBYTE Halftone[6][6] =
{
    {15, 9,17,32,22,30},
    { 7, 1, 3,19,35,23},
    {13, 5,11,27,26,33},
    {31,21,29,16,10,18},
    {20,36,24, 8, 2, 4},
    {28,25,34,14, 6,12}
};

#endif

/***************************************************************************************************/

#if MATRIX_BAYER4

#define Bayer4_Width 		4
#define Bayer4_Height 		4
#define Bayer4_MaxVal 		16

const UBYTE Bayer4[4][4] =
{
    { 1, 9, 3,11},
    {13, 5,15, 7},
    { 4,12, 2,10},
    {16, 8,14, 6}	
};

#endif

/***************************************************************************************************/

#if MATRIX_BAYER16

#define Bayer16_Width 		16
#define Bayer16_Height 		16
#define Bayer16_MaxVal 		254

const UBYTE Bayer16[16][16] =
{
    {  1,235, 59,219, 15,231, 55,215,  2,232, 56,216, 12,228, 52,212},
    {129, 65,187,123,143, 79,183,119,130, 66,184,120,140, 76,180,116},
    { 33,193, 17,251, 47,207, 31,247, 34,194, 18,248, 44,204, 28,244},
    {161, 97,145, 81,175,111,159, 95,162, 98,146, 82,172,108,156, 92},
    {  9,225, 49,209,  5,239, 63,223, 10,226, 50,210,  6,236, 60,220},
    {137, 73,177,113,133, 69,191,127,138, 74,178,114,134, 70,188,124},
    { 41,201, 25,241, 37,197, 21,254, 42,202, 26,242, 38,198, 22,252},
    {169,105,153, 89,165,101,149, 85,170,106,154, 90,166,102,150, 86},
    {  3,233, 57,217, 13,229, 53,213,  0,234, 58,218, 14,230, 54,214},
    {131, 67,185,121,141, 77,181,117,128, 64,186,122,142, 78,182,118},
    { 35,195, 19,249, 45,205, 29,245, 32,192, 16,250, 46,206, 30,246},
    {163, 99,147, 83,173,109,157, 93,160, 96,144, 80,174,110,158, 94},
    { 11,227, 51,211,  7,237, 61,221,  8,224, 48,208,  4,238, 62,222},
    {139, 75,179,115,135, 71,189,125,136, 72,176,112,132, 68,190,126},
    { 43,203, 27,243, 39,199, 23,253, 40,200, 24,240, 36,196, 20,254},
    {171,107,155, 91,167,103,151, 87,168,104,152, 88,164,100,148, 84} 
};

#endif

/***************************************************************************************************/

#if MATRIX_RECTANGULAR

#define Rectangular_Width  	3
#define Rectangular_Height 	3
#define Rectangular_MaxVal 	4

const UBYTE Rectangular[3][3] =
{
    {2, 3, 2},
    {4, 1, 4},
    {2, 3, 2} 
};

#endif

/***************************************************************************************************/

STATIC VOID DitherV(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD pen1, WORD pen2,
		    struct GradientSliderBase_intern *GradientSliderBase)
{
    LONG width = x2 - x1 + 1;
    LONG height = y2 - y1 + 1;
    
    LONG x, y, t, v, pixel;

    if (height <= 2)
    {
    	SetAPen(rp, pen1);
    	RectFill(rp, x1,y1,x2,y2);
    }
    
    if (height == 2)
    {
    	SetAPen(rp, pen2);
    	RectFill(rp, x1, y1 + 1, x2, y2);
    }
    
    if (height <= 2) return;
    
    for(y = 0 ; y < height ; y++) 
    {
        /* v = brightness. Make it go from 0 at y = 0 to MATRIX_MAXVAL at y = height - 1 */
	
        v = (y * MATRIX_MAXVAL + (height - 1) / 2)  / (height - 1);

        for(x = 0 ; x < width ; x++)
        {   
	    /* t = threshold */
	         	   
            t = MATRIX[y % MATRIX_HEIGHT][x % MATRIX_WIDTH];

	    /* if brightness is smaller than threshold use pen1, otherwise pen2 */

            if(v < t)
                pixel = pen1;
            else
                pixel = pen2;

            SetAPen(rp, pixel);
            WritePixel(rp, x1 + x, y1 + y);
        }
    }
}

/***************************************************************************************************/

STATIC VOID DitherH(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD pen1, WORD pen2,
		    struct GradientSliderBase_intern *GradientSliderBase)
{
    LONG width = x2 - x1 + 1;
    LONG height = y2 - y1 + 1;
    
    LONG x, y, t, v, pixel;

    if (width <= 2)
    {
    	SetAPen(rp, pen1);
    	RectFill(rp, x1,y1,x2,y2);
    }
    
    if (width == 2)
    {
    	SetAPen(rp, pen2);
    	RectFill(rp, x1 + 1, y1, x2, y2);
    }
    
    if (width <= 2) return;
    
    for(x = 0 ; x < width ; x++) 
    {
        /* v = brightness. Make it go from 0 at x = 0 to MATRIX_MAXVAL at x = width - 1 */
	
        v = (x * MATRIX_MAXVAL + (width - 1) / 2)  / (width - 1);

        for(y = 0 ; y < height ; y++)
        {   
	    /* t = threshold */
	         	   
            t = MATRIX[y % MATRIX_HEIGHT][x % MATRIX_WIDTH];

	    /* if brightness is smaller than threshold use pen1, otherwise pen2 */

            if(v < t)
                pixel = pen1;
            else
                pixel = pen2;

            SetAPen(rp, pixel);
            WritePixel(rp, x1 + x, y1 + y);
        }
    }
}

/***************************************************************************************************/

VOID DrawGradient(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, UWORD *penarray,
		  WORD numpens, WORD orientation, struct GradientSliderBase_intern *GradientSliderBase)
{
    UWORD *pen = penarray;
    ULONG step, pos = 0;
    WORD  x, y, width, height, oldx, oldy, endx, endy;
    WORD  pen1, pen2, i;

    pen1 = *pen++;

    switch(orientation)
    {
        case LORIENT_VERT:
	    height = y2 - y1 + 1;
	    step = height * 65536 / (numpens - 1);
	    oldy = y1;
	    for(i = 0; i < numpens - 1; i++)
	    {
		pen2 = *pen++;
		pos += step;
		y = y1 + (pos / 65536);
		endy = y - 1;
		if (endy >= oldy)
		{
		    DitherV(rp, x1, oldy, x2, endy, pen1, pen2, GradientSliderBase);
		    pen1 = pen2;
		    oldy = y;
	        }
	    }
	    break;

        case LORIENT_HORIZ:
	    width = x2 - x1 + 1;
	    step = width * 65536 / (numpens - 1);
	    oldx = x1;
	    for(i = 0; i < numpens - 1;i++)
	    {
		pen2 = *pen++;
		pos += step;
		x = x1 + (pos / 65536);
		endx = x - 1;
		if (endx >= oldx)
		{

		    DitherH(rp, oldx, y1, endx, y2, pen1, pen2, GradientSliderBase);
		    pen1 = pen2;
		    oldx = x;
	        }
	    }
	    break;
	    
    } /* switch(orientation) */
    
}

/***************************************************************************************************/

VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left	 = EG(o)->LeftEdge;
    ibox->Top	 = EG(o)->TopEdge;
    ibox->Width  = EG(o)->Width;
    ibox->Height = EG(o)->Height;

    if (gi)
    {
	if (EG(o)->Flags & GFLG_RELRIGHT)
	    ibox->Left	 += gi->gi_Domain.Width - 1;

	if (EG(o)->Flags & GFLG_RELBOTTOM)
	    ibox->Top	 += gi->gi_Domain.Height - 1;

	if (EG(o)->Flags & GFLG_RELWIDTH)
	    ibox->Width  += gi->gi_Domain.Width;

	if (EG(o)->Flags & GFLG_RELHEIGHT)
	    ibox->Height += gi->gi_Domain.Height;
    }
}


/***************************************************************************************************/

VOID DrawDisabledPattern(struct RastPort *rport, struct IBox *gadbox, UWORD pen,
			 struct GradientSliderBase_intern *GradientSliderBase)
{
    UWORD pattern[] = { 0x8888, 0x2222 };

    EnterFunc(bug("DrawDisabledPattern(rp=%p, gadbox=%p, pen=%d)\n",
    		rport, gadbox, pen));

    SetDrMd( rport, JAM1 );
    SetAPen( rport, pen );
    SetAfPt( rport, pattern, 1);

    /* render disable pattern */
    RectFill( rport, gadbox->Left,
    		     gadbox->Top,
		     gadbox->Left + gadbox->Width - 1,
		     gadbox->Top + gadbox->Height -1 );
		         
    SetAfPt ( rport, NULL, 0);

    ReturnVoid("DrawDisabledPattern");
}

/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
