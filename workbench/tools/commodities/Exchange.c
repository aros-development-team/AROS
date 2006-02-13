/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Exchange -- controls commodities.
 */

/******************************************************************************

    NAME

        Exchange

    SYNOPSIS

        CX_PRIORITY/K/N,CX_POPUP/K/S,CX_POPKEY/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Manages the commodities in the system

    INPUTS

        CX_PRIORITY  --  Priority of the Exchange broker
	CX_POPUP     --  Appear at startup
	CX_POPKEY    --  Hotkey combination for Exchange

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/


#include <cxintern.h>   /* Exchange is the ONLY commodity that may use this */
/* These are necessary as cxintern.h defines them for commodities.library
   purposes. */
#ifdef SysBase
#undef SysBase
#endif
#ifdef KeyMapBase
#undef KeyMapBase
#endif
#ifdef UtilityBase
#undef UtilityBase
#endif
#ifdef TimerBase
#undef TimerBase
#endif

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/libraries.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/locale.h>
#include <intuition/intuition.h>
#include <devices/rawkeycodes.h>

#include <string.h>

#include <stdio.h>

#define DEBUG 0
#include <aros/debug.h>

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "strings.h"

#define  P(x)   

#define BORDERY      4
#define SPACEY       4
#define EXTRAHEIGHT  6
#define LABELSPACEY  4

UBYTE version[] = "$VER: Exchange 0.3 (14.10.2001)";

static struct Gadget *lvgad;
static WORD lvtop, lvheight;

struct ExchangeState
{
    CxObj          *ec_broker;
    struct MsgPort *ec_msgPort;	     /* Message port for our broker */
    struct Catalog *ec_catalog;	     /* Commodities locale catalog */
    struct List     ec_brokerList;   /* Current list of brokers
					(struct BrokerCopy nodes */

    struct Screen  *ec_screen;
    struct VisualInfo *ec_visualInfo; /* Visualinfo for the gadgets */
    
    struct Window  *ec_window;	     /* The Exchange window */

    /* The gadgets */
    struct Gadget  *ec_context;
    struct Gadget  *ec_listView;
    struct Gadget  *ec_showBut;
    struct Gadget  *ec_hideBut;
    struct Gadget  *ec_killBut;
    struct Gadget  *ec_cycle;
    struct Gadget  *ec_textGad;
    struct Gadget  *ec_textGad2;

    struct Menu    *ec_menus;
    UBYTE   	    ec_lvitemname[CBD_NAMELEN + 1];

    /* Values settable via tooltypes/command arguments */
    BOOL       ec_cxPopup;
    STRPTR     ec_cxPopKey;	/* Hotkey for Exchange */
    LONG       ec_cxPri;	/* Priority of the Exchange broker */
};

/* Libraries to open */
struct LibTable
{
    APTR	lT_Library;
    STRPTR	lT_Name;
    ULONG	lT_Version;
}
libTable[] =
{
    { &IntuitionBase,	"intuition.library",	40L},
    { &GadToolsBase,	"gadtools.library",	40L},
    { &UtilityBase,	"utility.library",	40L},
    { &CxBase,		"commodities.library",	40L},
    { NULL }
};

/* Prototypes */
struct BrokerCopy *getNth(struct List *list, LONG n);

void    redrawList(struct ExchangeState *ec);
void    appearExchange(struct ExchangeState *ec);
void    setText(struct Gadget *gadget, STRPTR text, struct ExchangeState *ec);
void    setGadgetState(struct Gadget *gadget, BOOL status,
		       struct ExchangeState *ec);
void    updateInfo(struct ExchangeState *ec);
void    informBroker(LONG command, struct ExchangeState *ec);
void    switchActive(struct ExchangeState *ec);
void    realMain(struct ExchangeState *ec);
void    freeResources(struct ExchangeState *ec);
BOOL    initGadgets(struct ExchangeState *ec, struct Screen *scr, LONG fontHeight);
BOOL    readShellArgs(struct ExchangeState *ec);
BOOL    readWBArgs(int argc, char **argv, struct ExchangeState *ec);
BOOL    getResources(struct ExchangeState *es);
CONST_STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id);
void    setupMenus(struct Catalog *catalogPtr);
void    showSimpleMessage(struct ExchangeState *ec, CONST_STRPTR msgString);

int main(int argc, char **argv)
{
    struct ExchangeState ec;
    int    retval = RETURN_OK;

    if (getResources(&ec))
    {
	if (Cli() == NULL)
	{
	    /* We were called from Workbench */
	    if (readWBArgs(argc, argv, &ec))
	    {
		realMain(&ec);
	    }
	}
	else
	{
	    /* We were called from a Shell */
	    if (readShellArgs(&ec))
	    {
		realMain(&ec);
	    }
	    else
	    {
		PrintFault(IoErr(), "Exchange");
		retval = RETURN_FAIL;
	    }
	}
    }	
    else
    {
	retval = RETURN_FAIL;
    }
    
    freeResources(&ec);
    
    return retval;
}


struct LocaleBase *LocaleBase       = NULL;
struct Library *GadToolsBase        = NULL;
struct Library *CxBase              = NULL;
struct Library *IconBase            = NULL;
struct UtilityBase *UtilityBase     = NULL;
struct IntuitionBase *IntuitionBase = NULL;


/* Description of the Exchange broker */
struct NewBroker exBroker =
{
    NB_VERSION,
    NULL,			/* The next three fields are set by getCatalog() */
    NULL,
    NULL,
    NBU_UNIQUE | NBU_NOTIFY,	/* One Exchange is enough */
    COF_SHOW_HIDE,		/* nb_Flags */
    0,				/* nb_Pri -- should be set by tooltypes */
    NULL,	                /* ec->ec_msgPort */
    0				/* nb_ReservedChannel */
};


BOOL readShellArgs(struct ExchangeState *ec)
{	
    IPTR          *args[] = { NULL, NULL, NULL };
    struct RDArgs *rda;
    
    rda = ReadArgs("CX_PRIORITY/K/N,CX_POPUP/K/S,CX_POPKEY/K", (IPTR *)args,
		   NULL);

    if (rda == NULL)
    {
	return FALSE;
    }

    if (args[0] != NULL)
    {
	ec->ec_cxPri = (LONG)*args[0];
    }

    if (args[1] != NULL)
    {
	ec->ec_cxPopup = (BOOL)*args[1];
    }

    if (args[2] != NULL)
    {
	ec->ec_cxPopKey = (STRPTR)*args[2];
    }
    else
    {
	ec->ec_cxPopKey = "ctrl alt h";
    }

    FreeArgs(rda);

    return TRUE;
}


BOOL readWBArgs(int argc, char **argv, struct ExchangeState *ec)
{
    STRPTR    popUp;
    UBYTE   **tt, tmpString[128];

    IconBase = OpenLibrary("icon.library", 40);
    
    if (IconBase == NULL)
    {
	sprintf(tmpString, getCatalog(ec->ec_catalog, MSG_CANT_OPEN_LIB),
		"icon.library", 40L);
        showSimpleMessage(ec, tmpString);

	return FALSE;
    }
    
    tt = ArgArrayInit(argc, (UBYTE **)argv);
    
    ec->ec_cxPri    = ArgInt(tt, "CX_PRIORITY", 0);
    ec->ec_cxPopKey = ArgString(tt, "CX_POPKEY", "ctrl alt h");
    popUp = ArgString(tt, "CX_POPUP", "YES");

    if (Stricmp(popUp, "YES") == 0)
    {
	ec->ec_cxPopup = TRUE;
    }

    ArgArrayDone();
    CloseLibrary(IconBase);

    return TRUE;
}


static struct NewMenu nm[] =
{
    { NM_TITLE,	(STRPTR)MSG_MEN_PROJECT },
    { NM_ITEM,	(STRPTR)MSG_MEN_PROJECT_QUIT },
    { NM_END } /* petah: Should we use a trailing comma here? Look it up! */
};


BOOL getResources(struct ExchangeState *ec)
{
    struct DrawInfo  *drawInfo;
    LONG              topOffset;
    LONG    	      fontHeight;
    LONG    	      winHeight;
    struct LibTable  *tmpLibTable = libTable;
    UBYTE	      tmpString[256];

    memset(ec, 0, sizeof(struct ExchangeState));

    /* First, open necessary libraries - start with locale (for localized error
       messages thruout the program initialisation */

    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 40);

    if (LocaleBase != NULL)
    {
	/* TODO: OC_BuiltInLanguage should be NULL, but AROS locale doesn't support
	   it yet */
	ec->ec_catalog = OpenCatalog(NULL, "System/Tools/Commodities.catalog",
				     OC_BuiltInLanguage, (Tag)"english", TAG_DONE);

	if (ec->ec_catalog == NULL)
	{
	    D(bug("OpenCatalog() failed!\n"));
	}
    }
    else
    {
	D(bug("Warning: Can't open locale.library V40!\n"));
    }

    while (tmpLibTable->lT_Library)
    {
	*(struct Library **)tmpLibTable->lT_Library = 
	    OpenLibrary(tmpLibTable->lT_Name, tmpLibTable->lT_Version);

	if (tmpLibTable->lT_Library == NULL)
	{
	    sprintf(tmpString, getCatalog(ec->ec_catalog, MSG_CANT_OPEN_LIB),
		    tmpLibTable->lT_Name, tmpLibTable->lT_Version);
	    showSimpleMessage(ec, tmpString);

	    return FALSE;
	}
	else
	{
	    D(bug("Library %s opened!\n", tmpLibTable->lT_Name));
	}

	tmpLibTable++;
    }    

    D(bug("Libraries opened!\n"));
 
    /* Then, allocate the rest of the resources */
    
    ec->ec_msgPort = CreateMsgPort();
    
    if (ec->ec_msgPort == NULL)
    {
	return FALSE;
    }

    // ec->ec_hoykeyPort = CreateMsgPort();
    
    // if(ec->ec_hotkeyPort == NULL)
    //     return FALSE;

    
    NEWLIST(&ec->ec_brokerList);

    exBroker.nb_Name = getCatalog(ec->ec_catalog, MSG_EXCHANGE_CXNAME);
    exBroker.nb_Title = getCatalog(ec->ec_catalog, MSG_EXCHANGE_CXTITLE);
    exBroker.nb_Descr = getCatalog(ec->ec_catalog, MSG_EXCHANGE_CXDESCR);

    exBroker.nb_Port = ec->ec_msgPort;
    ec->ec_broker = CxBroker(&exBroker, NULL);

    if (ec->ec_broker == NULL)
    {
	return FALSE;
    }
    
    D(bug("Broker: %s Flags: %i\n", ec->ec_broker->co_Ext.co_BExt->bext_Name,
	      ec->ec_broker->co_Flags));


    // AttachCxObj(ec->ec_broker, Hotkey("ctrl alt help", ec->ec_hotkeyPort));

    if (CxObjError(ec->ec_broker) != 0)
    {
	return FALSE;
    }

    /* Start our broker */
    ActivateCxObj(ec->ec_broker, TRUE);

    ec->ec_screen = LockPubScreen(NULL);

    if (ec->ec_screen == NULL)
    {
	showSimpleMessage(ec, getCatalog(ec->ec_catalog, MSG_CANT_LOCK_SCR));

	return FALSE;
    }

    ec->ec_visualInfo = GetVisualInfoA(ec->ec_screen, NULL);

    if (ec->ec_visualInfo == NULL)
    {
	showSimpleMessage(ec, getCatalog(ec->ec_catalog, MSG_CANT_GET_VI));

	return FALSE;
    }

    drawInfo = GetScreenDrawInfo(ec->ec_screen);
    fontHeight = drawInfo->dri_Font->tf_YSize;
    topOffset = fontHeight + ec->ec_screen->WBorTop;
    FreeScreenDrawInfo(ec->ec_screen, drawInfo);

    if (!initGadgets(ec, ec->ec_screen, fontHeight))
    {
	return FALSE;
    }

    setupMenus(ec->ec_catalog);

    ec->ec_menus = CreateMenusA(nm, NULL);

    if (ec->ec_menus == NULL)
    {
	showSimpleMessage(ec, getCatalog(ec->ec_catalog, MSG_CANT_CREATE_MENUS));

	return FALSE;
    }

    if (!LayoutMenusA(ec->ec_menus, ec->ec_visualInfo, NULL))
    {
	showSimpleMessage(ec, getCatalog(ec->ec_catalog, MSG_CANT_LAYOUT_MENUS));

	return FALSE;
    }
	
    winHeight = BORDERY * 2 + fontHeight + LABELSPACEY +
    	    	(fontHeight + EXTRAHEIGHT) * 4 + 
		SPACEY * 3 +
		ec->ec_screen->WBorTop + fontHeight + 1 +
		ec->ec_screen->WBorBottom;
    
    ec->ec_window = OpenWindowTags(NULL,
				   WA_PubScreen,    (Tag) NULL,
				   WA_Gadgets,      (Tag) ec->ec_context,
				   WA_Left,         0,
				   WA_Top,          0,
				   WA_Width,        500,
				   WA_Height,       winHeight,
				   WA_Title,        (Tag)getCatalog(ec->ec_catalog, MSG_EXCHANGE_WINTITLE),
 				   WA_IDCMP,        BUTTONIDCMP | CYCLEIDCMP |
				                    LISTVIEWIDCMP | 
				                    IDCMP_CLOSEWINDOW |
				                    IDCMP_GADGETUP |
						    IDCMP_RAWKEY |
				                    IDCMP_MENUPICK |
						    IDCMP_VANILLAKEY,
				   WA_DragBar,      TRUE,
				   WA_CloseGadget,  TRUE,
				   WA_DepthGadget,  TRUE,
				   WA_SmartRefresh, TRUE,
				   WA_Activate	  , TRUE,
				   TAG_DONE);
    
    if (ec->ec_window == NULL)
    {
	showSimpleMessage(ec, getCatalog(ec->ec_catalog, MSG_CANT_CREATE_WIN));

	return FALSE;
    }

    SetMenuStrip(ec->ec_window, ec->ec_menus);

    UnlockPubScreen(NULL, ec->ec_screen);
    ec->ec_screen = NULL;

    return TRUE;
}


enum { ID_listView, ID_cycle, ID_showBut, ID_hideBut, ID_killBut,
       ID_textGad, ID_textGad2 };


struct NewGadget listView =
{
    10,
    32,
    180,
    78,
    NULL,
    NULL,			/* TextAttr */
    ID_listView,
    PLACETEXT_ABOVE,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget cycleBut =
{
    200,
    94,
    140,
    16,
    NULL,			/* Text */
    NULL,			/* TextAttr */
    ID_cycle,
    PLACETEXT_IN,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget killBut =
{
    344,
    94,
    140,
    16,
    NULL,                	/* Set by getCatalog()  */
    NULL,			/* TextAttr */
    ID_killBut,
    PLACETEXT_IN,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget showBut =
{
    200,
    76,
    140,
    16,
    NULL,                	/* Set by getCatalog() */
    NULL,			/* TextAttr */
    ID_showBut,
    PLACETEXT_IN,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget hideBut =
{
    344,
    76,
    140,
    16,
    NULL,                	/* Set by getCatalog() */
    NULL,			/* TextAttr */
    ID_hideBut,
    PLACETEXT_IN,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget textGad =
{
    200,
    32,
    284,
    18,
    NULL,	             	/* Set by getCatalog() */
    NULL,			/* TextAttr */
    ID_textGad,
    PLACETEXT_ABOVE,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


struct NewGadget textGad2 =
{
    200,
    32 + 20 + 2,
    284,
    18,
    NULL,
    NULL,			/* TextAttr */
    ID_textGad2,
    PLACETEXT_ABOVE,
    NULL,			/* Visual Info */
    NULL			/* User data */
};


/* Cycle gadget texts */
//STRPTR  strings[] = { "Active", "Inactive", NULL };
CONST_STRPTR strings[3];

BOOL initGadgets(struct ExchangeState *ec, struct Screen *scr, LONG fontHeight)
{
    struct Gadget *gad;
    LONG y, h;
    
    listView.ng_VisualInfo = ec->ec_visualInfo;
    cycleBut.ng_VisualInfo = ec->ec_visualInfo;
    textGad.ng_VisualInfo  = ec->ec_visualInfo;
    textGad2.ng_VisualInfo = ec->ec_visualInfo;
    showBut.ng_VisualInfo  = ec->ec_visualInfo;
    hideBut.ng_VisualInfo  = ec->ec_visualInfo;
    killBut.ng_VisualInfo  = ec->ec_visualInfo;

    listView.ng_GadgetText = getCatalog(ec->ec_catalog, MSG_EXCHANGE_LISTVIEW);
    showBut.ng_GadgetText = getCatalog(ec->ec_catalog, MSG_EXCHANGE_GAD_SHOW);
    hideBut.ng_GadgetText = getCatalog(ec->ec_catalog, MSG_EXCHANGE_GAD_HIDE);
    killBut.ng_GadgetText = getCatalog(ec->ec_catalog, MSG_EXCHANGE_GAD_REMOVE);

    strings[0] = getCatalog(ec->ec_catalog, MSG_EXCHANGE_CYCLE_ACTIVE);
    strings[1] = getCatalog(ec->ec_catalog, MSG_EXCHANGE_CYCLE_INACTIVE);
    strings[2] = NULL;

    gad = CreateContext(&ec->ec_context);

    y = scr->WBorTop + fontHeight + 1 + BORDERY + fontHeight + LABELSPACEY;
    h = fontHeight + EXTRAHEIGHT;

    listView.ng_TopEdge = lvtop = y;
    listView.ng_Height  = lvheight = h * 4 + SPACEY*3;
    
    showBut.ng_TopEdge = y + 2 * (h + SPACEY);
    showBut.ng_Height  = h;
    
    hideBut.ng_TopEdge = y + 2 * (h + SPACEY);
    hideBut.ng_Height  = h;
    
    killBut.ng_TopEdge = y + 3 * (h + SPACEY);
    killBut.ng_Height  = h;
    
    cycleBut.ng_TopEdge = y + 3 * (h + SPACEY);
    cycleBut.ng_Height  = h;
    
    textGad.ng_TopEdge = y;
    textGad.ng_Height  = h;
    
    textGad2.ng_TopEdge = y + 1 * (h + SPACEY);
    textGad2.ng_Height  = h;
    
    ec->ec_listView = gad = lvgad = CreateGadget(LISTVIEW_KIND, gad,
					  &listView,
					  GTLV_Labels, (IPTR) NULL,
					  GTLV_ShowSelected, (IPTR) NULL,
					  TAG_DONE);
    
    ec->ec_showBut = gad = CreateGadget(BUTTON_KIND, gad,
					&showBut,
					GA_Disabled, TRUE,
					TAG_DONE);

    ec->ec_hideBut = gad = CreateGadget(BUTTON_KIND, gad,
					&hideBut,
					GA_Disabled, TRUE,
					TAG_DONE);

    ec->ec_killBut = gad = CreateGadget(BUTTON_KIND, gad,
					&killBut,
					GA_Disabled, TRUE,
					TAG_DONE);
    
    ec->ec_cycle = gad = CreateGadget(CYCLE_KIND, gad,
				      &cycleBut,
				      GA_Disabled, TRUE,
				      GTCY_Labels, (Tag)strings,  /* Temporary */
				      TAG_DONE);
    
    /* NOTE! GadTools bug: The disabled state for cycle gadgets is not
       changed when doing a GT_SetGadgetAttrs() */

    /* Information window */
    ec->ec_textGad = gad = CreateGadget(TEXT_KIND, gad,
					&textGad,
					//GTTX_Text    , "Exchange",
					//GTTX_CopyText, TRUE,
					GTTX_Border  , TRUE,
					TAG_DONE);
    
    ec->ec_textGad2 = gad = CreateGadget(TEXT_KIND, gad,
					 &textGad2,
					 //GTTX_Text    , "Test message",
					 //GTTX_CopyText, TRUE,
					 GTTX_Border  , TRUE,
					 TAG_DONE);

    return gad ? TRUE : FALSE;
}



void freeResources(struct ExchangeState *ec)
{
    struct LibTable *tmpLibTable = libTable;

    if (IntuitionBase != NULL)
    {
	if (ec->ec_window != NULL)
	{
	    ClearMenuStrip(ec->ec_window);
	    CloseWindow(ec->ec_window);
	    
	    D(bug("Closed window\n"));
	}
    }

    if (GadToolsBase != NULL)
    {
        /* Passing NULL to these functions is safe! */
	FreeMenus(ec->ec_menus);
	FreeGadgets(ec->ec_context);
	FreeVisualInfo(ec->ec_visualInfo);

    	UnlockPubScreen(NULL, ec->ec_screen);
	
	D(bug("Freed visualinfo\n"));
    }

    if (CxBase != NULL)
    {
	FreeBrokerList(&ec->ec_brokerList);
	DeleteCxObjAll(ec->ec_broker);
    }

    // DeleteMsgPort(ec->hotkeyPort);
    DeleteMsgPort(ec->ec_msgPort);

    if (LocaleBase != NULL)
    {
	CloseCatalog(ec->ec_catalog);

	D(bug("Closed catalog\n"));

	CloseLibrary((struct Library *)LocaleBase);
    }

    while (tmpLibTable->lT_Name)	/* Check for name rather than pointer */
    {
	if (tmpLibTable->lT_Library != NULL)
	{
	    CloseLibrary((*(struct Library **)tmpLibTable->lT_Library));
	    D(bug("Closed %s!\n", tmpLibTable->lT_Name));
	}

    	tmpLibTable++;
    }
}

void ScrollListview(struct ExchangeState *ec, struct Gadget *gad, WORD delta)
{
    IPTR top, visible, total;
    LONG newtop;
    
    GT_GetGadgetAttrs(gad, ec->ec_window, NULL, GTLV_Top, (IPTR)&top,
    	    	    	    	      GTLV_Visible, (IPTR)&visible,
				      GTLV_Total, (IPTR)&total,
				      TAG_DONE);
				      
    newtop = (LONG)top + delta * 3;

    if (newtop + visible > total)
    {
    	newtop = total - visible;
    }
    
    if (newtop < 0) newtop = 0;
    
    if (newtop != top)
    {
    	GT_SetGadgetAttrs(gad, ec->ec_window, NULL, GTLV_Top, newtop, TAG_DONE);
    }
    				      
}

void realMain(struct ExchangeState *ec)
{
    BOOL   quitNow = FALSE;
    ULONG  signals;
    ULONG  winSig = 1 << ec->ec_window->UserPort->mp_SigBit;
    ULONG  cxSig  = 1 << ec->ec_msgPort->mp_SigBit;
    
    while (!quitNow)
    {
	signals = Wait(cxSig | winSig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E);

	if (signals & winSig)
	{
	    struct IntuiMessage *msg;

	    while ((msg = GT_GetIMsg(ec->ec_window->UserPort)) != NULL)
	    {
		D(bug("Got win signal %i\n", msg->Class));

		switch (msg->Class)
		{
		case IDCMP_CLOSEWINDOW:
		    quitNow = TRUE;
		    break;

    	    	case IDCMP_VANILLAKEY:
		    if (msg->Code == 27) quitNow = TRUE;
		    break;
		    
		case IDCMP_GADGETUP:
		    {		
			struct Gadget *gadget = (struct Gadget *)msg->IAddress;
			
			switch (gadget->GadgetID)
			{
			case ID_listView:
			    updateInfo(ec);
			    break;
			    
			case ID_cycle:
			    switchActive(ec);
			    break;
			    
			case ID_showBut:
			    informBroker(CXCMD_APPEAR, ec);
			    break;
			    
			case ID_hideBut:
			    informBroker(CXCMD_DISAPPEAR, ec);
			    break;

			case ID_killBut:
			    informBroker(CXCMD_KILL, ec);
			    break;
			}
			
		    } /* case IDCMP_GADGETUP: */
		    break;

		case IDCMP_MENUPICK:
		    {
			struct MenuItem *item;
			UWORD menuNum = msg->Code;
			
			while (menuNum != MENUNULL)
			{			
			    item = ItemAddress(ec->ec_menus, menuNum);
	
			    if (item != NULL)
			    {
				menuNum = item->NextSelect;
				
				D(bug("Menu selection: %i",
					  (int)GTMENUITEM_USERDATA(item)));

				switch ((LONG)GTMENUITEM_USERDATA(item))
				{
				case MSG_MEN_PROJECT_QUIT:
				    quitNow = TRUE;

				    break;
				}
			    }
			    else
			    {
				menuNum = MENUNULL;
			    }
			}
		    }
		    break;

		case IDCMP_RAWKEY:
		    {
			WORD delta;
			
			switch(msg->Code)
			{
			case RAWKEY_NM_WHEEL_UP:
				delta = -1;
				break;
				
			case RAWKEY_NM_WHEEL_DOWN:
				delta = 1;
				break;
				
			default:
				delta = 0;
				break;
			}
			
			if (delta)
			{
				if (lvgad)
				{
					ScrollListview(ec, lvgad, delta);
				}
			}
		    }
		    break;    
		} /* switch(msg->Class) */
		
		GT_ReplyIMsg(msg);

	    } /* while(GT_GetIMsg()) */
	} /* if(signals & winSig) */
	
	if (signals & cxSig)
	{
	    CxMsg *cxm;
	    
	    while ((cxm = (CxMsg *)GetMsg(ec->ec_msgPort)) != NULL)
	    {
		D(bug("Got cx signal %i\n", CxMsgID(cxm)));

		if (CxMsgType(cxm) == CXM_COMMAND)
		{
		    switch (CxMsgID(cxm))
		    {
		    case CXCMD_KILL:
		    case CXCMD_DISAPPEAR:
			quitNow = TRUE;
			break;

		    case CXCMD_UNIQUE:
			/* If somebody tried to start Exchange, we put the
			   already running program to front */
		    case CXCMD_APPEAR:
			appearExchange(ec);
			break;

		    case CXCMD_LIST_CHG:
			redrawList(ec);
			break;
			
		    case CXCMD_ENABLE:
			ActivateCxObj(ec->ec_broker, TRUE);
			break;

		    case CXCMD_DISABLE:
			ActivateCxObj(ec->ec_broker, FALSE);
			break;
		    }
		}

		ReplyMsg((struct Message *)cxm);
	    } /* while(GetMsg(...) != NULL) */
	}


	/* Abandon ship? */
	if (signals & SIGBREAKF_CTRL_C || signals & SIGBREAKF_CTRL_E)
	{
	    break;
	}
    }
}


void switchActive(struct ExchangeState *ec)
{
    IPTR  whichCycle;

    GT_GetGadgetAttrs(ec->ec_cycle, ec->ec_window, NULL,
		      GTCY_Active, (Tag)&whichCycle,
		      TAG_DONE);
    
    if (whichCycle == 0)
    {
	/* Activate */
	informBroker(CXCMD_ENABLE, ec);
    }
    else
    {
	/* Inactivate */
	informBroker(CXCMD_DISABLE, ec);
    }
}


void informBroker(LONG command, struct ExchangeState *ec)
{
    IPTR                whichGad;
    struct BrokerCopy  *bc;

    GT_GetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Selected, (Tag)&whichGad,
		      TAG_DONE);

    bc = getNth(&ec->ec_brokerList, whichGad);

    D(bug("Informing the broker <%s>. Reason %d\n", bc->bc_Node.ln_Name,
	      (int)command));

    /* We don't care about errors for now */
    CxNotify(bc->bc_Node.ln_Name, command);
}


struct BrokerCopy *getNth(struct List *list, LONG n)
{
    struct Node *brok = GetHead(list);

    while (n > 0)
    {
	brok = GetSucc(brok);
	n--;
    }

    return (struct BrokerCopy *)brok;
}


void updateInfo(struct ExchangeState *ec)
{
    IPTR  whichGad;

    GT_GetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Selected, (Tag)&whichGad,
		      TAG_DONE);
		      
    if (whichGad == ~0ul)
    {
	/* Disable the whole "right side". */
	setGadgetState(ec->ec_showBut, TRUE, ec);
	setGadgetState(ec->ec_hideBut, TRUE, ec);
	setGadgetState(ec->ec_cycle  , TRUE, ec);
	setGadgetState(ec->ec_killBut, TRUE, ec);
	setText(ec->ec_textGad,  NULL, ec);
	setText(ec->ec_textGad2, NULL, ec);
    }
    else
    {
	struct BrokerCopy *brokerCopy = getNth(&ec->ec_brokerList, whichGad);
	BOOL   showHide = (brokerCopy->bc_Flags & COF_SHOW_HIDE) == 0;

    	strncpy(ec->ec_lvitemname, brokerCopy->bc_Name, sizeof(ec->ec_lvitemname));
	
	D(bug("Broker: %s Flags: %i\n", brokerCopy->bc_Name,
		  brokerCopy->bc_Flags));

	setText(ec->ec_textGad,  (STRPTR)&brokerCopy->bc_Title, ec);
	setText(ec->ec_textGad2, (STRPTR)&brokerCopy->bc_Descr,	ec);

	D(bug("%s show/hide gadgets.\n", showHide ? "Disabling" : "Enabling"));
	setGadgetState(ec->ec_hideBut, showHide, ec);
	setGadgetState(ec->ec_showBut, showHide, ec);

	GT_SetGadgetAttrs(ec->ec_cycle, ec->ec_window, NULL,
			  GTCY_Active, (brokerCopy->bc_Flags & COF_ACTIVE) ? 0 : 1,
			  GA_Disabled, FALSE,
			  TAG_DONE);

	setGadgetState(ec->ec_killBut, FALSE, ec);
    }
}


void setText(struct Gadget *gadget, STRPTR text, struct ExchangeState *ec)
{
    GT_SetGadgetAttrs(gadget, ec->ec_window, NULL,
		      GTTX_Text, (Tag)text,
		      TAG_DONE);
}


void setGadgetState(struct Gadget *gadget, BOOL status,
		    struct ExchangeState *ec)
{
    GT_SetGadgetAttrs(gadget, ec->ec_window, NULL,
		      GA_Disabled, status,
		      TAG_DONE);
}


void appearExchange(struct ExchangeState *ec)
{
    WindowToFront(ec->ec_window);
    ActivateWindow(ec->ec_window);
}


void redrawList(struct ExchangeState *ec)
{
    struct BrokerCopy *node;
    LONG n, i = 0;

    n = GetBrokerList(&ec->ec_brokerList);

    D(bug("Antal %i\n", n));

    D(bug("First broker: %s\n", GetHead(&ec->ec_brokerList)->ln_Name));

    D(bug("Calling GT_SetGadgetAttrs()\n"));
    GT_SetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Labels, (IPTR)&ec->ec_brokerList,
		      TAG_DONE);

    n = -1;
    ForeachNode(&ec->ec_brokerList, node)
    {
    	if (strcmp(node->bc_Name, ec->ec_lvitemname) == 0)
	{
	    n = i;
	    break;
	}
	i++;
    }
    
    if (n == -1)
    {
    	ec->ec_lvitemname[0] = 0;
    }
    
    GT_SetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Selected, n, TAG_DONE);

    /* Update the text gadgets */
    updateInfo(ec);
}


CONST_STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
    if (catalogPtr != NULL)
    {
        return GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
    }
    else
    {
        return CatCompArray[id].cca_Str;
    }
}

void setupMenus(struct Catalog *catalogPtr)
{
    struct NewMenu *newMenuPtr = nm;
    ULONG id;

    while(newMenuPtr->nm_Type != NM_END)
    {

	if(newMenuPtr->nm_Label != NM_BARLABEL)
	{
	    id = (ULONG)newMenuPtr->nm_Label;

	    if(newMenuPtr->nm_Type == NM_TITLE)
		newMenuPtr->nm_Label = getCatalog(catalogPtr, id);
	    else
	    {
		newMenuPtr->nm_Label = getCatalog(catalogPtr, id) + 2; /* CAUTION: Menus will be crippled if the lack keyboard shortcuts! */
		newMenuPtr->nm_CommKey = getCatalog(catalogPtr, id);
		newMenuPtr->nm_UserData = (APTR)id; /* petah: OK? */
	    }
	}
	newMenuPtr++;
    }
}


void showSimpleMessage(struct ExchangeState *ec, CONST_STRPTR msgString)
{
    struct EasyStruct easyStruct;

    easyStruct.es_StructSize	= sizeof(easyStruct);
    easyStruct.es_Flags		= 0;
    easyStruct.es_Title		= getCatalog(ec->ec_catalog, MSG_EXCHANGE_CXNAME);
    easyStruct.es_TextFormat	= msgString;
    easyStruct.es_GadgetFormat	= getCatalog(ec->ec_catalog, MSG_OK);		

    if (IntuitionBase != NULL && !Cli() && ec->ec_window)
    {
	EasyRequestArgs(ec->ec_window, &easyStruct, NULL, NULL);
    }
    else
    {
	printf("%s", msgString);
    }
}
