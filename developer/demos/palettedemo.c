/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Demo/test for AROS boopsi objects
    Lang: english
*/

#include <aros/config.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <gadgets/arospalette.h>
#include <gadgets/aroscheckbox.h>
#include <intuition/classes.h>
#include <utility/tagitem.h>

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

#define GID_PALETTE	1
#define GID_PROP	2

STATIC VOID HandleEvents(struct Window *);

struct IntuitionBase *IntuitionBase;
struct Library *AROSPaletteBase;

Object *palette, *prop;


/*************
**  main()  **
*************/
int main(int argc, char **argv)
{
    struct Task*inputDevice;
    
    EnterFunc(bug("main()\n"));
    
    /* Initialize the input.device's tc_UserData to 0 */
    inputDevice = FindTask("input.device");
    if (inputDevice)
    {
    	D(bug("Initializing input device's indent count\n"));
    	inputDevice->tc_UserData = NULL;
    }
    
    
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (IntuitionBase)
    {
    	D(bug("Intuiton opened\n"));
 	
    	AROSPaletteBase = OpenLibrary(AROSPALETTENAME, 0);  
    	if (AROSPaletteBase)
    	{
    	    struct Screen *scr;

    	    D(bug("AROSPalette opened\n"));    	

    	    scr = LockPubScreen(NULL);
    	    if (scr)
    	    {
    	    	struct Window *window;

    	    	D(bug("Got screen %p\n", scr));

    	    	window = OpenWindowTags(NULL,
			 WA_PubScreen, (IPTR) scr,
			 WA_Left, 0,
			 WA_Top, 0,
			 WA_Width, 600,
			 WA_Height, 300,
			 WA_MinWidth, 100,
			 WA_MinHeight, 100,
			 WA_MaxWidth, 10000,
			 WA_MaxHeight, 10000,
			 WA_Title, (IPTR) "Try resize to another aspect ratio",
			 WA_IDCMP, 
			 	  IDCMP_GADGETUP 
			 	| IDCMP_MOUSEMOVE
			 	| IDCMP_MOUSEBUTTONS
			 	| IDCMP_REFRESHWINDOW
				| IDCMP_CLOSEWINDOW
				| IDCMP_NEWSIZE
			 	| IDCMP_RAWKEY,
			 WA_SimpleRefresh,	TRUE,
			 WA_DragBar,		TRUE,
			 WA_CloseGadget,	TRUE,
			 WA_SizeGadget,		TRUE,
			 WA_DepthGadget,	TRUE,
			 TAG_DONE);


    	    	if (window)
    	    	{
    		    D(bug("Window opened\n"));

    	    	    palette = NewObject(NULL, AROSPALETTECLASS,
    	    		GA_Left,		window->BorderLeft + 5,
    	    		GA_Top,			window->BorderTop * 2 + 5,
    	    		GA_RelWidth,		-(window->BorderLeft + window->BorderRight + 10),
    	    		GA_RelHeight,		-(window->BorderTop * 2 + window->BorderBottom + 30),
    	    		GA_RelVerify,		TRUE,
    	    		GA_ID,			GID_PALETTE,
    	    		AROSA_Palette_Depth,	3,
    	    		AROSA_Palette_IndicatorWidth,	40,
    	    		GA_Text,		(IPTR) "Palette gadget",
    	    		GA_LabelPlace,		GV_LabelPlace_Above,
    	    		TAG_DONE);
    	    		
		    prop = NewObject(NULL, PROPGCLASS,
		    	GA_RelVerify,	TRUE,
		    	GA_Left,	10,
		    	GA_RelBottom,	-(window->BorderBottom + 22),
    	    		GA_RelWidth,	-(window->BorderLeft + window->BorderRight + 10),
		    	GA_Height,	18,
		    	GA_Previous,	(IPTR) palette,
		    	GA_ID,		GID_PROP,
		    	PGA_Total,	8,
		    	PGA_Visible,	1,
		    	PGA_Top,	2,
		    	PGA_Freedom,	FREEHORIZ,
		    	TAG_DONE);

    	    	    if (palette && prop)
    	    	    {
    	    	    	D(bug("Palette created\n"));
    	    	    	
    	    	    	D(bug("Adding Palette\n"));
    	    	    	AddGList(window, (struct Gadget *)palette, 0, 2, NULL);

    	    	    	D(bug("Refreshing Palette\n"));
    	    	    	RefreshGList((struct Gadget *)palette, window, NULL, 2);
    	    	    	
    	    		HandleEvents(window); 
    	    	
		    }
		    
		    CloseWindow(window);
		    
    	    	    if (prop)
		        DisposeObject(prop);
    	    	    if (palette)
    	    		DisposeObject(palette);

    	    	} /* if (window opened) */

    	    	UnlockPubScreen(NULL, scr);
    	    } /*if (screen locked) */
    	    
    	    CloseLibrary(AROSPaletteBase);
    	}  /* if (paletteclass opened) */
    	
    	CloseLibrary((struct Library *)IntuitionBase);
    } /* if (intuition opened) */

    ReturnInt ("main", int, 0);

}

/*******************
**  HandleEvents  **
*******************/
VOID HandleEvents(struct Window *win)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
	
    EnterFunc(bug("HandleEvents(win=%p)\n", win));
    
    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    
	    switch (imsg->Class)
	    {
		
	    case IDCMP_GADGETUP: {
	    
	    	Object *gad;
	    	
	        D(bug("Received gadgetup"));

	        gad = (Object *)imsg->IAddress;
	        
	        switch (((struct Gadget *)gad)->GadgetID)
	        {
	            case GID_PALETTE:
	            	D(bug("Color selected: %d\n", imsg->Code));
	            	break;
	            	
	            case GID_PROP:
	            	D(bug("Changed to depth %d\n", imsg->Code));
	            	/* Update the palette gadget accordingly */
	            	SetGadgetAttrs((struct Gadget *)palette, win, NULL,
	            		AROSA_Palette_Depth, imsg->Code + 1,
	            		TAG_DONE);
    	    	    	RefreshGList((struct Gadget *)palette, win, NULL, 1);
	            	break;
	        }
	    } break;
		
	    case IDCMP_REFRESHWINDOW:
	    	BeginRefresh(win);
	    	EndRefresh(win, TRUE);
	    	break;
	    	
	    case IDCMP_RAWKEY:
	    case IDCMP_CLOSEWINDOW:
	    	terminated = TRUE;
	    	break;
		    					
	    } /* switch (imsg->Class) */
	    ReplyMsg((struct Message *)imsg);
	    
	    			
	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */
	
    ReturnVoid("HandleEvents");
} /* HandleEvents() */


