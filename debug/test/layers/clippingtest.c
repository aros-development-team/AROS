/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <stdio.h>
#include <stdlib.h>

#define ARG_TEMPLATE "SIMPLE/S"

#define ARG_SIMPLE 0
#define NUM_ARGS 1

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;

static struct Window *win;
static struct RastPort *rp;
static struct Layer *lay;

static struct RDArgs *myargs;
static IPTR args[NUM_ARGS];
static char s[256];

static void Cleanup(char *msg)
{
    if (msg)
    {
	printf("clippingtest: %s\n",msg);
    }

    if (win) CloseWindow(win);

    if (myargs) FreeArgs(myargs);

    if (LayersBase) CloseLibrary(LayersBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(0);
}

static void DosError(void)
{
    Fault(IoErr(),0,s,255);
    Cleanup(s);
}

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	Cleanup("Can't open intuition.library V39!");
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
    {
	Cleanup("Can't open graphics.library V39!");
    }

    if (!(LayersBase = OpenLibrary("layers.library",39)))
    {
	Cleanup("Can't open layers.library V39!");
    }
}

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
	DosError();
    }
}

static void MakeWin(void)
{
    win = OpenWindowTags(0,WA_Left,10,
			   WA_Top,20,
			   WA_Width,200,
			   WA_Height,200,
			   WA_Title,(IPTR)"Press key to flip color!",
			   WA_CloseGadget,TRUE,
			   WA_DepthGadget,TRUE,
			   WA_DragBar,TRUE,
			   WA_Activate,TRUE,
			   args[ARG_SIMPLE] ? WA_SimpleRefresh : TAG_IGNORE, TRUE,
			   WA_IDCMP,IDCMP_CLOSEWINDOW |
				    IDCMP_VANILLAKEY |
				    IDCMP_REFRESHWINDOW,
			   TAG_DONE);

    if (!win) Cleanup("Can't open window!");

    rp = win->RPort;
    lay = win->WLayer;
}

static void Action(void)
{
    struct Region *clip, *oldclip;
    struct Rectangle rect1;
    struct Rectangle rect2;
    struct IntuiMessage *msg;
    WORD col = 1;
    BOOL installed = TRUE;
    BOOL quitme = FALSE;

    rect1.MinX =  20;rect1.MinY = 80;
    rect1.MaxX = 180;rect1.MaxY = 120;
    rect2.MinX =  80;rect2.MinY = 20;
    rect2.MaxX = 120;rect2.MaxY = 180;

    Move(rp, 20, 20);
    Draw(rp, 180, 180);

    clip = NewRegion();
    if (!clip) Cleanup("Can't create clip region!");

    OrRectRegion(clip, &rect1);
    OrRectRegion(clip, &rect2);

    oldclip = InstallClipRegion(lay, clip);

    SetAPen(rp,col);
    RectFill(rp,0,0,1000,1000);

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

	    case IDCMP_VANILLAKEY:
	        switch (msg->Code)
	        {
	        case 'c':
	        case 'C':
	        if (installed)
	        {
	            InstallClipRegion(lay, oldclip);
	            installed = FALSE;
	        }
	        else
	        {
	            oldclip = InstallClipRegion(lay, clip);
	            installed = TRUE;
	        }    
		/* Fallthrough */

	        default:
		    col = 3 - col;
		    SetAPen(rp,col);
		    RectFill(rp,0,0,1000,1000);
		    break;
		}

	    case IDCMP_REFRESHWINDOW:
		BeginRefresh(win);
		SetAPen(rp,col);
		RectFill(rp,0,0,1000,1000);					
		EndRefresh(win,TRUE);
		break;
	    }

	    ReplyMsg((struct Message *)msg);
	}
    }

    if (installed)
	InstallClipRegion(lay, oldclip);
    DisposeRegion(clip);
}

int main(void)
{
    OpenLibs();
    GetArguments();
    MakeWin();
    Action();
    Cleanup(0);
    return 0;
}
