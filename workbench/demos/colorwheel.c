#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <gadgets/gradientslider.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/colorwheel.h>

#include <stdio.h>
#include <string.h>

/*****************************************************************************************/

struct IntuitionBase *IntuitionBase;
struct Library *GradientSliderBase;
struct Library *ColorWheelBase;

static struct Screen *scr;
static struct DrawInfo *dri;
static struct Window *win;
static struct Gadget *gradgad, *wheelgad;

/*****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("colorwheel: %s\n", msg);
    
    if (win)
    {
        if (wheelgad) RemoveGadget(win, wheelgad);
    	if (gradgad) RemoveGadget(win, gradgad);
	CloseWindow(win);
    }
    
    if (wheelgad) DisposeObject((Object *)wheelgad);
    if (gradgad) DisposeObject((Object *)gradgad);

    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);
    
    if (ColorWheelBase) CloseLibrary(ColorWheelBase);
    if (GradientSliderBase) CloseLibrary(GradientSliderBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

/*****************************************************************************************/

static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
        cleanup("Can't open intuition.library V39!");
    }
    
    if (!(GradientSliderBase = OpenLibrary("Gadgets/gradientslider.gadget", 0)))
    {
        cleanup("Can't open gradientslider.gadget!");
    }
    
    if (!(ColorWheelBase = OpenLibrary("Gadgets/colorwheel.gadget", 0)))
    {
        cleanup("Can't open colorwheel.gadget!");
    }
}

/*****************************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
        cleanup("Can't lock pub screen!");
    }
       
    if (!(dri = GetScreenDrawInfo(scr)))
    {
        cleanup("Can't get screen drawinfo!");
    }
}

/*****************************************************************************************/

static void makegads(void)
{
    static WORD pens[] = {1, 2, 3, ~0};
    
    gradgad = (struct Gadget *)NewObject(0, "gradientslider.gadget", GA_RelRight	, -30,
								     GA_Top		, 20,
								     GA_Width		, 20,
								     GA_RelHeight	, -40,
								     GRAD_PenArray	, pens,
								     GRAD_KnobPixels	, 10,
								     PGA_Freedom	, LORIENT_VERT,
								     TAG_DONE);
					 
    if (!gradgad) cleanup("Can't create gradientslider gadget!");
    
    wheelgad = (struct Gadget *)NewObject(0, "colorwheel.gadget", GA_Left	, 20,
    								  GA_Top	, 20,
								  GA_RelWidth	, -80,
								  GA_RelHeight	, -40,
								  WHEEL_Screen	, scr,
								  WHEEL_BevelBox, TRUE,
								  GA_Previous	, gradgad,
								  TAG_DONE);
								  
    if (!wheelgad) cleanup("Can't create colorwheel gadget!");
    
}

/*****************************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(0, WA_PubScreen	, scr,
    			    WA_Left		, 10,
			    WA_Top		, 20,
			    WA_Width		, 240,
			    WA_Height		, 200,
			    WA_MinWidth		, 50,
			    WA_MinHeight	, 50,
			    WA_MaxWidth		, 4000,
			    WA_MaxHeight	, 4000,
			    WA_AutoAdjust	, TRUE,
			    WA_Title		, "ColorWheel",
			    WA_CloseGadget	, TRUE,
			    WA_DragBar		, TRUE,
			    WA_DepthGadget	, TRUE,
			    WA_SizeGadget	, TRUE,
			    WA_SizeBBottom	, TRUE,
			    WA_Activate		, TRUE,
			    WA_ReportMouse	, TRUE,
			    WA_IDCMP		, IDCMP_CLOSEWINDOW | IDCMP_MOUSEMOVE,
			    WA_Gadgets		, gradgad,
			    TAG_DONE);
			    
    if (!win) cleanup("Can't open window!");
			    
}

/*****************************************************************************************/

static void handleall(void)
{
    struct IntuiMessage *msg;
    WORD		x;
    ULONG		hue;
    BOOL		quitme = FALSE;
    
    while(!quitme)
    {
        WaitPort(win->UserPort);
	
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	        case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_MOUSEMOVE:
		    x = scr->MouseX;
		    if (x < 0) x = 0; else if (x >= scr->Width) x = scr->Width - 1;
		    hue = x * ((ULONG)0xFFFFFFFF / scr->Width - 1);
		    
		    SetGadgetAttrs(wheelgad, win, 0, WHEEL_Hue, hue, TAG_DONE);
		    break;
		    
	    } /* switch(msg->Class) */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while(!quitme) */
    
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makegads();
    makewin();
    handleall();
    cleanup(0);
    
    return 0;
}

/*****************************************************************************************/
