/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/locale.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include <stdio.h>
#include <stdlib.h>

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "../strings.h"

#include "main.h"
#include "support.h"
#include "settings.h"
#include "logodata.h"

/************************************************************************************/

UBYTE version[] = "$VER: StartMenu 0.2 (13.10.2001)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K,HIDE/S"

#define ARG_PRI   0
#define ARG_HIDE  1
#define NUM_ARGS  2

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *CyberGfxBase = NULL;
struct Library *LayersBase = NULL;
struct Library *CxBase = NULL;
struct LocaleBase *LocaleBase = NULL;

/* Libraries to open */
struct LibTable
{
 APTR   lT_Library;
 STRPTR lT_Name;
 ULONG  lT_Version;
}
libTable[] =
{
 { &IntuitionBase,	"intuition.library",		39L },
 { &GfxBase,		"graphics.library",		39L },
 { &CyberGfxBase,	"cybergraphics.library",	39L },
 { &LayersBase,		"layers.library",		39L },
 { &CxBase,		"commodities.library",		39L },
 { NULL }
};

static struct NewBroker nb =
{
   NB_VERSION,
   NULL,
   NULL,
   NULL,
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   0,
   NULL,                             
   0 
};

static struct Catalog *catalogPtr;

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
static WORD logobutwidth, logobutheight, panelbutheight, panelwidth, panelheight, visiblepanelheight;
static WORD panelstuffy, logobutposx, logobutposy, logoposx, logoposy;
static WORD scrwidth, scrheight, scrdepth, shinepen, shadowpen, bgpen;
static WORD textpen, hitextpen;
static BOOL quitme, disabled, is_truecolor, panellogodown;

static LONG args[NUM_ARGS];
static char s[256];

static struct Task *maintask;
static ULONG actionbit;
static ULONG actionmask;

static BOOL tohide;
static BOOL hidden;
#define MINIMUM_VISIBLE 3

STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
    STRPTR string;

    if(catalogPtr)
	string = GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
    else
	string = CatCompArray[id].cca_Str;

    return(string);
}

static void HideAction(CxMsg *msg,CxObj *obj)
{
	struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);
	
	if (ie->ie_Class == IECLASS_RAWMOUSE)
	{
		tohide = ie->ie_position.ie_xy.ie_y < (scrheight - visiblepanelheight);
		if (tohide != hidden)
		{
			hidden = tohide;
		 	Signal(maintask, actionmask);
    	}
    }
}

static void HandleHide(void)
{
	if (tohide)
		while (--visiblepanelheight > MINIMUM_VISIBLE)
			MoveWindow(panelwin, 0, 1);
	else
		while (++visiblepanelheight < panelheight)
			MoveWindow(panelwin, 0, -1);
}

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message *cxmsg;
    struct LibTable *tmpLibTable = libTable;
    
    if(msg)
    {
	printf("%s", msg);
    }

    if(IntuitionBase)
    {
	if(panelwin)
	    CloseWindow(panelwin);
    
	if(dri)
	    FreeScreenDrawInfo(scr, dri);

	if(scr)
	    UnlockPubScreen(0, scr);
    }

    if(CxBase)
	if (cxbroker)
	    DeleteCxObjAll(cxbroker);

    if(cxport)
    {
        while((cxmsg = GetMsg(cxport)))
	{
	    ReplyMsg(cxmsg);
	}
        DeleteMsgPort(cxport);
    }
    
    if(myargs)
	FreeArgs(myargs);

    if(LocaleBase)
    {
        CloseCatalog(catalogPtr);
        CloseLibrary((struct Library *)LocaleBase); /* Passing NULL is valid */
        kprintf("Closed locale.library!\n");
    }
    
    while(tmpLibTable->lT_Name) /* Check for name rather than pointer */
    {
	if((*(struct Library **)tmpLibTable->lT_Library))
	{
	    CloseLibrary((*(struct Library **)tmpLibTable->lT_Library));
	    kprintf("Closed %s!\n", tmpLibTable->lT_Name);
	}

	tmpLibTable++;
    }

    if(actionbit)
	FreeSignal(actionbit);

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
    struct LibTable *tmpLibTable = libTable;
    UBYTE tmpString[128]; /* petah: What if library name plus error message exceeds 128 bytes? */

    if((LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 40)))
    {
	catalogPtr = OpenCatalog(NULL, "Sys/Commodities.catalog", OC_BuiltInLanguage, "english", TAG_DONE);
    }
    else
	kprintf("Warning: Can't open locale.library V40!\n");

    while(tmpLibTable->lT_Library)
    {
	if(!((*(struct Library **)tmpLibTable->lT_Library = OpenLibrary(tmpLibTable->lT_Name, tmpLibTable->lT_Version))))
        {
	    sprintf(tmpString, getCatalog(catalogPtr, MSG_CANT_OPEN_LIB), tmpLibTable->lT_Name, tmpLibTable->lT_Version);
	    Cleanup(tmpString);
	}
        else
	    kprintf("Library %s opened!\n", tmpLibTable->lT_Name);

	tmpLibTable++;
    }
}

/************************************************************************************/

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
	DosError();
    }
    
    if (args[ARG_PRI])  nb.nb_Pri = *(LONG *)args[ARG_PRI];
}

/************************************************************************************/

static void InitCX(void)
{
    if (!(cxport = CreateMsgPort()))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_MSGPORT));
    }
    
    nb.nb_Port = cxport;
    
    cxmask = 1L << cxport->mp_SigBit;
    
    if (!(cxbroker = CxBroker(&nb, 0)))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_BROKER));
    }
    
	if (args[ARG_HIDE])
	{
		if (!(cxcust = CxCustom(HideAction, 0)))
    	{
        	Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_CUSTOM));
    	}

    	AttachCxObj(cxbroker, cxcust);
	}
	ActivateCxObj(cxbroker, 1);
    
}

/***********************************************************************************/

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(NULL)))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_LOCK_SCR));
    }
    
    if (!(dri = GetScreenDrawInfo(scr)))
    {
        Cleanup("Can't get screen drawinfo! (This shouldn't happen?!)\n");
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
				 WA_SimpleRefresh, FALSE,
				 WA_BackFill, LAYERS_NOBACKFILL,
				 WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_REFRESHWINDOW,
				 TAG_DONE);
     
    if(!panelwin)
	Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_WIN));

    visiblepanelheight = panelheight;

    if(args[ARG_HIDE] && (panelwin->WLayer->LayerInfo->Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
    {
	maintask   = FindTask(0);
	actionbit  = AllocSignal(-1);
	actionmask = 1 << actionbit;
		 
	tohide = TRUE;
	hidden = TRUE;
	HandleHide();
    }
    else
	args[ARG_HIDE] = actionbit = actionmask = 0;

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
        sigs = Wait(cxmask | panelmask | actionmask | SIGBREAKF_CTRL_C);
	
	if (sigs & actionmask) HandleHide();
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

    nb.nb_Name = getCatalog(catalogPtr, MSG_STARTMENU_CXNAME);
    nb.nb_Title = getCatalog(catalogPtr, MSG_STARTMENU_CXTITLE);
    nb.nb_Descr = getCatalog(catalogPtr, MSG_STARTMENU_CXDESCR);

    GetArguments();
    GetVisual();
    MakePanelWin();
    InitCX();
    HandleAll();
    Cleanup(0);
    
    return 0;
}


/************************************************************************************/
