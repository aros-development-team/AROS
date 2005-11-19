

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/scale.h>
#include <utility/hooks.h>
#include <dos/rdargs.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/dos.h>

#include <aros/asmcall.h>
#include <aros/libcall.h>

#include <stdio.h>
#include <stdlib.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *LayersBase;

static struct Screen *scr;
static struct Window *win;
static struct DrawInfo *dri;
static struct BitMap *scalebitmap;

static BOOL follow = FALSE;
static BOOL do_lens = FALSE;

AROS_UFP3(void, LensCallback,
   AROS_UFPA(struct Hook                  *, hook     , A0),
   AROS_UFPA(struct Layer                 *, l        , A2),
   AROS_UFPA(struct CollectPixelsLayerMsg *, cplm     , A1));
         

struct LensParam
{
	struct Window * win;
	LONG            dx,dy;
};

static void Cleanup(char *msg)
{
    WORD rc;

    if (msg)
    {
	printf("lens: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }

    if (scalebitmap) FreeBitMap(scalebitmap);

    if (win) CloseWindow(win);

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

/*
 * The ScaleLayer callback is doing the actul work of
 * scaling the layer.
 */
AROS_UFH3(void, LensCallback,
     AROS_UFHA(struct Hook                  *, hook     , A0),
     AROS_UFHA(struct Layer                 *, l        , A2),
     AROS_UFHA(struct CollectPixelsLayerMsg *, cplm     , A1))
{
	AROS_USERFUNC_INIT
	struct LensParam * lp = (struct LensParam *)hook->h_Data;
	if (NULL != cplm->bm) {
		
		if (FALSE == do_lens) {
			/*
			 * Does a 1:1 copying
			 */
			BltBitMapRastPort(
				cplm->bm,
				cplm->xSrc,
				cplm->ySrc,
				win->RPort,
				cplm->xDest - win->LeftEdge - lp->dx + win->Width/2,
				cplm->yDest - win->TopEdge  - lp->dy + win->Height/2,
				cplm->width,
				cplm->height,
				cplm->minterm);
		} else {
			/*
			 * This is just a hack for now...
			 * Need to adjust the destination x/y coordinates
			 * in accordance to the source x/y coordinates taken
			 * from the absolute coordinates of the source.
			 */
			struct BitScaleArgs bsa;
			bsa.bsa_SrcX        = cplm->xSrc;
			bsa.bsa_SrcY        = cplm->ySrc;
			bsa.bsa_SrcWidth    = cplm->width;
			bsa.bsa_SrcHeight   = cplm->height;
			bsa.bsa_DestX       = 0;
			bsa.bsa_DestY       = 0;
			bsa.bsa_DestWidth   = cplm->width*2;
			bsa.bsa_DestHeight  = cplm->height*2;
			bsa.bsa_XSrcFactor  = cplm->width;
			bsa.bsa_XDestFactor = cplm->width*2;
			bsa.bsa_YSrcFactor  = cplm->height;
			bsa.bsa_YDestFactor = cplm->height*2;
			bsa.bsa_SrcBitMap   = cplm->bm;
			bsa.bsa_DestBitMap  = scalebitmap;
			BitMapScale(&bsa);

			/*
			 * Display the scaled content.
			 */
			BltBitMapRastPort(
				scalebitmap,
				bsa.bsa_DestX,
				bsa.bsa_DestY,
				win->RPort,
				bsa.bsa_DestX,
				bsa.bsa_DestY,
				cplm->width*2,//bsa.bsa_DestWidth,
				cplm->height*2,//bsa.bsa_DestHeight,
				cplm->minterm);
		}
	}
	AROS_USERFUNC_EXIT
}      


static void DoLens(struct Window * win, LONG dx, LONG dy) 
{
	struct Region * r = NewRegion();
	struct Rectangle rect;
	struct Layer * l = win->RPort->Layer;
	struct Hook h; 
	struct LensParam lp = {win,dx,dy};
	ULONG QWidth  = win->Width  / 4;
	ULONG QHeight = win->Height / 4;
	ULONG HWidth  = win->Width  / 2;
	ULONG HHeight = win->Height / 2;
	h.h_Entry = (IPTR (*)())LensCallback;
	h.h_Data  = (void *)&lp;

	rect.MinX = dx + win->LeftEdge + QWidth  - HWidth;
	rect.MinY = dy + win->TopEdge  + QHeight - HHeight;
	rect.MaxX = dx + win->LeftEdge + win->Width  - 1 - QWidth - HWidth;
	rect.MaxY = dy + win->TopEdge  + win->Height - 1 - QHeight- HHeight;

	if (rect.MinX < 0) 
		rect.MinX = 0;

	if (rect.MinY < 0) 
		rect.MinY = 0;

	OrRectRegion(r, &rect);

	CollectPixelsLayer(l->back,
	                   r,
	                   &h);
	
	DisposeRegion(r);
	
}

static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,10,
				 WA_Top,10,
				 WA_Width,200,
				 WA_Height,100,
				 WA_AutoAdjust,TRUE,
				 WA_SmartRefresh,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,FALSE,
				 WA_DragBar,FALSE,
				 WA_IDCMP,IDCMP_REFRESHWINDOW | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE | IDCMP_CLOSEWINDOW,
				 WA_ReportMouse, TRUE,
				 TAG_DONE)))
    {
	Cleanup("Can't open window!");
    }
    scalebitmap = AllocBitMap(win->Width,
                              win->Height,
                              GetBitMapAttr(win->RPort->BitMap, BMA_DEPTH),
                              BMF_CLEAR,
                              scr->RastPort.BitMap
                             );


    DoLens(win,0,0);

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

		case IDCMP_MOUSEMOVE:
			if (win ==  msg->IDCMPWindow) {
				printf("mouse move %d/%d!\n",
				       msg->MouseX,
				       msg->MouseY);
				if (TRUE == follow) {
					MoveWindow(win,
					           win->MouseX, // - win->LeftEdge,
					           win->MouseY  //- win->TopEdge
					);
					DoLens(win,0,0);
				} else {
					/* 
					 * Keep window still; 
					 * this works, but center is a bit off.
					 */
					DoLens(win,win->MouseX,win->MouseY);
				}
			}
		break;	

		case IDCMP_DELTAMOVE:
			printf("delta move!");
		break;	
	    }

	    ReplyMsg((struct Message *)msg);
	}
    }
}

void ParseArgs(void)
{
	STRPTR args[2] = {(STRPTR)FALSE, (STRPTR)FALSE};
	struct RDArgs * rda;
	rda = ReadArgs("FOLLOW/S,LENS/S",(IPTR *)args, NULL);
	if (TRUE == (ULONG)args[0]) {
		follow = TRUE;
	}
	if (TRUE == (ULONG)args[1]) {
		do_lens = TRUE;
	}
	
	if (rda) {
		FreeArgs(rda);
	}
}


int main(int argc, char **argv)
{
    ParseArgs();
    OpenLibs();
    GetVisual();
    MakeWin();
    HandleAll();
    Cleanup(0);

    return 0;
}
