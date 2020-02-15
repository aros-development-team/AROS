/*
    Copyright © 2000-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

/*****************************************************************************************/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <gadgets/gradientslider.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/colorwheel.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*****************************************************************************************/

struct IntuitionBase 	*IntuitionBase;
struct GfxBase 		*GfxBase;
struct UtilityBase	*UtilityBase;
struct Library 		*GradientSliderBase;
struct Library 		*ColorWheelBase;

static struct Screen 	*scr;
static struct DrawInfo 	*dri;
static struct ViewPort	*vp;
static struct ColorMap	*cm;
static struct Window 	*win;
static struct Gadget 	*gradgad, *wheelgad;
static WORD		pen1 = -1, pen2 = -1;
static BOOL		truecolor;

static WORD 		pens[] = {1, 2, ~0};

/*****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("colorwheel: %s\n", msg);
    
    if (win)
    {
        if (wheelgad) RemoveGadget(win, wheelgad);
    	if (gradgad) RemoveGadget(win, gradgad);
	CloseWindow(win);
    }
    
    if (wheelgad) DisposeObject((Object *)wheelgad);
    if (gradgad) DisposeObject((Object *)gradgad);

    if (pen1 != -1) ReleasePen(cm, pen1);
    if (pen2 != -1) ReleasePen(cm, pen2);
    
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);
    
    if (ColorWheelBase) CloseLibrary(ColorWheelBase);
    if (GradientSliderBase) CloseLibrary(GradientSliderBase);
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

/*****************************************************************************************/

static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
        cleanup("Can't open intuition.library V39!");
    }
    
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
        cleanup("Can't open graphics.library V39!");
    }
    
    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 39)))
    {
        cleanup("Can't open utility.library V39!");
    }
    
    if (!(GradientSliderBase = OpenLibrary("Gadgets/gradientslider.gadget", 0)))
    {
        cleanup("Can't open gradientslider.gadget!");
    }
    
    if (!(ColorWheelBase = OpenLibrary("Gadgets/colorwheel.gadget", 0)))
    {
        cleanup("Can't open colorwheel.gadget!");
    }
}

/*****************************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
        cleanup("Can't lock pub screen!");
    }
       
    if (!(dri = GetScreenDrawInfo(scr)))
    {
        cleanup("Can't get screen drawinfo!");
    }
    
    vp = &scr->ViewPort;
    cm = vp->ColorMap;
    
    pen1 = ObtainPen(cm, -1, 0, 0, 0, PENF_EXCLUSIVE);
    pen2 = ObtainPen(cm, -1, 0, 0, 0, PENF_EXCLUSIVE);
    
    pens[0] = pen1;
    pens[1] = pen2;
    
    if ((pen1 == -1) || (pen2 == -1)) cleanup("Can't obtain 2 pens!");
    
    if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= 15) truecolor = TRUE;
}

/*****************************************************************************************/

#define BORDERX     4
#define BORDERY     4
#define SPACINGX    4
#define SPACINGY    4
#define GRADWIDTH   20

static void makegads(void)
{
    struct ColorWheelRGB    rgb;
    struct ColorWheelHSB    hsb;
    Object  	    	    *im;
    WORD    	    	    sizeheight = 14;
    WORD    	    	    gradx, grady, gradw, gradh;
    WORD    	    	    wheelx, wheely, wheelw, wheelh;
    
    im = NewObject(NULL, SYSICLASS, SYSIA_DrawInfo, (IPTR)dri, SYSIA_Which, SIZEIMAGE, TAG_DONE);
    if (im)
    {
    	sizeheight = ((struct Image *)im)->Height;
	DisposeObject(im);
    }
    
    wheelx = scr->WBorLeft + BORDERX;
    wheely = scr->WBorTop + scr->Font->ta_YSize + 1 + BORDERY;
    wheelw = -(scr->WBorLeft + scr->WBorRight + BORDERX * 2 + SPACINGX + GRADWIDTH);
    wheelh = -(scr->WBorTop + scr->Font->ta_YSize + 1 + sizeheight + BORDERY * 2);
    
    gradx = -(scr->WBorRight + BORDERX + GRADWIDTH) + 1;
    grady = scr->WBorTop + scr->Font->ta_YSize + 1 + BORDERY;
    gradw = GRADWIDTH;
    gradh = -(scr->WBorTop + scr->Font->ta_YSize + 1 + sizeheight + BORDERY * 2);
    
    gradgad = (struct Gadget *)NewObject(0, "gradientslider.gadget", GA_RelRight	, gradx,
								     GA_Top		, grady,
								     GA_Width		, gradw,
								     GA_RelHeight	, gradh,
								     GRAD_PenArray	, (IPTR)pens,
								     GRAD_KnobPixels	, 10,
								     PGA_Freedom	, LORIENT_VERT,
								     TAG_DONE);
					 
    if (!gradgad) cleanup("Can't create gradientslider gadget!");
    
    wheelgad = (struct Gadget *)NewObject(0, "colorwheel.gadget", GA_Left		, wheelx,
    								  GA_Top		, wheely,
								  GA_RelWidth		, wheelw,
								  GA_RelHeight		, wheelh,
								  GA_RelVerify		, TRUE,
								  WHEEL_Screen		, (IPTR)scr,
								  WHEEL_BevelBox	, TRUE,
								  WHEEL_GradientSlider	, (IPTR)gradgad,
								  GA_Previous		, (IPTR)gradgad,
								  ICA_TARGET		, ICTARGET_IDCMP,
								  TAG_DONE);
								  
    if (!wheelgad) cleanup("Can't create colorwheel gadget!");
    
    GetAttr(WHEEL_HSB, (Object *)wheelgad, (IPTR *)&hsb);
    hsb.cw_Brightness = 0xFFFFFFFF;
    ConvertHSBToRGB(&hsb, &rgb);
    
    SetRGB32(vp, pen1, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);
}

/*****************************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(0, WA_PubScreen	, (IPTR)scr,
    			    WA_Left		, 10,
			    WA_Top		, 20,
			    WA_Width		, 200,
			    WA_Height		, 190,
			    WA_MinWidth		, 50,
			    WA_MinHeight	, 50,
			    WA_MaxWidth		, 4000,
			    WA_MaxHeight	, 4000,
			    WA_AutoAdjust	, TRUE,
			    WA_Title		, (IPTR)"ColorWheel",
			    WA_CloseGadget	, TRUE,
			    WA_DragBar		, TRUE,
			    WA_DepthGadget	, TRUE,
			    WA_SizeGadget	, TRUE,
			    WA_SizeBBottom	, TRUE,
			    WA_Activate		, TRUE,
			    WA_ReportMouse	, TRUE,
			    WA_IDCMP		, IDCMP_CLOSEWINDOW | IDCMP_IDCMPUPDATE,
			    WA_Gadgets		, (IPTR)gradgad,
			    TAG_DONE);
			    
    if (!win) cleanup("Can't open window!");
			    
}

/*****************************************************************************************/

static void handleall(void)
{
    struct IntuiMessage *msg;
    BOOL		quitme = FALSE;
    
    while(!quitme)
    {
        WaitPort(win->UserPort);
	
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	        case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_IDCMPUPDATE:
		    {
		        struct ColorWheelRGB rgb;
			struct ColorWheelHSB hsb;
			struct TagItem	     *tags = (struct TagItem *)msg->IAddress;
			
			hsb.cw_Hue        = GetTagData(WHEEL_Hue, 0, tags);
			hsb.cw_Saturation = GetTagData(WHEEL_Saturation, 0, tags);
			hsb.cw_Brightness = 0xFFFFFFFF;

			ConvertHSBToRGB(&hsb, &rgb);
			
			SetRGB32(vp, pen1, rgb.cw_Red, rgb.cw_Green, rgb.cw_Blue);
			
			if (truecolor)
			{
			    RefreshGList(gradgad, win, 0, 1);
			}
		    }
		    break;
		    
	    } /* switch(msg->Class) */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while(!quitme) */
    
}

/*****************************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makegads();
    makewin();
    handleall();
    cleanup(0);
    
    return 0;
}

/*****************************************************************************************/
