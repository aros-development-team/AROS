
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>

#include <math.h>
#include <stdio.h>

#define SCREENWIDTH  320
#define SCREENHEIGHT 200
#define SCREENCY (SCREENHEIGHT / 2)

/***********************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct Screen *scr;
struct Window *win;
struct RastPort *rp;

ULONG cgfx_coltab[256];
UBYTE Keys[128];

/***********************************************************************************/

static void cleanup(char *msg)
{
    if (msg)
    {
        printf("cgfxdemo: %s\n",msg);
    }
    
    if (win) CloseWindow(win);
    
    if (scr) UnlockPubScreen(0, scr);
    
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
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
    
    if (!(CyberGfxBase = OpenLibrary("cybergraphics.library",0)))
    {
        cleanup("Can't open cybergraphics.library!");
    }
}

/***********************************************************************************/

static void getvisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        cleanup("Can't lock pub screen!");
    }
    
    if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) <= 8)
    {
        cleanup("Need hi or true color screen!");
    }
}

/***********************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)scr, 
    			       WA_InnerWidth, 320,
    			       WA_InnerHeight, 200,
			       WA_Title, (IPTR)"CGFXDemo",
			       WA_DragBar, TRUE,
			       WA_DepthGadget, TRUE,
			       WA_CloseGadget, TRUE,
			       WA_Activate, TRUE,
			       WA_IDCMP, IDCMP_CLOSEWINDOW |
			       		 IDCMP_RAWKEY,
			       TAG_DONE);
			       
   if (!win) cleanup("Can't open window");

   rp = win->RPort; 
}

/***********************************************************************************/

#define KC_LEFT         0x4F
#define KC_RIGHT     	0x4E
#define KC_UP        	0x4C
#define KC_DOWN      	0x4D
#define KC_ESC       	0x45

/***********************************************************************************/

static void getevents(void)
{
    struct IntuiMessage *msg;
    
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        switch(msg->Class)
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

}

/***********************************************************************************/

static void action(void)
{
    static LONG tab[320 * 200];
    LONG start = 0;
    
    while(!Keys[KC_ESC])
    {
        LONG i, col = start;
	
	for (i = 0; i < 320 * 200;i++)
	{
	     tab[i] = col++;
	}
        
	WritePixelArray(tab, 0, 0, 320, 
			win->RPort, win->BorderLeft, win->BorderTop, 320, 200,
			RECTFMT_RGBA);
	start+=100;
	
        getevents();
    }
}

/***********************************************************************************/

void main(void)
{
    openlibs();
    getvisual();
    makewin();
    action();
    cleanup(0);
}

/***********************************************************************************/

