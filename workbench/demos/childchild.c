#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <stdlib.h>

#define WINWIDTH    	400
#define WINHEIGHT   	400
#define WINCX	    	(WINWIDTH / 2)
#define WINCY	    	(WINHEIGHT / 2)

struct IntuitionBase 	*IntuitionBase;
struct GfxBase      	*GfxBase;
struct Screen	    	*scr;
struct DrawInfo     	*dri;
struct Window	    	*win, *win2, *win3;
struct RastPort     	*rp;

static void cleanup(char *msg)
{
    if(msg) printf("childchild: %s\n", msg);
    
    if (win3) CloseWindow(win3);
    if (win2) CloseWindow(win2);
    if (win) CloseWindow(win);
    
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
    makewin();
    handleall();
    cleanup(0);

    return 0;
}
