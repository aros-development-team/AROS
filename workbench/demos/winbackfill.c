
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;

static struct Screen *scr;
static struct Window *win;
static struct DrawInfo *dri;
static struct BitMap *patternbm;

static struct Hook backfillhook;


static void Cleanup(char *msg)
{
    WORD rc;

    if (msg)
    {
	printf("winbackfill: %s\n",msg);
	rc = RETURN_WARN;
    }
    else
    {
	rc = RETURN_OK;
    }

    if (win) CloseWindow(win);

    if (patternbm)
    {
	WaitBlit();
	FreeBitMap(patternbm);
    }

    if (dri) FreeScreenDrawInfo(scr,dri);
    UnlockPubScreen(0,scr);

    if (LayersBase) CloseLibrary(LayersBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(rc);
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


static void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup("Can't lock pub screen!");
    }

    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup("Can't get drawinfo!");
    }
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
	Cleanup("Can't create pattern bitmap!");
    }

    if (!(temprp = CreateRastPort()))
    {
	Cleanup("Can't create rastport!");
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


static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,10,
				 WA_Top,10,
				 WA_Width,300,
				 WA_Height,150,
				 WA_Title,(IPTR)"Window Backfill Test",
				 WA_SimpleRefresh,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,TRUE,
				 WA_DragBar,TRUE,
				 WA_SizeGadget,TRUE,
				 WA_MinWidth,50,
				 WA_MinHeight,50,
				 WA_MaxWidth,scr->Width,
				 WA_MaxHeight,scr->Height,
				 WA_IDCMP,IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
				 WA_BackFill,(IPTR)&backfillhook,
				 TAG_DONE)))
    {
	Cleanup("Can't open window!");
    }

    ScreenToFront(win->WScreen);

}


static void HandleAll(void)
{
    struct IntuiMessage *msg;

    BOOL quitme = FALSE;

    while (!quitme)
    {
	WaitPort(win->UserPort);

	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
			quitme = TRUE;
			break;

		case IDCMP_REFRESHWINDOW:
			BeginRefresh(win);
			EndRefresh(win,TRUE);
			break;
	    }

	    ReplyMsg((struct Message *)msg);
	}
    }
}


int main(void)
{
    OpenLibs();
    InitBackfillHook();
    GetVisual();
    MakePattern();
    MakeWin();
    HandleAll();
    Cleanup(0);

    return 0;
}
