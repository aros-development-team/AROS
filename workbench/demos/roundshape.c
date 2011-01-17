
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define INSTALL_NULL_SHAPE_FIRST 0

#define WINWIDTH  150
#define WINHEIGHT 150

/***********************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Screen *scr;
struct Window *win;
struct Region *shape;
struct RastPort *rp;

UBYTE Keys[128];

/***********************************************************************************/

static void cleanup(char *msg)
{
    if (msg)
    {
        printf("roundshape: %s\n",msg);
    }

    if (win) CloseWindow(win);
    if (shape) DisposeRegion(shape);    
    if (scr) UnlockPubScreen(0, scr);
    
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
    
}

/***********************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        cleanup("Can't lock pub screen!");
    }    
}

/***********************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(NULL, WA_CustomScreen	, (IPTR)scr,
    			       WA_InnerWidth	, WINWIDTH,
    			       WA_InnerHeight	, WINHEIGHT,
			       WA_Title		, (IPTR)"Round Shape",
			       WA_DragBar	, TRUE,
			       WA_DepthGadget	, TRUE,
			       WA_CloseGadget	, TRUE,
			       WA_Activate	, TRUE,
			       WA_MinWidth  	, 160,
			       WA_MinHeight 	, 160,
			       WA_MaxWidth  	, 2000,
			       WA_MaxHeight 	, 2000,
			       WA_SizeGadget	, TRUE,
			       WA_SizeBBottom	, TRUE,
			       WA_SizeBRight	, TRUE,
			       WA_NoCareRefresh , TRUE,
			       WA_Shape     	, 0,
			       WA_IDCMP		, IDCMP_CLOSEWINDOW |
			       			  IDCMP_RAWKEY |
						  IDCMP_NEWSIZE,
			       TAG_DONE);

   if (!win) cleanup("Can't open window");

   rp = win->RPort;

}

/***********************************************************************************/

static void renderwin(void)
{
    struct Region *oldshape;
    struct Rectangle rect;
    WORD cx, cy, r, y;

    static WORD oldheight = 0;
    static WORD oldwidth  = 0;

    if (oldheight == win->GZZHeight && oldwidth == win->GZZWidth)
        return;

    oldheight = win->GZZHeight;
    oldwidth  = win->GZZWidth;

    cx = win->BorderLeft + win->GZZWidth / 2;
    cy = win->BorderTop + win->GZZHeight / 2;

    r = ((win->GZZWidth < win->GZZHeight) ? win->GZZWidth : win->GZZHeight) / 2 - 10;

#if INSTALL_NULL_SHAPE_FIRST
    oldshape = ChangeWindowShape(win, shape, NULL);
    if (oldshape) DisposeRegion(oldshape);
#endif

    shape = NewRectRegion(0, 0, win->Width - 1, win->BorderTop - 1);

    rect.MinX = win->Width - win->BorderRight;
    rect.MinY = win->Height - win->BorderBottom;
    rect.MaxX = win->Width - 1;
    rect.MaxY = win->Height - 1;

    OrRectRegion(shape, &rect);

    for(y = 0; y < r; y++)
    {
    	WORD dx = (WORD)sqrt((double)(r * r - y * y));

	SetAPen(rp, 3);
    	RectFill(rp, cx - dx, cy - y, cx + dx, cy - y);
    	RectFill(rp, cx - dx, cy + y, cx + dx, cy + y);

	SetAPen(rp, 2);
	WritePixel(rp, cx - dx, cy - y);
	WritePixel(rp, cx - dx, cy + y);

	SetAPen(rp, 1);
	WritePixel(rp, cx + dx, cy - y);
	WritePixel(rp, cx + dx, cy + y);

	rect.MinX = cx - dx;
	rect.MinY = cy - y;
	rect.MaxX = cx + dx;
	rect.MaxY = cy - y;

	OrRectRegion(shape, &rect);

	rect.MinX = cx - dx;
	rect.MinY = cy + y;
	rect.MaxX = cx + dx;
	rect.MaxY = cy + y;

	OrRectRegion(shape, &rect);
    }

    oldshape = ChangeWindowShape(win, shape, NULL);
    if (oldshape) DisposeRegion(oldshape);

    RefreshWindowFrame(win);
}

/***********************************************************************************/

#define KC_ESC 0x45

/***********************************************************************************/

static void action(void)
{
    struct IntuiMessage *msg;

    do
    {
    	WaitPort(win->UserPort);

	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
            switch (msg->Class)
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

    		case IDCMP_NEWSIZE:
	    	    renderwin();
	    	    break;
	    }
            ReplyMsg((struct Message *)msg);
	}

    } while(!Keys[KC_ESC]);
}

/***********************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makewin();
    renderwin();
    action();
    cleanup(0);

    return 0;
}

/***********************************************************************************/

