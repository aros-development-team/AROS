/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

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

#include <stdarg.h>

struct myargs
{
    LONG *left;
    LONG *top;
    LONG *width;
    LONG *height;
    LONG *depth;
    STRPTR mode;
    LONG *oscan;
    LONG *scroll;
    LONG *drag;
    LONG *likewb;
};

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct DosLibrary *DOSBase;

struct Window *openwindow(struct Screen *screen, const char *title, LONG x, LONG y, LONG w, LONG h);

ULONG handleevents(struct Window *win, struct Screen *screen, WORD x, WORD y);

#define W1_LEFT		100
#define W1_TOP		100
#define W1_WIDTH	350
#define W1_HEIGHT	210

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
    struct myargs args = {NULL};
    struct RDArgs *rda;

    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
        {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
		rda = ReadArgs("LEFT/K/N,TOP/K/N,WIDTH/N,HEIGHT/N,DEPTH/K/N,MODEID/K,OVERSCAN/K/N,SCROLL/K/N,DRAG/K/N,LIKEWB/K/N", (IPTR *)&args, NULL);
		if (rda) {
		    struct Screen *screen;
		    struct Window *w1;
		    ULONG oserr = 0;
		    struct TagItem tags[] = {
			{SA_Width,     640			         },
			{SA_Height,    480			         },
			{SA_Depth,     4			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{TAG_IGNORE,   0			         },
			{SA_Title,     (IPTR)"Screen opening and movement test"},
			{SA_ErrorCode, (IPTR)&oserr			         },
			{TAG_DONE,     0				 }
		    };

		    if (args.width)
		        tags[0].ti_Data = *args.width;
		    if (args.height)
		        tags[1].ti_Data = *args.height;
		    if (args.depth)
		        tags[2].ti_Data = *args.depth;
		    printf("Opening screen, size: %lux%lu, depth: %lu\n", tags[0].ti_Data, tags[1].ti_Data, tags[3].ti_Data);
		    if (args.mode) {
		        tags[3].ti_Tag  = SA_DisplayID;
			tags[3].ti_Data = strtoul(args.mode, NULL, 16);
			printf("ModeID: 0x%08lX\n", tags[3].ti_Data);
		    }
		    if (args.scroll) {
			tags[4].ti_Tag = SA_AutoScroll;
			tags[4].ti_Data = *args.scroll;
			printf("SA_AutoScroll: %ld\n", tags[4].ti_Data);
		    }
		    if (args.drag) {
			tags[5].ti_Tag = SA_Draggable;
			tags[5].ti_Data = *args.drag;
			printf("SA_Draggable: %ld\n", tags[5].ti_Data);
		    }
		    if (args.likewb) {
			tags[6].ti_Tag = SA_LikeWorkbench;
			tags[6].ti_Data = *args.likewb;
			printf("SA_LikeWorkbench: %ld\n", tags[6].ti_Data);
		    }
		    if (args.oscan) {
			tags[7].ti_Tag = SA_Overscan;
			tags[7].ti_Data = *args.oscan;
			printf("SA_Overscan: %ld\n", tags[7].ti_Data);
		    }
		    if (args.left) {
			tags[8].ti_Tag = SA_Left;
			tags[8].ti_Data = *args.left;
			printf("SA_Left: %ld\n", tags[8].ti_Data);
		    }
		    if (args.top) {
			tags[9].ti_Tag = SA_Top;
			tags[9].ti_Data = *args.top;
			printf("SA_Top: %ld\n", tags[9].ti_Data);
		    }

		    screen = OpenScreenTagList(NULL, tags);
                    if (screen) {
			w1 = openwindow(screen, "Screen data",  W1_LEFT, W1_TOP, W1_WIDTH, W1_HEIGHT);
			if (w1) {
			    WORD x = w1->BorderLeft;
		            WORD y = w1->BorderTop;
			    struct BitMap *bitmap = screen->RastPort.BitMap;

			    y = drawtext(w1, x, y, "Requested size: %lux%lu", tags[0].ti_Data, tags[1].ti_Data);
			    y = drawtext(w1, x, y, "Requested depth: %lu", tags[2].ti_Data);
			    if (args.mode)
			        y = drawtext(w1, x, y, "Requested ModeID: 0x%08lX", tags[3].ti_Data);
			    y = drawtext(w1, x, y, "Actual size: %ux%u", screen->Width, screen->Height);
			    y = drawtext(w1, x, y, "Actual ModeID: 0x%08X", screen->ViewPort.ColorMap->VPModeID);
			    y = drawtext(w1, x, y, "Flags: 0x%04lX", screen->Flags);
			    y = drawtext(w1, x, y, "BitMap size: %ux%u", GetBitMapAttr(bitmap, BMA_WIDTH), GetBitMapAttr(bitmap, BMA_HEIGHT));
			    y = drawtext(w1, x, y, "BitMap depth: %u", GetBitMapAttr(bitmap, BMA_DEPTH));
			    handleevents(w1, screen, x, y);
			    CloseWindow(w1);
			}
		        CloseScreen(screen);
		    } else
		        printf("Failed to open screen, error: %d\n", (int)oserr);
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
                          WA_InnerWidth, 	w,
                          WA_InnerHeight, 	h,
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
	        y1 = drawtext(win, x, y, "Screen position: (%d, %d)               ", screen->LeftEdge, screen->TopEdge);
		y1 = drawtext(win, x, y1, "Mouse position: (%d, %d)               ", screen->MouseX, screen->MouseY);
		y1 = drawtext(win, x, y1, "ViewPort size: %dx%d               ", screen->ViewPort.DWidth, screen->ViewPort.DHeight);
		y1 = drawtext(win, x, y1, "ViewPort position: (%d, %d)               ", screen->ViewPort.DxOffset, screen->ViewPort.DyOffset);
		y1 = drawtext(win, x, y1, "RasInfo position: (%d, %d)               ", screen->ViewPort.RasInfo->RxOffset, screen->ViewPort.RasInfo->RyOffset);
		y1 = drawtext(win, x, y1, "Window position: (%d, %d)               ", win->LeftEdge, win->TopEdge);
		drawtext(win, x, y1, "Window mouse position: (%d, %d)       ",
                    win->MouseX, win->MouseY);
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



