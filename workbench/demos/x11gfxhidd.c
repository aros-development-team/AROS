/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Demo showing x11gfx.hidd
    Lang: English.
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

struct Screen * openscreen(void);
struct Window *openwindow(struct Screen *screen, LONG x, LONG y, LONG w, LONG h);

VOID test_blttemplate( struct Window *w);
VOID test_bltpattern(struct Window *w);
VOID test_bltmask(struct Window *w);
VOID test_flood(struct Window *w);
VOID test_readpixel(struct Window *w);

VOID handleevents(struct Window *win);

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
		w1 = openwindow(screen, 100, 100, 200, 200);
		if (w1)
		{

#ifdef USE_TWO_WINDOWS
		    struct Window *w2;

		    w2 = openwindow(screen, 150, 150, 250, 250);
		    if (w2)
		    {

#endif	       
			/* Wait forever */
			test_readpixel(w1);
			handleevents(w1);

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



struct Window *openwindow(struct Screen *screen, LONG x, LONG y, LONG w, LONG h)
{

  struct Window *window;
  printf("Opening window, screen=%p\n", screen);
  
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY,
			  WA_Left,	x,
			  WA_Top,	y,
                          WA_Width, 	w,
                          WA_Height, 	h,
			  WA_CustomScreen, screen,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_CloseGadget,	TRUE,
			  
			  

			  WA_Title,		"X11 gfxhidd demo",

                          TAG_END);

  printf("Window opened\n");
  
  return window;
}
struct Screen * openscreen(void)
{
  struct Screen * screen;
   
  printf("Opening screen\n");
  screen = OpenScreenTags(NULL,
                          SA_Width, 	640,
                          SA_Height, 	480,
                          TAG_END);


#if 0
   screen = LockPubScreen(NULL);
#endif

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
	
	printf("Wrote pen %ld, read pen %d\n", i, pen);
    
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


VOID handleevents(struct Window *win)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
	
    EnterFunc(bug("HandleEvents(win=%p)\n", win));
    
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
	    	terminated = TRUE;
	    	break;
		    					
	    } /* switch (imsg->Class) */
	    ReplyMsg((struct Message *)imsg);
	    
	    			
	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */
	
    ReturnVoid("HandleEvents");
} /* HandleEvents() */


