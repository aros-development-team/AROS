/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Support functions for the colorwheel class
    Lang: English
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/colorwheel.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <intuition/classes.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <gadgets/colorwheel.h>

#include <math.h>

#include "colorwheel_intern.h"

/***************************************************************************************************/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#define SQR(x) 	((x) * (x))
#define CW_PI 	3.1415926535

#define USE_WRITEPIXELARRAY 1

/***************************************************************************************************/

BOOL CalcWheelColor(LONG x, LONG y, DOUBLE cx, DOUBLE cy, ULONG *hue, ULONG *sat)
{
    DOUBLE d, r, rx, ry, l, h, s;

    rx = (DOUBLE) cx - x;
    ry = (DOUBLE) y - cy;

    d = (SQR(cy) * SQR(rx) + SQR(cx) * SQR(ry) - SQR(cx) * SQR(cy));

    r = sqrt (SQR(rx) + SQR(ry));

    if (r != 0.0)
        h = atan2 (rx / r, ry / r);
    else
        h = 0.0;

    l = sqrt (SQR((cx * cos (h + 0.5 * CW_PI))) + SQR((cy * sin (h + 0.5 * CW_PI))));
    s = r / l;
    h = 360.0 * h / (2.0 * CW_PI) + 180;
    h = h / 360.0;
    
    if (s == 0.0)
        s = 0.00001;
    else if (s > 1.0)
        s = 1.0;

    *hue = (ULONG)rint (h * 0xFFFFFFFF);
    *sat = (ULONG)rint (s * 0xFFFFFFFF);
    
    return (d > 0.0) ? FALSE : TRUE;
}   
    
/***************************************************************************************************/

STATIC VOID TrueWheel(struct ColorWheelData *data, struct RastPort *rp, struct IBox *box,
		      struct ColorWheelBase_intern *ColorWheelBase)
{
    struct ColorWheelHSB 	hsb;
    struct ColorWheelRGB 	rgb;
    ULONG			col;
    WORD 			x, y, left, top, width, height;
    DOUBLE 			cx, cy;

    left   = box->Left   + 1;
    top    = box->Top    + 1;
    width  = box->Width  - 2;
    height = box->Height - 2;

    if (width & 1) width--;
    if (height & 1) height--;
    
    cx = (DOUBLE)width  / 2.0;
    cy = (DOUBLE)height / 2.0;
    
    hsb.cw_Brightness = 0xFFFFFFFF;
    
#if USE_WRITEPIXELARRAY
    if (data->rgblinebuffer) FreeVec(data->rgblinebuffer);
    data->rgblinebuffer = AllocVec(width * sizeof(LONG), MEMF_ANY);
#endif

    if (data->rgblinebuffer)
    {
        ULONG backrgb[3];
	ULONG backcol;
	
	GetRGB32(data->scr->ViewPort.ColorMap, 0, 1, backrgb);
	
	backcol = ((backrgb[0] >>  8) & 0xFF0000) |
		  ((backrgb[1] >> 16) & 0x00FF00) |
		  ((backrgb[2] >> 24) & 0x0000FF);
		  
	for(y = 0; y < height; y++)
	{
            for(x = 0; x < width; x++)
	    {
		if (CalcWheelColor(x, y, cx, cy, &hsb.cw_Hue, &hsb.cw_Saturation))
		{
	            ConvertHSBToRGB(&hsb, &rgb);
		    col = ((rgb.cw_Red   >>  8) & 0xFF0000) |
			  ((rgb.cw_Green >> 16) & 0x00FF00) |
			  ((rgb.cw_Blue  >> 24) & 0x0000FF);

		    data->rgblinebuffer[x] = col;
		} else {
		    data->rgblinebuffer[x] = backcol;
		}
		
	    } /* for(x = 0; x < width; x++) */
	    
	    
	    WritePixelArray(data->rgblinebuffer,
	    		    0,
			    0,
			    width * sizeof(LONG),
			    rp,
			    left,
			    top + y,
			    width,
			    1,
			    RECTFMT_ARGB);
			    
	} /* for(y = 0; y < height; y++) */
	
    } /* if (data->rgblinebuffer) */
    else
    {
    	for(y = 0; y < height; y++)
	{
            for(x = 0; x < width; x++)
	    {
		if (CalcWheelColor(x, y, cx, cy, &hsb.cw_Hue, &hsb.cw_Saturation))
		{
	            ConvertHSBToRGB(&hsb, &rgb);
		    col = ((rgb.cw_Red   >>  8) & 0xFF0000) |
			  ((rgb.cw_Green >> 16) & 0x00FF00) |
			  ((rgb.cw_Blue  >> 24) & 0x0000FF);

		    WriteRGBPixel(rp, left + x, top + y, col);
		}
		
	    } /* for(x = 0; x < width; x++) */
	    
	} /* for(y = 0; y < height; y++) */
	
    } /* data->rgbinebuffer == NULL */
}

/***************************************************************************************************/

VOID RenderWheel(struct ColorWheelData *data, struct RastPort *rp, struct IBox *box,
		 struct ColorWheelBase_intern *ColorWheelBase)
{
    WORD cx, cy, rx, ry;
    
    if ((box->Width < 8) || (box->Height < 8)) return;
    
    SetDrMd(rp, JAM1);
    
    if (CyberGfxBase && (GetBitMapAttr(rp->BitMap, BMA_DEPTH) >= 15))
    {
	TrueWheel(data, rp, box, ColorWheelBase);
    } else {
    }
    
    rx = box->Width / 2;
    ry = box->Height / 2;
    
    cx = box->Left + rx;
    cy = box->Top + ry;
    
    rx--; ry--;
        
    SetAPen(rp, data->dri->dri_Pens[SHADOWPEN]);
    DrawEllipse(rp, cx, cy, rx, ry);
    DrawEllipse(rp, cx, cy, rx - 1, ry);
    DrawEllipse(rp, cx, cy, rx, ry - 1);
    DrawEllipse(rp, cx, cy, rx - 1, ry - 1);
     
}

/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
/***************************************************************************************************/
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

void DrawDisabledPattern(struct RastPort *rport, struct IBox *gadbox, UWORD pen,
			 struct ColorWheelBase_intern *ColorWheelBase)
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
