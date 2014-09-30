/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
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
struct Screen *scr;
struct Window *win;
struct RastPort *rp;

UBYTE buf[SCREENWIDTH * SCREENHEIGHT];

/***********************************************************************************/

static void cleanup(char *msg)
{
    if (msg)
    {
        printf("BltTemplateAlpha: %s\n",msg);
    }
    
    if (win) CloseWindow(win);
    
    if (scr) UnlockPubScreen(0, scr);
    
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
			       WA_Title		, (IPTR)"BltTemplateAlpha: Moving mouse is useless!",
			       WA_DragBar	, TRUE,
			       WA_DepthGadget	, TRUE,
			       WA_CloseGadget	, TRUE,
			       WA_Activate	, TRUE,
			       WA_IDCMP		, IDCMP_CLOSEWINDOW,
			       WA_BackFill  	, (IPTR) LAYERS_NOBACKFILL,
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

/***********************************************************************************/

/***********************************************************************************/

static void action(void)
{
    int x, y;
    for(y = 0; y < SCREENHEIGHT; y++)
    {
    	for(x = 0; x < SCREENWIDTH; x++)
	{
	    buf[y * SCREENWIDTH + x] = (y + x) & 255;   
	}	
    }
    
    SetABPenDrMd(rp, 1, 2, JAM1);
    
    BltTemplateAlpha(buf, 0, SCREENWIDTH, rp, win->BorderLeft, win->BorderTop, SCREENWIDTH, SCREENHEIGHT);
    
    
    WaitPort(win->UserPort);   
}

/***********************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makewin();
    action();
    cleanup(0);

    return 0;
}

/***********************************************************************************/

