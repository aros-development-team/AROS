
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/colorwheel.h>
#include <proto/intuition.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREENWIDTH  300
#define SCREENHEIGHT 200
#define SCREENCY (SCREENHEIGHT / 2)

/***********************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct Library *ColorWheelBase;
struct Screen *scr;
struct Window *win;
struct RastPort *rp;

ULONG cgfx_coltab[256];
UBYTE Keys[128];

/***********************************************************************************/

static void cleanup(char *msg)
{
    if (msg)
    {
        printf("TrueColorPens: %s\n",msg);
    }
    
    if (win) CloseWindow(win);
    
    if (scr) UnlockPubScreen(0, scr);
    
    if (ColorWheelBase) CloseLibrary(ColorWheelBase);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

/***********************************************************************************/

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
    
    if (!(CyberGfxBase = OpenLibrary("cybergraphics.library",0)))
    {
        cleanup("Can't open cybergraphics.library!");
    }

    if (!(ColorWheelBase = OpenLibrary("gadgets/colorwheel.gadget",0)))
    {
        cleanup("Can't open colorwheel.gadget!");
    }

}

/***********************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        cleanup("Can't lock pub screen!");
    }
    
    if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) <= 8)
    {
        cleanup("Need hi or true color screen!");
    }
}

/***********************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(NULL, WA_CustomScreen	, (IPTR)scr, 
    			       WA_InnerWidth	, SCREENWIDTH,
    			       WA_InnerHeight	, SCREENHEIGHT,
			       WA_Title		, (IPTR)"TrueColorPens: Press SPACE!",
			       WA_DragBar	, TRUE,
			       WA_DepthGadget	, TRUE,
			       WA_CloseGadget	, TRUE,
			       WA_Activate	, TRUE,
			       WA_GimmeZeroZero , TRUE,
			       WA_IDCMP		, IDCMP_CLOSEWINDOW |
			       			  IDCMP_RAWKEY,
			       TAG_DONE);
			       
   if (!win) cleanup("Can't open window");

   rp = win->RPort; 
}

/***********************************************************************************/

#define KC_LEFT         0x4F
#define KC_RIGHT     	0x4E
#define KC_UP        	0x4C
#define KC_DOWN      	0x4D
#define KC_ESC       	0x45
#define KC_SPACE    	0x40

/***********************************************************************************/

static void getevents(void)
{
    struct IntuiMessage *msg;
    
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        switch(msg->Class)
	{
	    case IDCMP_CLOSEWINDOW:
	        Keys[KC_ESC] = 1;
		break;
		
	    case IDCMP_RAWKEY:
	        {
		    WORD code = msg->Code & ~IECODE_UP_PREFIX;
		    
		    Keys[code] = (code == msg->Code) ? 1 : 0;

		}
	        break;

	}
        ReplyMsg((struct Message *)msg);
    }

}

/***********************************************************************************/

static void action(void)
{
    LONG mode = 0;
    ULONG hue = 0, rgb;
    WORD x = 20, y = 0;
    WORD dx = 2, dy = 3;
    
    while(!Keys[KC_ESC])
    {
    	struct ColorWheelHSB cwhsb = {hue, 0xFFFFFFFF, 0xFFFFFFFF};
	struct ColorWheelRGB cwrgb;

    	WaitTOF();
	
    	ConvertHSBToRGB(&cwhsb, &cwrgb);
		
    	rgb = (cwrgb.cw_Red & 0xFF000000) >> 8;
	rgb += (cwrgb.cw_Green & 0xFF000000) >> 16;
	rgb += (cwrgb.cw_Blue & 0xFF000000) >> 24;
	
	SetRPAttrs(rp, RPTAG_FgColor, rgb,
	    	       RPTAG_BgColor, 0xFFFFFF - rgb,
	    	       TAG_DONE);
		       
    	switch(mode)
	{
	    case 0:
		SetDrMd(rp, JAM1);
		Move(rp, x, y);
		Text(rp, "Text JAM1", 4);		
	    	break;
		
	    case 1:
		SetDrMd(rp, JAM2);
		Move(rp, x, y);
		Text(rp, "Text JAM2", 4);		
	    	break;
	    	
	    case 2:
	    	SetDrMd(rp, JAM1);
		RectFill(rp, x, y, x + 30, y + 30);
		break;
		
	    case 3:
	    	SetDrMd(rp, JAM1);
		Move(rp, x, y);
		Draw(rp, x + 30, y);
		Draw(rp, x + 30, y + 30);
    	    	Draw(rp, x, y + 30);
		Draw(rp, x, y);
		break;
		
	    case 4:
	    	SetDrMd(rp, JAM1);
		DrawEllipse(rp, x, y, 30, 30);
		break;
	}
			       
        getevents();
	
	if (Keys[KC_SPACE])
	{
	    Keys[KC_SPACE] = 0;
	    mode = (mode + 1) % 5;
	    hue = 0;
	}
	
	x += dx; if ((x > SCREENWIDTH) || (x < 0)) dx = -dx;
	y += dy; if ((y > SCREENHEIGHT) || (y < 0)) dy = -dy;
	
	hue += 0x1000000;
	
    } /* while(!Keys[KC_ESC]) */
}

/***********************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makewin();
    action();
    cleanup(0);

    return 0; /* keep compiler happy */
}

/***********************************************************************************/

