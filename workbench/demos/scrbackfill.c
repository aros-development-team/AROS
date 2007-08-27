
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
#include <stdlib.h>


#define PATTERNWIDTH  32
#define PATTERNHEIGHT 32
#define PATTERNCOL1 SHADOWPEN
#define PATTERNCOL2 SHINEPEN

struct LayerHookMsg
{
    struct Layer *lay;		/* not valid for layerinfo backfill hook!!! */
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

static struct Hook backfillhook, *old_layerinfohook;
static BOOL hook_installed;


static void Cleanup(char *msg)
{
    struct Window *tempwin;
    
    WORD rc;

    if (msg)
    {
	printf("scrbackfill: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }

    if (win) CloseWindow(win);

    if (hook_installed)
    {
    	InstallLayerInfoHook(&scr->LayerInfo,old_layerinfohook);

	tempwin = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
    				   WA_Left,0,
				   WA_Top,0,
				   WA_Width,scr->Width,
				   WA_Height,scr->Height,
				   WA_Borderless,TRUE,
				   WA_Backdrop,TRUE,
				   WA_BackFill,(IPTR)LAYERS_NOBACKFILL,
				   TAG_DONE);

	if (tempwin) CloseWindow(tempwin);
    }
    
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
    WORD x1,y1,x2,y2,px,py,pw,ph;

    myrp = *rp;

    myrp.Layer = 0;

    x1 = msg->bounds.MinX;
    y1 = msg->bounds.MinY;
    x2 = msg->bounds.MaxX;
    y2 = msg->bounds.MaxY;

    px = x1 % PATTERNWIDTH;

    pw = PATTERNWIDTH - px;

    do
    {
	y1 = msg->bounds.MinY;
	py = y1  % PATTERNHEIGHT;

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


static void StartBackfillHook(void)
{
    struct Window *tempwin;
    
    old_layerinfohook = InstallLayerInfoHook(&scr->LayerInfo,&backfillhook);
    
    hook_installed = TRUE;
    
    /* The non-window area is not automatically backefilled with
       the new hook, which is ok, since this is also true for the
       real Amiga.
       
       So we usa a little trick: We Open a backdrop window with the
       size of the screen and close it immediately. Layers library will then
       automatically backfill in DeleteLayer().*/
      
    tempwin = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
    			       WA_Left,0,
			       WA_Top,0,
			       WA_Width,scr->Width,
			       WA_Height,scr->Height,
			       WA_Borderless,TRUE,
			       WA_Backdrop,TRUE,
			       WA_BackFill,(IPTR)LAYERS_NOBACKFILL,
			       TAG_DONE);

    if (tempwin) CloseWindow(tempwin);
}


static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,10,
				 WA_Top,10,
				 WA_Width,200,
				 WA_Height,dri->dri_Font->tf_YSize + scr->WBorTop + 1,
				 WA_AutoAdjust,TRUE,
				 WA_Title,(IPTR)"Screen Backfill Test",
				 WA_SimpleRefresh,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,TRUE,
				 WA_DragBar,TRUE,
				 WA_IDCMP,IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
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
    StartBackfillHook();
    MakeWin();
    HandleAll();
    Cleanup(0);

    return 0;
}
