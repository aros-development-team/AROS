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
static struct Gadget	*gad;
static WORD 	    	winwidth, winheight;

static void cleanup(char *msg)
{
    if (msg) printf("coolbutton: %s\n", msg);
    
    if (win)
    {
    	/* It's always better to remove gadgets before closing window.
	   You *must* do this, if you want to kill (DisposeObject) a
	   gadget, when the window is supposed to stay open, or when
	   the window is killed after the gadget is killed. */
	   
    	if (gad) RemoveGadget(win, gad);
	CloseWindow(win);
    }
    
    if (gad) DisposeObject((Object *)gad);
    
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

static void makegadget(void)
{
    struct RastPort 	    temprp;
    const struct CoolImage  *image = &cool_switchimage; 
    char    	    	    *buttontext = "Hello world";
    WORD    	    	    x, y, w, h, ih;
    
    /* Calc. the size of the button. The coolbuttonclass for now always
       uses the screen font */
       
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font); /* dri_Font is screen font */

    /* gadget width */
    
    w = TextLength(&temprp, buttontext, strlen(buttontext));
    w += image->width;
    /* add some extra width */
    w += 20;
    
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
    
    DeinitRastPort(&temprp);
    
    /* Calc. inner window size */
    
    winwidth  = w + 8;
    winheight = h + 8;
    
    /* Calc. gadget pos */
    
    x = scr->WBorLeft + 4;
    y = scr->WBorTop + scr->Font->ta_YSize + 1 + 4; /* scr->Font is the TextAttr of Screen font. I could also use dri->dri_Font->tf_YSize  */

    /* Create gadget.
    
       GA_Immediate -> want IDCMP_GADGETDOWN msgs.
       GA_RelVerify -> want IDCMP_GADGETUP msgs
    */
       
    gad = (struct Gadget *)NewObject(cool_buttonclass, NULL, GA_Left	     , x    	    	,
    	    	    	    	    	    	    	     GA_Top 	     , y    	    	,
							     GA_Width	     , w    	    	,
							     GA_Height	     , h    	    	,
    	    	    	    	    	    	    	     GA_Text	     , (IPTR)buttontext ,
							     GA_Immediate    , TRUE 	    	,
							     GA_RelVerify    , TRUE 	    	,
    	    	    	    	    	    	    	     COOLBT_CoolImage, (IPTR)image  	,
							     TAG_DONE);

    if (!gad) cleanup("Can't create gadget!");
}

static void makewin(void)
{
    /* Make window. Gadget is directly specified with WA_Gadgets. If
       we wanted to add the gadget after the window is created:
       
       AddGadget(win, gad, -1);
       RefreshGList(gad, win, NULL, 1);
    */
    
    win = OpenWindowTags(NULL, WA_PubScreen 	, (IPTR)scr 	    	,
    	    	    	       WA_Title     	, (IPTR)"Cool Button"     	,
			       WA_InnerWidth	, winwidth  	    	,
			       WA_InnerHeight	, winheight 	    	,
			       WA_CloseGadget	, TRUE	    	    	,
			       WA_DragBar   	, TRUE	    	    	,
			       WA_DepthGadget	, TRUE	    	    	,
			       WA_Activate  	, TRUE	    	    	,
			       WA_IDCMP     	, IDCMP_CLOSEWINDOW |
			            	     	  IDCMP_GADGETUP    |
					      	  IDCMP_GADGETDOWN	,
			       WA_Gadgets   	, (IPTR)gad 	    	,
			       TAG_DONE);
    
    if (!win) cleanup("Error creating window!");    
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

    makegadget();
    makewin();
    handleall();
    cleanup(NULL);

    return 0;
}

