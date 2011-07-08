
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include <aros/debug.h>

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
struct Hook 	shapehook;

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

static BOOL shapefunc(struct Hook *hook, struct Layer *lay, struct ShapeHookMsg *msg)
{
    struct Region *newshape;
    WORD x2, y2;
    BOOL success = TRUE;    

    switch(msg->Action)
    {
    	case SHAPEHOOKACTION_CREATELAYER:
	case SHAPEHOOKACTION_SIZELAYER:
	case SHAPEHOOKACTION_MOVESIZELAYER:
	    x2 = msg->NewBounds->MaxX - msg->NewBounds->MinX;
	    y2 = msg->NewBounds->MaxY - msg->NewBounds->MinY;
	    
	    if ((newshape = NewRegion()))
	    {
	    	struct Rectangle rect;
		
		rect.MinX = 9;
		rect.MinY = 0;
		rect.MaxX = x2 - 9;
		rect.MaxY = y2;
		success &= OrRectRegion(newshape, &rect);
		
		rect.MinX = 6;
		rect.MinY = 1;
		rect.MaxX = x2 - 6;
		rect.MaxY = y2 - 1;
		success &= OrRectRegion(newshape, &rect);
		
		rect.MinX = 4;
		rect.MinY = 2;
		rect.MaxX = x2 - 4;
		rect.MaxY = y2 - 2;		
		success &= OrRectRegion(newshape, &rect);
		
		rect.MinX = 3;
		rect.MinY = 3;
		rect.MaxX = x2 - 3;
		rect.MaxY = y2 - 3;
		success &= OrRectRegion(newshape, &rect);
		
		rect.MinX = 2;
		rect.MinY = 4;
		rect.MaxX = x2 - 2;
		rect.MaxY = y2 - 4;
		success &= OrRectRegion(newshape, &rect);
		
		rect.MinX = 1;
		rect.MinY = 6;
		rect.MaxX = x2 - 1;
		rect.MaxY = y2 - 6;
		success &= OrRectRegion(newshape, &rect);

		rect.MinX = 0;
		rect.MinY = 9;
		rect.MaxX = x2;
		rect.MaxY = y2 - 9;
		success &= OrRectRegion(newshape, &rect);
		
		if (success)
		{
	    	    if (msg->OldShape) DisposeRegion(msg->OldShape);
	    	    msg->NewShape = shape = newshape;
		}
		else
		{
		    DisposeRegion(newshape);
		}
		
	    } /* if ((newshape = NewRegion())) */
	    
    } /* switch(msg->Action) */
    
    return success;
}

/***********************************************************************************/

static void makewin(void)
{
    shapehook.h_Entry = HookEntry;
    shapehook.h_SubEntry = (HOOKFUNC)shapefunc;
    
    
    win = OpenWindowTags(NULL, WA_CustomScreen	, (IPTR)scr,
    			       WA_InnerWidth	, WINWIDTH,
    			       WA_InnerHeight	, WINHEIGHT,
			       WA_Title		, (IPTR)"Round Edged Window",
			       WA_DragBar	, TRUE,
			       //WA_DepthGadget	, TRUE,
			       //WA_CloseGadget	, TRUE,
			       WA_Activate	, TRUE,
			       WA_MinWidth  	, 160,
			       WA_MinHeight 	, 160,
			       WA_MaxWidth  	, 2000,
			       WA_MaxHeight 	, 2000,
			       WA_SizeGadget	, TRUE,
			       WA_SizeBBottom	, TRUE,
			       WA_NoCareRefresh , TRUE,
			       WA_ShapeHook 	, (IPTR)&shapehook,
			       WA_IDCMP		, IDCMP_CLOSEWINDOW |
			       			  IDCMP_RAWKEY |
						  IDCMP_NEWSIZE,
			       TAG_DONE);

   if (!win) cleanup("Can't open window");

   rp = win->RPort;

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
    action();
    cleanup(0);

    return 0;
}

/***********************************************************************************/

