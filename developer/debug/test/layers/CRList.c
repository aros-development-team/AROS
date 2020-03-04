/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/rastport.h>
#include <utility/hooks.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <string.h>
#include <setjmp.h>

#define ARG_TEMPLATE "FAST=F/S,NUMBERS=N/S,EXTRA/S,HOOK/S"

#define ARG_FAST    0
#define ARG_NUMBERS 1
#define ARG_EXTRA   2
#define ARG_HOOK    3
#define NUM_ARGS    4

static struct Screen *scr;
static struct Window *win;
static struct Layer *lay;
static struct RDArgs *MyArgs;
static IPTR Args[NUM_ARGS];
static char s[256];
static jmp_buf exit_buf;

static void Cleanup(char *msg)
{
    WORD rc;

    if (msg)
    {
    	Printf("crlist: %s\n",msg);
	rc = RETURN_WARN;
    } else {
    	rc = RETURN_OK;
    }

    if (MyArgs) FreeArgs(MyArgs);

    if (rc != RETURN_OK)
        longjmp(exit_buf, rc);
}

static void GetArguments(void)
{
    if (!(MyArgs = ReadArgs(ARG_TEMPLATE,Args,0)))
    {
    	Fault(IoErr(),0,s,255);
	Cleanup(s);
    }
}

static void Show(struct RastPort *rp, struct ClipRect *cr)
{
    WORD x, y, i, count = 0;

    while(cr)
    {
    	Printf("#%04ld (%4ld,%4ld) - (%4ld, %4ld)  Size: %4ld x %4ld  %s%s\n",
		++count,
		cr->bounds.MinX,
		cr->bounds.MinY,
		cr->bounds.MaxX,
		cr->bounds.MaxY,
		cr->bounds.MaxX - cr->bounds.MinX + 1,
		cr->bounds.MaxY - cr->bounds.MinY + 1,
		(cr->lobs ? "HIDDEN " : ""),
		(cr->BitMap ? "BITMAP ": ""));

	if (cr->BitMap && Args[ARG_EXTRA])
	{
	    ULONG w = GetBitMapAttr(cr->BitMap, BMA_WIDTH);
	    ULONG h = GetBitMapAttr(cr->BitMap, BMA_HEIGHT);

	    Printf("  -> BitMap 0x%p, size %ld x %ld\n", cr->BitMap, w, h);
	}

	for(i = 0; i < (Args[ARG_FAST] ? 1 : 8);i++)
	{
	    SetAPen(rp,1 + (i & 1));
	    RectFill(rp,cr->bounds.MinX,cr->bounds.MinY,cr->bounds.MaxX,cr->bounds.MinY);
	    RectFill(rp,cr->bounds.MaxX,cr->bounds.MinY,cr->bounds.MaxX,cr->bounds.MaxY);
	    RectFill(rp,cr->bounds.MinX,cr->bounds.MaxY,cr->bounds.MaxX,cr->bounds.MaxY);
	    RectFill(rp,cr->bounds.MinX,cr->bounds.MinY,cr->bounds.MinX,cr->bounds.MaxY);
	    
	    if (!Args[ARG_FAST]) Delay(10);
	}
	
	if (Args[ARG_NUMBERS])
	{
	    __sprintf(s,"%d",count);
	    i = TextLength(rp,s,strlen(s));
	    
	    x = (cr->bounds.MinX + cr->bounds.MaxX - i) / 2;
	    y = (cr->bounds.MinY + cr->bounds.MaxY - rp->TxHeight) / 2;
	    
	    if (x < 0)
	    {
	    	x = 0;
	    } else if (x >= scr->Width - i)
	    {
	    	x = scr->Width - i - 1;
	    }
	  
	    if (y < 0)
	    {
	    	y = 0;
	    } else if (y >= scr->Height - rp->TxHeight)
	    {
	    	y = scr->Height - rp->TxHeight - 1;
	    }
	    
	    i = strlen(s);
	    
	    SetAPen(rp,1);
	    Move(rp,x + 1, y + 1 + rp->TxBaseline);
	    Text(rp,s,i);
	    
	    SetAPen(rp,2);
	    Move(rp,x, y + rp->TxBaseline);
	    Text(rp,s,i);
	}
    	cr = cr->Next;
    }
}

AROS_UFH3(static void, ClipRectHook,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(struct RastPort *, rp, A2),
	  AROS_UFHA(struct BackFillMessage *, msg, A1))
{
    AROS_USERFUNC_INIT

    Printf("# RastPort 0x%p, BitMap 0x%p, Layer 0x%p\n", rp, rp->BitMap, msg->Layer);
    Printf("  -> (%4ld,%4ld) - (%4ld, %4ld), Offset (%4ld, %4ld)\n", msg->Bounds.MinX, msg->Bounds.MinY, msg->Bounds.MaxX, msg->Bounds.MaxY, msg->OffsetX, msg->OffsetY);

    AROS_USERFUNC_EXIT
}

static struct Hook crHook =
{
    .h_Entry = (HOOKFUNC)ClipRectHook
};

static void Action(void)
{
    extern struct IntuitionBase *IntuitionBase;
    struct RastPort *rp;

    PutStr("Activate the window whose cliprects you want to see.\n");
    PutStr("You have 3 seconds of time!\n\n");

    Delay(3*50);

    win = IntuitionBase->ActiveWindow;

    if (!win) Cleanup("No active window!");

    scr = win->WScreen;

    if (!(rp = CloneRastPort(&scr->RastPort)))
    {
    	Cleanup("Can't clone screen rastport!");
    }
    SetDrMd(rp,JAM1);

    lay = win->WLayer;

    Show(rp, lay->ClipRect);
    if (lay->_cliprects)
    {
        PutStr("This window has ClipRegion installed. Listing hidden cliprects...\n");
        Show(rp, lay->_cliprects);
    }

    if (Args[ARG_HOOK])
    {
    	struct Rectangle rect = {20, 20, win->Width - 40 + 1, win->Height - 40 + 1};

        Printf("Running ClipRectHook on Window's RastPort 0x%p, BitMap 0x%p, Layer 0x%p...\n", win->RPort, win->RPort->BitMap, lay);
        DoHookClipRects(&crHook, win->RPort, &rect);
    }

    FreeRastPort(rp);
}

int main(void)
{
    int rc;

    if ((rc = setjmp(exit_buf)) != 0)
        return rc;

    GetArguments();
    Action();
    Cleanup(0);

    return 0;
}
