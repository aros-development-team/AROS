/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <string.h>
#include <setjmp.h>

#define ARG_TEMPLATE "FAST=F/S,NUMBERS=N/S,RECTFILL=RF/S"

#define ARG_FAST     0
#define ARG_NUMBERS  1
#define ARG_RECTFILL 2
#define NUM_ARGS     3

extern struct IntuitionBase *IntuitionBase;

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
    	Printf("damagelist: %s\n",msg);
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

static void Action(void)
{
    struct RastPort *rp;
    struct Region *dr;
    struct RegionRectangle *rr;
    WORD x, y, x1, y1, x2, y2, i, count = 0;
    
    PutStr("Activate the window whose damagelist you want to see.\n");
    PutStr("You have 3 seconds of time!\n\n");
    
    Delay(3*50);

    win = IntuitionBase->ActiveWindow;
    if (!win) Cleanup("No active window!");

    scr = win->WScreen;

    lay = win->WLayer;

    dr = lay->DamageList;
    if (!dr) Cleanup("Layer does not have a damagelist!");
    rr = dr->RegionRectangle;
    if (!rr) Cleanup("Damagelist is empty!");
    
    if (!(rp = CloneRastPort(&win->WScreen->RastPort)))
    {
    	Cleanup("Can´t clone screen rastport!");
    }
    SetDrMd(rp,JAM1);
    
    while(rr)
    {
    	x1 = lay->bounds.MinX + dr->bounds.MinX + rr->bounds.MinX;
	y1 = lay->bounds.MinY + dr->bounds.MinY + rr->bounds.MinY;
	x2 = lay->bounds.MinX + dr->bounds.MinX + rr->bounds.MaxX;
	y2 = lay->bounds.MinY + dr->bounds.MinY + rr->bounds.MaxY;

    	Printf("#%04d (%4d,%4d) - (%4d, %4d)  Size: %4d x %4d\n",
		++count,
		x1,
		y1,
		x2,
		y2,
		x2 - x1 + 1,
		y2 - y1 + 1);

	
	for(i = 0; i < (Args[ARG_FAST] ? 1 : 8);i++)
	{
	    SetAPen(rp,1 + (i & 1));
	    
	    if (Args[ARG_RECTFILL])
	    {
	    	RectFill(rp,x1,y1,x2,y2);
	    } else {
		RectFill(rp,x1,y1,x2,y1);
		RectFill(rp,x2,y1,x2,y2);
		RectFill(rp,x1,y2,x2,y2);
		RectFill(rp,x1,y1,x1,y2);
	    }
	    
	    if (!Args[ARG_FAST]) Delay(10);
	}
	
	if (Args[ARG_NUMBERS])
	{
	    __sprintf(s,"%d",count);
	    i = TextLength(rp,s,strlen(s));
	    
	    x = (x1 + x2 - i) / 2;
	    y = (y1 + y2 - rp->TxHeight) / 2;
	    
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
    	rr = rr->Next;
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
