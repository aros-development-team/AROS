#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

/* Look in this file (compiler/coolimages/include/coolimages.h) to
   see which images are available in coolimages lib. */
   
#include <linklibs/coolimages.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct IntuitionBase 	*IntuitionBase;
struct GfxBase	     	*GfxBase;
struct UtilityBase   	*UtilityBase;
struct Library	     	*CyberGfxBase;

static struct Window 	*win;
static struct Screen 	*scr;
static struct DrawInfo  *dri;
static struct Gadget	*gad[20];

static void cleanup(char *msg)
{
    int i;
    if (msg) printf("coolbutton: %s\n", msg);
    
    if (win)
    {
    	/* It's always better to remove gadgets before closing window.
	   You *must* do this, if you want to kill (DisposeObject) a
	   gadget, when the window is supposed to stay open, or when
	   the window is killed after the gadget is killed. */
	
	for (i = 0; i < 20; i++)
	{
	    if (gad[i]) RemoveGadget(win, gad[i]);
	}
	CloseWindow(win);
    }
    
    for (i = 0; i < 20; i++)
    {
	if (gad[i]) DisposeObject((Object *)gad[i]);
    }
    
    CleanupCoolButtonClass();
    
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
    
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit(0);
}

static void openlibs(void)
{
    /* coolimages.lib needs this 3 libraries to be open, otherwise
       InitCoolButtonClass() will fail and return FALSE */
       
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GfxBase       = (struct GfxBase *)OpenLibrary("graphics.library", 40);
    UtilityBase   = (struct UtilityBase *)OpenLibrary("utility.library", 39);
    
    if (!IntuitionBase || !GfxBase || !UtilityBase) cleanup("Error opening library!");
    
    /* cybergraphics.library is optional, but at the moment coolimages.lib has not yet support
       for rendering on indexed screens. So rendering will for now only work on hicolor
       or truecolor screens. And this requires cybergraphics.library */
       
    CyberGfxBase = OpenLibrary("cybergraphics.library", 39);
}

static void getvisual(void)
{
    scr = LockPubScreen(NULL);
    if (!scr) cleanup("Can't lock pub screen!");
    
    dri = GetScreenDrawInfo(scr);
    if (!dri) cleanup("Can't get screen drawinfo!");
}

static void makegadget(int i, const struct CoolImage  *image, CONST_STRPTR buttontext)
{
    WORD    	    	    x, y, w, h, ih;
    
    /* Calc. the size of the button. The coolbuttonclass for now always
       uses the screen font */

    /* gadget width */
    
    w = 110;
    
    /* gadget height */
    
    h  = dri->dri_Font->tf_YSize;
    ih = image->height;
    
    if (h > ih)
    {
    	/* Font is higher than image */
	
	h += 6;
    }
    else
    {
    	/* Image is higher than font. Add some smaller extra height */
    	h = ih + 4; 
    }
   
    /* Calc. gadget pos */
    
    if (i < 10)
    {
	x = 30;
	y = 28 + (scr->Font->ta_YSize + 10) * i;
    }
    else
    {
	x = 50 + w;
	y = 28 + (scr->Font->ta_YSize + 10) * (i - 10);
    }

    /* Create gadget.
    
       GA_Immediate -> want IDCMP_GADGETDOWN msgs.
       GA_RelVerify -> want IDCMP_GADGETUP msgs
    */
       
    gad[i] = (struct Gadget *)NewObject(cool_buttonclass, NULL, GA_Left	     , x    	    	,
    	    	    	    	    	    	    	     GA_Top 	     , y    	    	,
							     GA_Width	     , w    	    	,
							     GA_Height	     , h    	    	,
							     GA_Text         , (IPTR)buttontext ,
							     GA_Immediate    , TRUE 	    	,
							     GA_RelVerify    , TRUE 	    	,
							     COOLBT_CoolImage, (IPTR)image      ,
							     TAG_DONE);

    if (!gad[i]) cleanup("Can't create gadget!");
}

static void makewin(void)
{
    int i;
    
    win = OpenWindowTags(NULL, WA_PubScreen 	, (IPTR)scr 	    	,
    	    	    	       WA_Title     	, (IPTR)"Cool Button"  	,
			       WA_InnerWidth	, 300	  	    	,
			       WA_InnerHeight	, 250 	    		,
			       WA_CloseGadget	, TRUE	    	    	,
			       WA_DragBar   	, TRUE	    	    	,
			       WA_DepthGadget	, TRUE	    	    	,
			       WA_Activate  	, TRUE	    	    	,
			       WA_IDCMP     	, IDCMP_CLOSEWINDOW |
			            	     	  IDCMP_GADGETUP    |
					      	  IDCMP_GADGETDOWN	,
			       TAG_DONE);
    
    if (!win) cleanup("Error creating window!");    
    
    for (i = 0; i < 20; i++)
    {
	if (gad[i])
	{
	    AddGadget(win, gad[i], -1);
	    RefreshGList(gad[i], win, NULL, 1);
	}
    }
}


static void handleall(void)
{
    struct IntuiMessage *msg;
    BOOL    	    	quitme = FALSE;
    
    while (!quitme)
    {
    	WaitPort(win->UserPort);
	
	/* If you have also gadtools gadgets in your window, you would
	   use GT_GetIMsg/GT_ReplyIMsg */
	   
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (msg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		    
		case IDCMP_GADGETDOWN:
		    printf("Button clicked\n");
		    break;
		    
		case IDCMP_GADGETUP:
		    printf("Button released\n");
		    break;
	    }

	    ReplyMsg((struct Message *)msg);
	}
    }
}

int main(void)
{
    openlibs();
    getvisual();

    if (!InitCoolButtonClass(CyberGfxBase))
    {
	cleanup("Can't init cool button class!");
    }

    makegadget(0, &cool_saveimage, "saveimage");
    makegadget(1, &cool_loadimage, "loadimage");
    makegadget(2, &cool_useimage, "useimage");
    makegadget(3, &cool_cancelimage, "cancelimage");
    makegadget(4, &cool_dotimage, "dotimage");
    makegadget(5, &cool_dotimage2, "dotimage2");
    makegadget(6, &cool_warnimage, "warnimage");
    makegadget(7, &cool_diskimage, "diskimage");
    makegadget(8, &cool_switchimage, "switchimage");
    makegadget(9, &cool_monitorimage, "monitorimage");
    makegadget(10, &cool_mouseimage, "mouseimage");
    makegadget(11, &cool_infoimage, "infoimage");
    makegadget(12, &cool_askimage, "askimage");
    makegadget(13, &cool_keyimage, "keyimage");
    makegadget(14, &cool_clockimage, "clockimage");
    makegadget(15, &cool_flagimage, "flagimage");
    makegadget(16, &cool_headimage, "headimage");
    makegadget(17, &cool_windowimage, "windowimage");
    makegadget(18, &cool_kbdimage, "kbdimage");

    makewin();
    handleall();
    cleanup(NULL);

    return 0;
}

