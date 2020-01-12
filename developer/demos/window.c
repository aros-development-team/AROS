/*
    Copyright © 1999, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Demo showing moving and sizing of windows
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

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;

struct IntuiMessage *msg;

struct Window *openwindow(LONG x, LONG y, LONG w, LONG h);


int main(int argc, char **argv)
{
int x, y;

    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
        {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
	      struct Window *w1;

		w1 = openwindow(100, 100, 100, 100);

		if (w1)
		{
		    printf( "MoveWindow()...\n" );
		    for( x=0 ; x<50 ; x++ )
		    {
			MoveWindow(w1,1,0);
//			RefreshWindowFrame(w1);
		    }
		    for( y=0 ; y<50 ; y++ )
		    {
			MoveWindow(w1,0,1);
//			RefreshWindowFrame(w1);
		    }

		    printf( "ChangeWindowBox()...\n" );
		    for( x=0 ; x<50 ; x++ )
		    {
			ChangeWindowBox(w1,150-x,150-x,100+x,100+x);
			RefreshWindowFrame(w1);
		    }

		    printf( "SizeWindow()...\n" );
		    for( y=0 ; y<50 ; y++ )
		    {
			SizeWindow(w1,-1,-1);
			RefreshWindowFrame(w1);
		    }

		    printf( "Done!\nPress a key or click closegadget to quit.\n" );

		    Wait(1L<<w1->UserPort->mp_SigBit);
		    msg = (struct IntuiMessage *)GetMsg(w1->UserPort);
		    /* Catch any message to quit */
		    ReplyMsg((struct Message *)msg);

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



struct Window *openwindow(LONG x, LONG y, LONG w, LONG h)
{

  struct Window *window;
  struct Rectangle R;
  R.MinX = 10;
  R.MinY = 10;
  R.MaxX = 100;
  R.MaxY = 100;
  
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY|IDCMP_CLOSEWINDOW,
			  WA_Left,	x,
			  WA_Top,	y,
                          WA_Height, 	w,
                          WA_Width, 	h,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_Zoom,		(IPTR)&R,
			  WA_CloseGadget,	TRUE,
			  WA_Title,		(IPTR)"Windowing demo",
                          TAG_END);

  printf("Window opened\n");
  
  return window;
}

