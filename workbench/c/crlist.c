#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>

#define ARG_TEMPLATE "FAST=F/S,NUMBERS=N/S"

#define ARG_FAST    0
#define ARG_NUMBERS 1
#define NUM_ARGS    2

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;

static struct Screen *scr;
static struct Window *win;
static struct Layer *lay;
static struct RDArgs *MyArgs;
static LONG Args[NUM_ARGS];
static char s[256];

static void Cleanup(char *msg)
{
    WORD rc;
    
    if (msg)
    {
    	printf("crlist: %s\n",msg);
	rc = RETURN_WARN;
    } else {
    	rc = RETURN_OK;
    }
    
    if (MyArgs) FreeArgs(MyArgs);
    
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(rc);
}

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0)))
    {
    	Cleanup("Can´t open intuition.library!");
    }
    
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0)))
    {
    	Cleanup("Can´t open graphics.library!");
    }
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
    struct ClipRect *cr;
    WORD x, y, i, count = 0;
    
    puts("Activate the window whose cliprects you want to see.\n");
    puts("You have 3 seconds of time!\n\n");
    
    Delay(3*50);

    win = IntuitionBase->ActiveWindow;
    scr = win->WScreen;
    
    if (!win) Cleanup("No active window!");
    
    if (!(rp = CloneRastPort(&win->WScreen->RastPort)))
    {
    	Cleanup("Can´t clone screen rastport!");
    }
    SetDrMd(rp,JAM1);
    
    lay = win->WLayer;
    
    cr = lay->ClipRect;
    while(cr)
    {
    	printf("#%04d (%4d,%4d) - (%4d, %4d)  Size: %4d x %4d  %s%s\n",
		++count,
		cr->bounds.MinX,
		cr->bounds.MinY,
		cr->bounds.MaxX,
		cr->bounds.MaxY,
		cr->bounds.MaxX - cr->bounds.MinX + 1,
		cr->bounds.MaxY - cr->bounds.MinY + 1,
		(cr->lobs ? "HIDDEN " : ""),
		(cr->BitMap ? "BITMAP ": ""));
		
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
	    sprintf(s,"%d",count);
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
    
    FreeRastPort(rp);
}

void main(void)
{
    OpenLibs();
    GetArguments();
    Action();
    Cleanup(0);
}
