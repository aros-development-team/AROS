#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <stdio.h>
#include <stdlib.h>

#define WINWIDTH    	400
#define WINHEIGHT   	400
#define WINCX	    	(WINWIDTH / 2)
#define WINCY	    	(WINHEIGHT / 2)

#define PATTERNWIDTH  32
#define PATTERNHEIGHT 32
#define PATTERNCOL1 SHADOWPEN
#define PATTERNCOL2 SHINEPEN

struct LayerHookMsg
{
    struct Layer *lay;
    struct Rectangle bounds;
    LONG offsetx;
    LONG offsety;
};

struct IntuitionBase 	*IntuitionBase;
struct GfxBase      	*GfxBase;
struct Screen	    	*scr;
struct DrawInfo     	*dri;
struct Window	    	*win, *win2, *win3;
struct RastPort     	*rp;
struct BitMap	    	*patternbm;
struct Hook 	    	backfillhook;

static void cleanup(char *msg)
{
    if(msg) printf("childchild: %s\n", msg);
    
    if (win3) CloseWindow(win3);
    if (win2) CloseWindow(win2);
    if (win) CloseWindow(win);
    
    if (patternbm) FreeBitMap(patternbm);
    
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);
    
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
    	cleanup("Can't open intuition.library!");
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
    	cleanup("Can't open graphics.library!");
    }
   
}

static void getvisual(void)
{
    if (!(scr = LockPubScreen(0))) cleanup("Can't lock pub screen!");
    if (!(dri = GetScreenDrawInfo(scr))) cleanup("Can't get drawinfo!");
}


static void MyBackfillFunc(struct Hook *hook,struct RastPort *rp,
			   struct LayerHookMsg *msg)
{
    struct RastPort myrp;
    struct Layer *lay;
    WORD x1,y1,x2,y2,px,py,pw,ph;

    myrp = *rp;
    lay = msg->lay;

    myrp.Layer = 0;

    x1 = msg->bounds.MinX;
    y1 = msg->bounds.MinY;
    x2 = msg->bounds.MaxX;
    y2 = msg->bounds.MaxY;

    px = (x1 - lay->bounds.MinX) % PATTERNWIDTH;

    pw = PATTERNWIDTH - px;

    do
    {
	y1 = msg->bounds.MinY;
	py = (y1 - lay->bounds.MinY) % PATTERNHEIGHT;

	ph = PATTERNHEIGHT - py;

	if (pw > (x2 - x1 + 1)) pw = x2 - x1 + 1;

	do
	{
	    if (ph > (y2 - y1 + 1)) ph = y2 - y1 + 1;

	    BltBitMap(patternbm,
		      px,
		      py,
		      rp->BitMap,
		      x1,
		      y1,
		      pw,
		      ph,
		      192,
		      255,
		      0);

	    y1 += ph;

	    py = 0;
	    ph = PATTERNHEIGHT;

	} while (y1 <= y2); /* while(y1 < y2) */

	x1 += pw;

	px = 0;
	pw = PATTERNWIDTH;

    } while (x1 <= x2); /* while (x1 < x2) */
}


static void InitBackfillHook(void)
{
    backfillhook.h_Entry = HookEntry;
    backfillhook.h_SubEntry = (HOOKFUNC)MyBackfillFunc;
}


static void MakePattern(void)
{
    struct RastPort *temprp;

    if (!(patternbm = AllocBitMap(PATTERNWIDTH * 2,
				  PATTERNHEIGHT * 2,
				  GetBitMapAttr(scr->RastPort.BitMap,BMA_DEPTH),
				  BMF_CLEAR,
				  scr->RastPort.BitMap)))
    {
	cleanup("Can't create pattern bitmap!");
    }

    if (!(temprp = CreateRastPort()))
    {
	cleanup("Can't create rastport!");
    }	

    temprp->BitMap = patternbm;

    SetAPen(temprp,dri->dri_Pens[PATTERNCOL1]);

    RectFill(temprp,0,0,10,10);

    RectFill(temprp,0,0,PATTERNWIDTH - 1,PATTERNHEIGHT - 1);

    SetAPen(temprp,dri->dri_Pens[PATTERNCOL2]);
    RectFill(temprp,0,
		    0,
		    PATTERNWIDTH / 2 - 1,
		    PATTERNHEIGHT / 2 - 1);

    RectFill(temprp,PATTERNWIDTH / 2,
		    PATTERNHEIGHT / 2,
		    PATTERNWIDTH - 1,
		    PATTERNHEIGHT - 1);

    FreeRastPort(temprp);
}

static void makewin(void)
{
    win = OpenWindowTags(0, WA_PubScreen, (IPTR)scr,
    	    	    	    WA_Left, 20,
    	    	    	    WA_Top, 20,
			    WA_Width, WINWIDTH,
			    WA_Height, WINHEIGHT,
			    WA_Title, (IPTR)"Parent",
			    WA_CloseGadget, TRUE,
			    WA_DragBar, TRUE,
			    WA_DepthGadget, TRUE,
			    WA_IDCMP, IDCMP_CLOSEWINDOW,
			    WA_Activate, TRUE,
			    WA_SimpleRefresh, TRUE,
			     WA_NoCareRefresh,TRUE,
			    WA_BackFill, (IPTR)&backfillhook,
			    WA_SizeGadget, TRUE,
			    WA_MinWidth,20,
			    WA_MinHeight,20,
			    WA_MaxWidth,1000,
			    WA_MaxHeight,1000,
			    TAG_DONE);


    if (!win) cleanup("Can't create parent window!");

    win2 = OpenWindowTags(0, WA_PubScreen, (IPTR)scr,
    	    	    	     WA_Parent, (IPTR)win,
    	    	    	     WA_Left, 20,
    	    	    	     WA_Top, 20,
			     WA_Width, WINWIDTH * 2 / 3,
			     WA_Height, WINHEIGHT * 2 / 3,
			     WA_Title, (IPTR)"Child",
			     WA_DragBar, TRUE,
			     WA_DepthGadget, TRUE,
			     WA_SimpleRefresh, TRUE,
			     WA_NoCareRefresh,TRUE,
			     WA_MinWidth,20,
			     WA_MinHeight,20,
			     WA_MaxWidth,1000,
			     WA_MaxHeight,1000,
			     WA_SizeGadget,TRUE,
			     TAG_DONE);

    
    if (!win2) cleanup("Can't create child window!");

    win3 = OpenWindowTags(0, WA_PubScreen, (IPTR)scr,
    	    	    	     WA_Parent, (IPTR)win2,
    	    	    	     WA_Left, 20,
    	    	    	     WA_Top, 20,
			     WA_Width, WINWIDTH / 2,
			     WA_Height, WINHEIGHT / 2,
			     WA_Title, (IPTR)"Grand Child",
			     WA_DragBar, TRUE,
			     WA_DepthGadget, TRUE,
			     WA_SimpleRefresh, TRUE,
			     WA_NoCareRefresh,TRUE,
			     WA_SizeGadget, TRUE,
			     WA_MinWidth,20,
			     WA_MinHeight,20,
			     WA_MaxWidth,1000,
			     WA_MaxHeight,1000,
			     WA_NoCareRefresh,TRUE,
			     TAG_DONE);

    
    if (!win3) cleanup("Can't create grand child window!");

}

static void handleall(void)
{
    WaitPort(win->UserPort);
}

int main(void)
{
    openlibs();
    getvisual();
    MakePattern();
    InitBackfillHook();
    makewin();
    handleall();
    cleanup(0);

    return 0;
}
