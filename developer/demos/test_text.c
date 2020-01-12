/*
    Copyright © 1998, The AROS Development Team. All rights reserved.
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

#define WIN_WIDTH	400
#define WIN_HEIGHT	300

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;

struct Window *openwindow(STRPTR title, LONG x, LONG y, LONG w, LONG h);

void test_text(struct Window *w);

int handleevents(struct Window *win);

enum { EV_KEY_PRESSED, EV_QUIT };

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
	      struct Window *w1;

	      
		w1 = openwindow("Text speed test. Press key to show JAM1"
			,0, 0
			, WIN_WIDTH, WIN_HEIGHT
		);
		
		if (w1)
		{
		    struct TextAttr ta = { "topaz.font", 8, 0, 0 };
		    struct TextFont *tf;
		    
		    /* Get the default font */
		    
		    
		    tf = OpenFont(&ta);
		    
		    SetFont(w1->RPort, tf);
		    
		    if (handleevents(w1) == EV_QUIT)
		    	goto quit;
			
		    SetDrMd(w1->RPort, JAM1);
		    test_text(w1);
		    
		    SetWindowTitles(w1, "Press any key to test JAM2", NULL);
		    

		    if (handleevents(w1) == EV_QUIT)
		    	goto quit;
			
		    SetDrMd(w1->RPort, JAM2);
		    SetAPen(w1->RPort, DETAILPEN);
		    SetBPen(w1->RPort, BLOCKPEN);
		    test_text(w1);
		    
		    SetWindowTitles(w1, "Press a key to quit", NULL);
		    
		    handleevents(w1);


quit:

		    CloseWindow(w1);
	      }
              CloseLibrary((struct Library *)DOSBase);
	  }
	  CloseLibrary((struct Library *)GfxBase);
	}
	CloseLibrary((struct Library *) IntuitionBase);
    }
    return 0;
} /* main */

#define TEST_TEXT "Blah"

void test_text(struct Window *w)
{
    int len, pixlen, pixleft, iw;
    int x, y, bottom;
    struct TextFont *tf;
    
    tf = w->RPort->Font;
    
    bottom = WIN_HEIGHT - w->BorderBottom - 1 - (tf->tf_YSize - tf->tf_Baseline);
    printf("bottom: %d\n", bottom);
    y = w->BorderTop + tf->tf_Baseline;
    
    iw = w->Width - w->BorderLeft - w->BorderRight;
    
    len = sizeof (TEST_TEXT) - 1;
    
    pixlen = TextLength(w->RPort, TEST_TEXT, len);
    
    for (; y < bottom; y += tf->tf_YSize)
    {
    	pixleft = iw;
	x = w->BorderLeft;
    	do
	{
	    pixleft -= pixlen;
	    Move(w->RPort, x, y);
	    Text(w->RPort, TEST_TEXT, len);
	    x += pixlen;
	}
	while (pixleft > pixlen);
    }
    return;
}				


struct Window *openwindow(STRPTR title, LONG x, LONG y, LONG w, LONG h)
{

  struct Window *window;
  
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY,
			  WA_Left,		x,
			  WA_Top,		y,
                          WA_Width, 		w,
                          WA_Height, 		h,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_CloseGadget,	TRUE,
			  WA_Title,		(IPTR)title,
                          TAG_END
  );

  
  return window;
}

int handleevents(struct Window *win)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL done = FALSE;
    int event = 0;
	
    EnterFunc(bug("HandleEvents(win=%p)\n", win));
    
    while (!done)
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
	    	done = TRUE;
		event = EV_QUIT;
	    	break;
		
	    case IDCMP_RAWKEY:
	    	if (imsg->Code & 0x80)
		{ 
		    	done = TRUE;
		    	event = EV_KEY_PRESSED;
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
	
    ReturnInt("HandleEvents", int, event);
} /* HandleEvents() */


