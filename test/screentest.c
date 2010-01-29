/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: gfxtest.c 32486 2010-01-28 09:26:14Z sonic $

    Screen opening test
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

struct myargs
{
    LONG *width;
    LONG *height;
    STRPTR mode;
};

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct DosLibrary *DOSBase;

struct Screen * openscreen(ULONG width, ULONG height, ULONG mode);
struct Window *openwindow(struct Screen *screen, const char *title, LONG x, LONG y, LONG w, LONG h);

ULONG handleevents(struct Window *win, struct Screen *screen, WORD x, WORD y);

#define W1_LEFT		100
#define W1_TOP		100
#define W1_WIDTH	350
#define W1_HEIGHT	200

WORD drawtext(struct Window *win, WORD x, WORD y, char *fmt, ...)
{
    va_list args;
    char buf[256];
    struct IntuiText it = {
        1, 0,
	JAM2,
	0, 0,
	NULL,
	buf,
	NULL
    };
    
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    PrintIText(win->RPort, &it, x, y);
    return y + win->IFont->tf_YSize;
}	

int __nocommandline = 1;

int main(int argc, char **argv)
{
    struct myargs args = {NULL, NULL, NULL};
    struct RDArgs *rda;
    ULONG width = 640;
    ULONG height = 480;
    ULONG mode = INVALID_ID;

    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
        {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
		rda = ReadArgs("WIDTH/N,HEIGHT/N,MODEID", (IPTR *)&args, NULL);
		if (rda) {
		    struct Screen *screen;
		    struct Window *w1;

		    if (args.width)
		        width = *args.width;
		    if (args.height)
		        height = *args.height;
		    if (args.mode)
		        mode = strtoul(args.mode, NULL, 16);
                    if ((screen = openscreen(width, height, mode))) {
			w1 = openwindow(screen, "Screen data",  W1_LEFT, W1_TOP, W1_WIDTH, W1_HEIGHT);
			if (w1) {
			    WORD x = w1->BorderLeft;
		            WORD y = w1->BorderTop;
			    struct BitMap *bitmap = screen->RastPort.BitMap;

			    y = drawtext(w1, x, y, "Requested size: %ux%u", width, height);
			    y = drawtext(w1, x, y, "Requested ModeID: 0x%08lX", mode);
			    y = drawtext(w1, x, y, "Actual size: %ux%u", screen->Width, screen->Height);
			    y = drawtext(w1, x, y, "Actual ModeID: 0x%08lX", screen->ViewPort.ColorMap->VPModeID);
			    y = drawtext(w1, x, y, "BitMap size: %ux%u", GetBitMapAttr(bitmap, BMA_WIDTH), GetBitMapAttr(bitmap, BMA_HEIGHT));
			    handleevents(w1, screen, x, y);
			    CloseWindow(w1);
			}
		        CloseScreen(screen);
		    } else
		        printf("Failed to open screen\n");
		    FreeArgs(rda);
	        } else
		    printf("Error parsing arguments\n");
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
  printf("Opening window, screen=0x%p\n", screen);
  
  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_INTUITICKS,
			  WA_Left,	x,
			  WA_Top,	y,
                          WA_Width, 	w,
                          WA_Height, 	h,
			  WA_CustomScreen, screen,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_DragBar,		TRUE,
			  WA_CloseGadget,	TRUE,
			  WA_NoCareRefresh,	TRUE,
			  WA_NotifyDepth,	TRUE,
			  WA_Title,		title,

                          TAG_END);

  printf("Window opened\n");
  
  return window;
}


struct Screen * openscreen(ULONG width, ULONG height, ULONG mode)
{
  struct Screen * screen;
   
  printf("Opening screen, size: %ux%u\n", width, height);
  screen = OpenScreenTags(NULL,
                          SA_Width, 	width,
                          SA_Height, 	height,
			  SA_Depth,	24,
			  SA_DisplayID,	mode,
			  SA_Title,	"Screen opening and movement test",
                          TAG_END);
  return screen;
}

ULONG handleevents(struct Window *win, struct Screen *screen, WORD x, WORD y)
{
    struct IntuiMessage *imsg;
    WORD y1;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
    ULONG retidcmp = 0;
	
    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    
	    switch (imsg->Class)
	    {
	    case IDCMP_CLOSEWINDOW:
	    	terminated = TRUE;
	    	break;
	    case IDCMP_INTUITICKS:
	        y1 = drawtext(win, x, y, "Screen position: (%u, %u)               ", screen->LeftEdge, screen->TopEdge);
		y1 = drawtext(win, x, y1, "ViewPort size: %ux%u               ", screen->ViewPort.DWidth, screen->ViewPort.DHeight);
		y1 = drawtext(win, x, y1, "ViewPort position: (%u, %u)               ", screen->ViewPort.DxOffset, screen->ViewPort.DyOffset);
		drawtext(win, x, y1, "RasInfo position: (%u, %u)               ", screen->ViewPort.RasInfo->RxOffset, screen->ViewPort.RasInfo->RyOffset);
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



