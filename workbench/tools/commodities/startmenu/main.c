#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include <stdio.h>

#include "main.h"
#include "support.h"
#include "settings.h"
#include "logodata.h"

/************************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define ARG_PRI   0
#define NUM_ARGS  1

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
struct Library *LayersBase;
struct Library *CxBase;

static struct NewBroker nb =
{
   NB_VERSION,
   "StartMenu", 
   "StartMenu 0.9", 
   "Program launch commoditiy", 
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   0,
   NULL,                             
   0 
};

static struct MsgPort *cxport;

static struct RDArgs *myargs;
static struct Screen *scr;
static struct Window *panelwin;
static struct DrawInfo *dri;
static struct TextFont *scrfont;
static struct RastPort *panelrp;

static CxObj *cxbroker, *cxcust;
static UBYTE *logodata;
static ULONG *logopal;
static UWORD *scrpens;
static ULONG cxmask, panelmask;
static WORD scrfontw, scrfonth, logowidth, logoheight, logocols, logomod;
static WORD logobutwidth, logobutheight, panelbutheight, panelwidth, panelheight;
static WORD panelstuffy, logobutposx, logobutposy, logoposx, logoposy;
static WORD scrwidth, scrheight, scrdepth, shinepen, shadowpen, bgpen;
static WORD textpen, hitextpen;
static BOOL quitme, disabled, is_truecolor, panellogodown;

static LONG args[NUM_ARGS];
static char s[256];

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message *cxmsg;
    
    if (msg)
    {
	printf("StartMenu: %s\n",msg);
    }

    if (panelwin) CloseWindow(panelwin);
    
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);
    
    if (cxbroker) DeleteCxObjAll(cxbroker);
    if (cxport)
    {
        while((cxmsg = GetMsg(cxport)))
	{
	    ReplyMsg(cxmsg);
	}
        DeleteMsgPort(cxport);
    }
    
    if (myargs) FreeArgs(myargs);

    if (CxBase) CloseLibrary(CxBase);
    if (LayersBase) CloseLibrary(LayersBase);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(0);
}

/************************************************************************************/

static void DosError(void)
{
    Fault(IoErr(),0,s,255);
    Cleanup(s);
}

/************************************************************************************/

static void Init(void)
{
#if USE_LOGO2
    logopal    = logo2_pal;
    logodata   = logo2_data;
    
    logowidth  = LOGO2_WIDTH;
    logoheight = LOGO2_HEIGHT;
    logocols   = LOGO2_COLORS;
    logomod    = LOGO2_MODULO;
#else
    logopal    = logo1_pal;
    logodata   = logo1_data;
    
    logowidth  = LOGO1_WIDTH;
    logoheight = LOGO1_HEIGHT;
    logocols   = LOGO1_COLORS;
    logomod    = LOGO1_MODULO;
#endif
}

/************************************************************************************/

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

    if (!(CyberGfxBase = OpenLibrary("cybergraphics.library",0)))
    {
	Cleanup("Can't open cybergraphics.library!");
    }

    if (!(LayersBase = OpenLibrary("layers.library",39)))
    {
	Cleanup("Can't open layers.library V39!");
    }

    if (!(CxBase = OpenLibrary("commodities.library",39)))
    {
	Cleanup("Can't open commodities.library V39!");
    }
}

/************************************************************************************/

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
	DosError();
    }
    
    if (args[ARG_PRI]) nb.nb_Pri = *(LONG *)args[ARG_PRI];
}

/************************************************************************************/

static void InitCX(void)
{
    if (!(cxport = CreateMsgPort()))
    {
        Cleanup("Can't create MsgPort!\n");
    }
    
    nb.nb_Port = cxport;
    
    cxmask = 1L << cxport->mp_SigBit;
    
    if (!(cxbroker = CxBroker(&nb, 0)))
    {
        Cleanup("Can't create CxBroker object!\n");
    }
    
    ActivateCxObj(cxbroker, 1);
    
}

/***********************************************************************************/

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        Cleanup("Can't lock pub screen!");
    }
    
    if (!(dri = GetScreenDrawInfo(scr)))
    {
        Cleanup("Can't get screen drawinfo!");
    }

    scrwidth = scr->Width;
    scrheight = scr->Height;
    
    scrdepth = GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH);
    if (scrdepth > 8) is_truecolor = TRUE;
    
    scrfont = dri->dri_Font;
    scrfontw = scrfont->tf_XSize;
    scrfonth = scrfont->tf_YSize;
    
    scrpens = dri->dri_Pens;
    shinepen = scrpens[SHINEPEN];
    shadowpen = scrpens[SHADOWPEN];
    bgpen = scrpens[BACKGROUNDPEN];
    textpen = scrpens[TEXTPEN];
    hitextpen = scrpens[HIGHLIGHTTEXTPEN];
    
}

/***********************************************************************************/

static void RenderPanelLogo(BOOL recessed)
{
    RenderFrame(panelrp,
                dri,
		logobutposx,
		logobutposy,
    		logobutposx + logobutwidth - 1,
		logobutposy + logobutheight - 1,
		recessed); 

    if (is_truecolor)
    {
        FillPixelArray(panelrp,
		       logobutposx + 1,
		       logobutposy + 1,
		       logobutwidth - 2,
		       logobutheight - 2,
		       logopal[0]);
		       
	WriteLUTPixelArray(logodata,
			   0,
			   0,
			   logomod,
			   panelrp,
			   logopal,
			   logoposx + (recessed ? 1 : 0),
			   logoposy + (recessed ? 1 : 0),
			   logowidth,
			   logoheight,
			   CTABFMT_XRGB8);
    }

}

/***********************************************************************************/

static void RenderPanelWin(void)
{
    if (!panelwin) return;
    
    RenderFrame(panelrp, dri, 0, 0, panelwidth - 1, panelheight -1, FALSE);
        
    SetAPen(panelrp, bgpen);
    RectFill(panelrp, 1, 1, panelwidth - 2, panelheight - 2);
    
    RenderPanelLogo(panellogodown);
}

/***********************************************************************************/

static void MakePanelWin(void)
{
    WORD h1, h2;
    
    h1 = scrfonth + BUTTON_EXTRA_HEIGHT;
    h2 = logoheight + LOGO_EXTRA_HEIGHT;
    
    panelwidth = scrwidth;
    
    panelbutheight = (h1 > h2) ? h1 : h2;    
    panelheight = panelbutheight + PANEL_EXTRA_HEIGHT;    
    panelstuffy = (panelheight - panelbutheight) / 2;
    
    logobutwidth = logowidth + LOGO_EXTRA_WIDTH;
    logobutheight = panelbutheight;
    
    logobutposx = LOGO_POS_X;
    logobutposy = panelstuffy;
    
    logoposx = logobutposx + LOGO_EXTRA_WIDTH / 2;
    logoposy = logobutposy + (logobutheight - logoheight) / 2;
    
    panelwin = OpenWindowTags(0, WA_PubScreen, scr,
    				 WA_Left, 0,
				 WA_Top, scrheight - panelheight,
				 WA_Width, scrwidth,
				 WA_Height, panelheight,
				 WA_Borderless, TRUE,
				 WA_RMBTrap, TRUE,
				 WA_SimpleRefresh, TRUE,
				 WA_BackFill, LAYERS_NOBACKFILL,
				 WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_REFRESHWINDOW,
				 TAG_DONE);

    if (!panelwin) Cleanup("Can't open panel window!");
    
    panelmask = 1L << panelwin->UserPort->mp_SigBit;
    panelrp = panelwin->RPort;
    
    SetFont(panelrp, scrfont);
    
    RenderPanelWin();
}

/************************************************************************************/

static void HandleCx(void)
{
    CxMsg *msg;
    
    while((msg = (CxMsg *)GetMsg(cxport)))
    {
       switch(CxMsgType(msg))
       {
	    case CXM_COMMAND:
               switch(CxMsgID(msg))
               {
        	  case CXCMD_DISABLE:
        	     ActivateCxObj(cxbroker,0L);
		     disabled = TRUE;
        	     break;

        	  case CXCMD_ENABLE:
        	     ActivateCxObj(cxbroker,1L);
		     disabled = FALSE;
        	     break;

        	  case CXCMD_KILL:
        	     quitme = TRUE;
        	     break;

               } /* switch(CxMsgID(msg)) */
               break;

       } /* switch (CxMsgType(msg))*/
       
       ReplyMsg((struct Message *)msg);
       
   } /* while((msg = (CxMsg *)GetMsg(cxport))) */
}

/************************************************************************************/

static void HandlePanel(void)
{
    struct IntuiMessage *msg;
    
    while((msg = (struct IntuiMessage *)GetMsg(panelwin->UserPort)))
    {
        switch(msg->Class)
	{
	    case IDCMP_REFRESHWINDOW:
	        BeginRefresh(panelwin);
		RenderPanelWin();
		EndRefresh(panelwin, TRUE);
	        break;
		
	} /* switch(msg->Class) */
	
    } /* while ((msg = (struct IntuiMessage *)GetMsg(panelwin->UserPort))) */
}

/************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;
    
    while(!quitme)
    {
        sigs = Wait(cxmask | panelmask | SIGBREAKF_CTRL_C);
	
	if (sigs & cxmask) HandleCx();
	if (sigs & panelmask) HandlePanel();
	
	if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
	
    } /* while(!quitme) */
    
}

/************************************************************************************/

int main(void)
{
    Init();
    OpenLibs();
    GetArguments();
    InitCX();
    GetVisual();
    MakePanelWin();
    HandleAll();
    Cleanup(0);
    
    return 0;
}


/************************************************************************************/
