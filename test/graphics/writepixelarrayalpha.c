/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREENWIDTH  300
#define SCREENHEIGHT 200
#define SCREENCY (SCREENHEIGHT / 2)

/***********************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct Screen *scr;
struct Window *win;
struct RastPort *rp;
struct BitMap *bm;

ULONG cgfx_coltab[256];
UBYTE Keys[128];

/***********************************************************************************/

static void cleanup(char *msg)
{
    if (msg)
    {
        printf("WritePixelArrayAlpha: %s\n",msg);
    }
    
    if (bm) FreeBitMap(bm);
    if (win) CloseWindow(win);
    
    if (scr) UnlockPubScreen(0, scr);
    
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

/***********************************************************************************/

static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
        cleanup("Can't open intuition.library V39!");
    }
    
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
        cleanup("Can't open graphics.library V39!");
    }
    
    if (!(CyberGfxBase = OpenLibrary("cybergraphics.library",0)))
    {
        cleanup("Can't open cybergraphics.library!");
    }
}

/***********************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        cleanup("Can't lock pub screen!");
    }
    
    if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) <= 8)
    {
        cleanup("Need hi or true color screen!");
    }
}

/***********************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(NULL, WA_CustomScreen	, (IPTR)scr, 
    			       WA_InnerWidth	, SCREENWIDTH,
    			       WA_InnerHeight	, SCREENHEIGHT,
			       WA_Title		, (IPTR)"WritePixelArrayAlpha: Move mouse!",
			       WA_DragBar	, TRUE,
			       WA_DepthGadget	, TRUE,
			       WA_CloseGadget	, TRUE,
			       WA_Activate	, TRUE,
			       WA_ReportMouse   , TRUE,
			       WA_MouseQueue	, 1,
			       WA_IDCMP		, IDCMP_CLOSEWINDOW | IDCMP_MOUSEMOVE,
			       WA_BackFill  	, (IPTR)LAYERS_NOBACKFILL,
			       TAG_DONE);
			       
    if (!win) cleanup("Can't open window");

    rp = win->RPort; 

    bm = AllocBitMap(win->GZZWidth, win->GZZHeight * 2, 0, 0, win->RPort->BitMap);
    if (bm)
    {
	struct RastPort temprp;

	InitRastPort(&temprp);
    	temprp.BitMap = bm;
	
	ClipBlit(win->RPort, win->BorderLeft, win->BorderTop, &temprp, 0, 0, win->GZZWidth, win->GZZHeight, 192);
    }
}

/***********************************************************************************/

#define KC_LEFT         0x4F
#define KC_RIGHT     	0x4E
#define KC_UP        	0x4C
#define KC_DOWN      	0x4D
#define KC_ESC       	0x45

/***********************************************************************************/

#if 0
static void getevents(void)
{
    struct IntuiMessage *msg;
    
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        switch(msg->Class)
	{
	    case IDCMP_CLOSEWINDOW:
	        Keys[KC_ESC] = 1;
		break;
		
	    case IDCMP_RAWKEY:
	        {
		    WORD code = msg->Code & ~IECODE_UP_PREFIX;
		    
		    Keys[code] = (code == msg->Code) ? 1 : 0;

		}
	        break;

	}
        ReplyMsg((struct Message *)msg);
    }

}
#endif

/***********************************************************************************/

static void action(void)
{
    static LONG tab[SCREENWIDTH * SCREENHEIGHT];
    LONG x, y;
    LONG ar1, ar2, ar3, ar4;
    LONG ag1, ag2, ag3, ag4;
    LONG ab1, ab2, ab3, ab4;
    LONG r1, r2, r3, r4;
    LONG g1, g2, g3, g4;
    LONG b1, b2, b3, b4;
    LONG tr1, tg1, tb1;
    LONG tr2, tg2, tb2;
    LONG tr3, tg3, tb3;
    LONG tr4, tg4, tb4;
    LONG ttr1, ttg1, ttb1;
    LONG ttr2, ttg2, ttb2;
    LONG tttr, tttg, tttb, ttta;
    
    LONG col;
    BOOL quitme = FALSE;
    
    struct IntuiMessage *msg;
    
    while(!quitme)
    {
	ar1 = 0xFF; ag1 = 0xFF; ab1 = 0xFF; 
	ar2 = 0xFF; ag2 = 0x00; ab2 = 0x00; 
	ar3 = 0x00; ag3 = 0xFF; ab3 = 0x00; 
	ar4 = 0x00; ag4 = 0x00; ab4 = 0xFF; 

	r1 = 0xFF; g1 = 0xFF; b1 = 0xFF; 
	r2 = 0xFF; g2 = 0x00; b2 = 0x00; 
	r3 = 0x00; g3 = 0xFF; b3 = 0x00; 
	r4 = 0x00; g4 = 0x00; b4 = 0xFF; 

        x = scr->MouseX;
	if (x < 0) x = 0; else if (x >= scr->Width) x = scr->Width - 1;

	r1 = ar1 + (ar2 - ar1) * x / (scr->Width - 1);
	g1 = ag1 + (ag2 - ag1) * x / (scr->Width - 1);
	b1 = ab1 + (ab2 - ab1) * x / (scr->Width - 1);
	
	r2 = ar2 + (ar3 - ar2) * x / (scr->Width - 1);
	g2 = ag2 + (ag3 - ag2) * x / (scr->Width - 1);
	b2 = ab2 + (ab3 - ab2) * x / (scr->Width - 1);

	r3 = ar3 + (ar4 - ar3) * x / (scr->Width - 1);
	g3 = ag3 + (ag4 - ag3) * x / (scr->Width - 1);
	b3 = ab3 + (ab4 - ab3) * x / (scr->Width - 1);

	r4 = ar4 + (ar1 - ar4) * x / (scr->Width - 1);
	g4 = ag4 + (ag1 - ag4) * x / (scr->Width - 1);
	b4 = ab4 + (ab1 - ab4) * x / (scr->Width - 1);
	
	ttta = (scr->MouseY * 256) / (scr->Height);
	if (ttta < 0) ttta = 0; else if (ttta > 255) ttta = 255;
	
        for(y = 0; y < SCREENHEIGHT; y ++)
	{
	    for(x = 0; x < SCREENWIDTH; x++)
	    {
	        tr1 = r1 + (r2 - r1) * x / (SCREENWIDTH - 1);
		tg1 = g1 + (g2 - g1) * x / (SCREENWIDTH - 1);
		tb1 = b1 + (b2 - b1) * x / (SCREENWIDTH - 1);
	
		tr2 = r3 + (r4 - r3) * x / (SCREENWIDTH - 1);
		tg2 = g3 + (g4 - g3) * x / (SCREENWIDTH - 1);
		tb2 = b3 + (b4 - b3) * x / (SCREENWIDTH - 1);
		
		tr3 = r1 + (r3 - r1) * y / (SCREENHEIGHT - 1);
		tg3 = g1 + (g3 - g1) * y / (SCREENHEIGHT - 1);
		tb3 = b1 + (b3 - b1) * y / (SCREENHEIGHT - 1);
		
		tr4 = r2 + (r4 - r2) * y / (SCREENHEIGHT - 1);
		tg4 = g2 + (g4 - g2) * y / (SCREENHEIGHT - 1);
		tb4 = b2 + (b4 - b2) * y / (SCREENHEIGHT - 1);
		
		ttr1 = tr1 + (tr2 - tr1) * y / (SCREENHEIGHT - 1);
		ttg1 = tg1 + (tg2 - tg1) * y / (SCREENHEIGHT - 1);
		ttb1 = tb1 + (tb2 - tb1) * y / (SCREENHEIGHT - 1);
		
		ttr2 = tr3 + (tr4 - tr3) * x / (SCREENWIDTH - 1);
		ttg2 = tg3 + (tg4 - tg3) * x / (SCREENWIDTH - 1);
		ttb2 = tb3 + (tb4 - tb3) * x / (SCREENWIDTH - 1);
		
		tttr = (ttr1 + ttr2) / 2;
		tttg = (ttg1 + ttg2) / 2;
		tttb = (ttb1 + ttb2) / 2;

#if AROS_BIG_ENDIAN
		col = (ttta << 24) + (tttr << 16) + (tttg << 8) + tttb;
#else		
		col = (tttb << 24) + (tttg << 16) + (tttr << 8) + ttta;
#endif		
		//kprintf("col[%d,%d] = %08x\n", x,y,col);
	        tab[y * SCREENWIDTH + x] = col;
		
	    } /* for(y = 0; y < SCREENHEIGHT; y ++) */
	    
	} /* for(y = 0; y < SCREENHEIGHT; y ++) */
	
	
	if (bm)
	{
	    struct RastPort temprp;
	    
	    InitRastPort(&temprp);
	    temprp.BitMap = bm;
	    
	    BltBitMap(bm, 0, 0, bm, 0, SCREENHEIGHT, SCREENWIDTH, SCREENHEIGHT, 192, 255, 0);
	    
	    WritePixelArrayAlpha(tab, 0, 0, SCREENWIDTH * sizeof(LONG), 
			    &temprp, 0, SCREENHEIGHT, SCREENWIDTH, SCREENHEIGHT,
			    0);

	    BltBitMapRastPort(bm, 0, SCREENHEIGHT, win->RPort, win->BorderLeft, win->BorderTop, win->GZZWidth, win->GZZHeight, 192);
	}
	else
	{
	    WritePixelArrayAlpha(tab, 0, 0, SCREENWIDTH * sizeof(LONG), 
			    win->RPort, win->BorderLeft, win->BorderTop, SCREENWIDTH, SCREENHEIGHT,
			    0);
	}
		
    	WaitPort(win->UserPort);
	
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		    
	    }
	    ReplyMsg((struct Message *)msg);
	}
    
    }
    
}

/***********************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makewin();
    action();
    cleanup(0);

    return 0; /* keep compiler happy */
}

/***********************************************************************************/

