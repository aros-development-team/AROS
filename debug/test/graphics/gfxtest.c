/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Demo showing gfx hidd
*/

#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#define SDEBUG 1
#define DEBUG 1

#include <aros/debug.h>

#define USE_TWO_WINDOWS

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;

static BOOL quit = FALSE;

struct Screen * openscreen(void);
struct Window *openwindow(struct Screen *screen, const char *title, LONG x, LONG y, LONG w, LONG h);

VOID test_blttemplate( struct Window *w);
VOID test_bltpattern(struct Window *w);
VOID test_bltmask(struct Window *w);
VOID test_flood(struct Window *w);
VOID test_readpixel(struct Window *w);
VOID test_linedrawing(struct Window *w1, struct Window *w2);

ULONG handleevents(struct Window *win, ULONG idcmp);

#define W1_LEFT		100
#define W1_TOP		100
#define W1_WIDTH	200
#define W1_HEIGHT	200

#define W2_LEFT		150
#define W2_TOP		150
#define W2_WIDTH	250
#define W2_HEIGHT	250


int main(int argc, char **argv)
{
    /* Intialize debugging */
    SDInit();
    
    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
        {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
	      struct Screen *screen;
	      struct Window *w1;

	      
              if ((screen = openscreen())) 
              {
		w1 = openwindow(screen, "Window 1",  W1_LEFT, W1_TOP, W1_WIDTH, W1_HEIGHT);
		if (w1)
		{

#ifdef USE_TWO_WINDOWS
		    struct Window *w2;

		    w2 = openwindow(screen, "Window 2", W2_LEFT, W2_TOP, W2_WIDTH, W2_HEIGHT);
		    if (w2)
		    {

#endif	       
			/* Wait forever */
			// test_readpixel(w1);
/*			SetAPen(w1->RPort, 3);
			SetBPen(w1->RPort, 4);
			test_blttemplate(w1);
*/
			test_linedrawing(w1, w2);
/*			handleevents(w1, 0);*/

#ifdef USE_TWO_WINDOWS		
			CloseWindow(w2);
		    }
#endif		    
		    CloseWindow(w1);
		}
		CloseScreen(screen);
	      }
              CloseLibrary((struct Library *)DOSBase);
	  }
	  CloseLibrary((struct Library *)GfxBase);
	}
	CloseLibrary((struct Library *) IntuitionBase);
    }
    return 0;
} /* main */



struct Window *openwindow(struct Screen *screen, const char *title, LONG x, LONG y, LONG w, LONG h)
{

  struct Window *window;
  printf("Opening window, screen=%p\n", screen);
  
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_CHANGEWINDOW,
			  WA_Left,	x,
			  WA_Top,	y,
                          WA_Width, 	w,
                          WA_Height, 	h,
			  WA_CustomScreen, screen,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_CloseGadget,	TRUE,
			  WA_SmartRefresh,	TRUE,
			  WA_NotifyDepth,	TRUE,
			  WA_Title,		title,

                          TAG_END);

  printf("Window opened\n");
  
  return window;
}


struct Screen * openscreen(void)
{
  struct Screen * screen;
  UWORD pens[] = { ~0 };
ULONG patterncoltab[] = { 
    (16L << 16) + 0,	/* 16 colors, loaded at index 0 */
    
    0xB3B3B3B3, 0xB3B3B3B3, 0xB3B3B3B3, /* Grey70	*/
    0x00000000, 0x00000000, 0x00000000, /* Black	*/
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, /* White	*/
    0x66666666, 0x88888888, 0xBBBBBBBB, /* AMIGA Blue   */
    
    0x00000000, 0x00000000, 0xFFFFFFFF, /* Blue		*/
    0x00000000, 0xFFFFFFFF, 0x00000000, /* Green	*/
    0xFFFFFFFF, 0x00000000, 0x00000000, /* Red		*/
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, /* Cyan		*/
    
    0x33333333, 0x33333333, 0x33333333, /* Pattern Col 1 */
    0xcdcdcdcd, 0x6c6c6c6c, 0xc7c7c7c7, /* Pattern Col 2 */
    0x8e8e8e8e, 0x85858585, 0x93939393, /* Pattern Col 3 */
    0x22222222, 0x22222222, 0x22222222, /* Pattern Col 4 */
    
    0x77777777, 0x77777777, 0x77777777, /* Pattern Col 5 */
    0x66666666, 0x66666666, 0x66666666, /* Pattern Col 6 */
    0x55555555, 0x55555555, 0x55555555, /* Pattern Col 7 */
    0x44444444, 0x44444444, 0x44444444, /* Pattern Col 8 */
    
    0L		/* Termination */
};    
   
  printf("Opening screen\n");
  if ((screen = OpenScreenTags(NULL,
                          SA_Width, 	640,
                          SA_Height, 	480,
			  SA_Depth,	24,
			  SA_Title,	"gfx hidd demo",
			  SA_Pens,	pens,
                          TAG_END))) {
  } else {
      screen = OpenScreenTags(NULL,
                          SA_Width, 	640,
                          SA_Height, 	480,
			  SA_Title,	"gfx hidd demo",
			  TAG_END);
  }

  if (screen)
        LoadRGB32(&screen->ViewPort, patterncoltab);

/*  screen->RastPort.longreserved[0] = window->RPort->longreserved[0];

  Draw(&screen->RastPort, 100, 100);
*/
  return screen;
}


VOID test_readpixel(struct Window *w)
{
    ULONG i;
    
    for (i = 0; i < 16; i ++) {
    	UBYTE pen;
    	SetAPen(w->RPort, i);
	WritePixel(w->RPort, 70, 70);
	
	pen = ReadPixel(w->RPort, 70, 70);
	
	printf("Wrote pen %ld, read pen %d\n", (long)i, pen);
    
    }
}

VOID test_flood(struct Window *w)
{

    struct TmpRas tmpras;
    BYTE *buffer;
    
D(bug("Window layer: %p\n", w->WLayer));    

    buffer = AllocRaster(w->WLayer->Width, w->WLayer->Height);
D(bug("buffer: %p\n", buffer));    
    if (!buffer)
    	return;
	
    InitTmpRas(&tmpras, buffer, RASSIZE(w->WLayer->Width, w->WLayer->Height));
    w->RPort->TmpRas = &tmpras;
    
    SetOutlinePen(w->RPort, 1);
    SetAPen(w->RPort, 1);
    
    SetDrPt(w->RPort, ~0L);
    
    Move(w->RPort, 50, 50);
    Draw(w->RPort, 100, 100);
    Draw(w->RPort, 50,  100);
    Draw(w->RPort, 50, 50);
    
D(bug("Calling Flood()\n"));    
    Flood(w->RPort, 0, 70, 80);   /* outline mode */

    w->RPort->TmpRas = NULL;
    
}

#define MASK_WIDTH 32
#define MASK_HEIGHT 6

#define SRC_X 50
#define SRC_Y 50

#define DEST_X 100
#define DEST_Y 50

VOID test_bltmask(struct Window *w)
{
/*    ULONG mask[] = {
	0xAAAAAAAA,
	0xAAAAAAAA,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xAAAAAAAA,
	0xAAAAAAAA
    };
*/    
    /* Fill a area to blit from */
    
    SetAPen(w->RPort, 1);
    
    RectFill(w->RPort, SRC_X, SRC_Y, SRC_X + MASK_WIDTH - 1, SRC_Y + MASK_HEIGHT);
    
    /* Blit from source area */
/*    BltMaskBitMapRastPort(w->RPort
    	, SRC_X, SRC_Y
	, DEST_X, DEST_Y
	, MASK_WIDTH, MASK_HEIGHT
	, 0x00C0
	, (PLANEPTR) mask
   );
*/
				
    return;
}

VOID test_blttemplate(struct Window *w)
{
    UBYTE template[] = { 0xAA, 0xAA, 0xAA, 0xAA
			   , 0xAA, 0xAA, 0xAA, 0xAA };
		
    BltTemplate((PLANEPTR)template
		, 0 /* xsrc */
		, 4 /* modulo */
		, w->RPort
		, 50, 50  /* xdest, ydest */
		, 16 , 2  /* width, height */
    );

    return;
}

VOID test_bltpattern(struct Window *w)
{
    UWORD afpt[] = { 0xAAAA , 0x5555 };


    SetDrMd(w->RPort, JAM2);
    SetAPen(w->RPort, 1);
    SetBPen(w->RPort, 2);
		
		
    SetAfPt(w->RPort, afpt, 1); 
		
    BltPattern(w->RPort, NULL, 50, 50, 100, 100, 0);
    
    return;

}

VOID test_linedrawing(struct Window *w1, struct Window *w2)
{
    struct RastPort *rp;
    struct Window *frontwin, *backwin, *tmp;
    LONG x;
    ULONG innerwidth;
     
    rp = w1->RPort;
    frontwin = w2;
    backwin = w1;
     
    SetAPen(rp, 3);
    SetDrMd(rp, COMPLEMENT);
    
    innerwidth = W1_WIDTH - w1->BorderLeft - w1->BorderRight;
     
    for (x = 0; x < innerwidth && !quit; x ++) {
    	Move(rp, x + w1->BorderLeft, w1->BorderTop);
	Draw(rp, w1->BorderLeft + innerwidth - x  - 1, W1_HEIGHT - w1->BorderBottom - 1);
	
	
	Delay(25);
	WindowToFront(backwin);
	Delay(25);
	/* Wait for IDCMP_CHANGEWINDOW */
kprintf("WAITING FOR TOFRONT ON %s\n", backwin->Title);
	handleevents(backwin, IDCMP_CHANGEWINDOW);
	
	tmp = backwin;
	backwin = frontwin;
	frontwin = tmp;

    	Move(rp, x + w1->BorderLeft, w1->BorderTop);
	Draw(rp, w1->BorderLeft + innerwidth - x  - 1, W1_HEIGHT - w1->BorderBottom - 1);
	
	Delay(25);
	

	WindowToFront(backwin);
	Delay(25);
	/* Wait for IDCMP_CHANGEWINDOW */
kprintf("WAITING FOR TOFRONT ON %s\n", backwin->Title);
	handleevents(backwin, IDCMP_CHANGEWINDOW);
	
	tmp = backwin;
	backwin = frontwin;
	frontwin = tmp;
    }
     
     
}


ULONG handleevents(struct Window *win, ULONG idcmp)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
    ULONG retidcmp = 0;
	
    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    
	    switch (imsg->Class)
	    {
	    case IDCMP_REFRESHWINDOW:
	    	BeginRefresh(win);
	    	EndRefresh(win, TRUE);
	    	break;
	    	
	    case IDCMP_CLOSEWINDOW:
	    	quit = terminated = TRUE;
	    	break;
		
	    default:
	    	if ((idcmp & imsg->Class) == imsg->Class) {
		    retidcmp = imsg->Class;
		    terminated = TRUE;
		}
		break;
		    					
	    } /* switch (imsg->Class) */
	    ReplyMsg((struct Message *)imsg);
	    
	    			
	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */
    
    return retidcmp;
	
} /* HandleEvents() */



