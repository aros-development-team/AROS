/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/16 14:03:41  digulla
    More demos

    Revision 1.3  1996/08/15 13:17:32  digulla
    More types of IntuiMessages are checked
    Problem with empty window was due to unhandled REFRESH
    Commented some annoying debug output out

    Revision 1.2  1996/08/13 15:35:44  digulla
    Removed some comments
    Replied IntuiMessage

    Revision 1.1  1996/08/13 13:48:27  digulla
    Small Demo: Open a window, render some gfx and wait for a keypress

    Revision 1.5  1996/08/01 17:40:44  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <clib/dos_protos.h>
#include <clib/aros_protos.h>
#include <clib/utility_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <stdlib.h>

/* Don't define symbols before the entry point. */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern const char dosname[];
static LONG tinymain(void);

__AROS_LH0(LONG,entry,struct ExecBase *,sysbase,,)
{
    __AROS_FUNC_INIT
    LONG error=RETURN_FAIL;

    SysBase=sysbase;
    DOSBase=(struct DosLibrary *)OpenLibrary((STRPTR)dosname,39);
    GfxBase=(struct GfxBase *)OpenLibrary(GRAPHICSNAME,39);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if(DOSBase && GfxBase && IntuitionBase)
    {
	error=tinymain();
	CloseLibrary((struct Library *)DOSBase);
	CloseLibrary((struct Library *)GfxBase);
	CloseLibrary((struct Library *)IntuitionBase);
    }
    return error;
    __AROS_FUNC_EXIT
}

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;
const char dosname[]="dos.library";

void D(char *str)
{
    VPrintf(str,NULL);
    Flush(Output());
}

void Refresh (struct RastPort * rp)
{
    int len;

    SetAPen (rp, 1);
    SetDrMd (rp, JAM2);

    Move (rp, 0, 0);
    Draw (rp, 320, 256);

    Move (rp, 640, 0);
    Draw (rp, 0, 512);

    Move (rp, 300, 40);
    Text (rp, "Hello World.", 12);

    SetAPen (rp, 3);
    RectFill (rp, 90, 0, 120, 30);
    SetAPen (rp, 0);
    RectFill (rp, 100, 10, 110, 20);

    SetAPen (rp, 1);
    RectFill (rp, 150, 10, 160, 20);

    SetAPen (rp, 2);
    RectFill (rp, 200, 10, 210, 20);

    SetAPen (rp, 3);
    RectFill (rp, 250, 10, 260, 20);

    len = TextLength (rp, "Hello World.", 12);

    SetAPen (rp, 2);
    RectFill (rp, 299, 59, 300+len, 60+rp->Font->tf_YSize);

    SetAPen (rp, 1);
    Move (rp, 300, 60 + rp->Font->tf_Baseline);
    Text (rp, "Hello World.", 12);

    SetDrMd (rp, JAM1);
    SetAPen (rp, 1);
    Move (rp, 301, 101);
    Text (rp, "Hello World.", 12);
    SetAPen (rp, 2);
    Move (rp, 300, 100);
    Text (rp, "Hello World.", 12);
}

static void end (void);

static LONG tinymain(void)
{
    struct NewWindow nw;
    struct Window * win;
    struct RastPort * rp;
    struct IntuiMessage * im;
    int cont, draw;
    ULONG args[4];

    args[0] = (ULONG) tinymain;
    args[1] = (ULONG) Refresh;
    args[2] = (ULONG) _entry;
    args[4] = (ULONG) end;
    VPrintf ("main=%08lx\nRefresh=%08lx\nentry=%08lx\nend=%08lx\n", args);

    nw.LeftEdge = 100;
    nw.TopEdge = 100;
    nw.Width = 640;
    nw.Height = 512;
    nw.DetailPen = nw.BlockPen = (UBYTE)-1;
    nw.IDCMPFlags = IDCMP_RAWKEY
		  | IDCMP_REFRESHWINDOW
		  | IDCMP_MOUSEBUTTONS
		  | IDCMP_MOUSEMOVE
		  ;
    nw.Flags = 0L;
    nw.FirstGadget = NULL;
    nw.CheckMark = NULL;
    nw.Title = "Open a window demo";
    nw.Type = WBENCHSCREEN;

    D("OpenWindow\n");
    win = OpenWindow (&nw);

    if (!win)
    {
	D("Couldn't open window\n");
	return 10;
    }

    rp = win->RPort;

    cont = 1;
    draw = 0;

    while (cont)
    {
	if ((im = (struct IntuiMessage *)GetMsg (win->UserPort)))
	{
	    /* D("Got msg\n"); */
	    switch (im->Class)
	    {
	    case IDCMP_RAWKEY:
		cont = 0;
		break;

	    case IDCMP_MOUSEBUTTONS:
		if (im->Code == SELECTDOWN)
		{
		    SetAPen (rp, 2);
		    Move (rp, im->MouseX, im->MouseY);
		    draw = 1;
		}
		else if (im->Code == SELECTUP)
		    draw = 0;

		break;

	    case IDCMP_MOUSEMOVE:
		if (draw)
		    Draw (rp, im->MouseX, im->MouseY);

		break;

	    case IDCMP_REFRESHWINDOW:
		Refresh (rp);
		break;
	    }

	    ReplyMsg ((struct Message *)im);
	}
	else
	{
	    /* D("Waiting\n"); */
	    Wait (1L << win->UserPort->mp_SigBit);
	}
    }

    D("CloseWindow\n");
    CloseWindow (win);

    return 0;
}

static void end (void) {}
