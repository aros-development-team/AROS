/*
   (C) 2000-2001 AROS - The Amiga Research OS
   $Id$

   Desc: Exchange -- controls commodities
   Lang: English
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

    HISTORY

    0x.01.2000  SDuvan   implemented

******************************************************************************/

#define  AROS_ALMOST_COMPATIBLE

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

#include <string.h>

#include <aros/debug.h>

#define  P(x)   x

#define BORDERY      4
#define SPACEY       4
#define EXTRAHEIGHT  6
#define LABELSPACEY  4

struct ExchangeState
{
    CxObj          *ec_broker;
    struct MsgPort *ec_msgPort;	     /* Message port for our broker */
    struct Catalog *ec_catalog;	     /* Commodities locale catalog */
    struct List     ec_brokerList;   /* Current list of brokers
					(struct BrokerCopy nodes */

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


    /* Values settable via tooltypes/command arguments */
    BOOL       ec_cxPopup;
    STRPTR     ec_cxPopKey;	/* Hotkey for Exchange */
    LONG       ec_cxPri;	/* Priority of the Exchange broker */
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


int main(int argc, char **argv)
{
    struct ExchangeState ec;
    int    retval = RETURN_OK;

    if(getResources(&ec))
    {
	if(Cli() == NULL)
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
    "Exchange",
    "Commodities Exchange",	/* Should get this from the catalog */
    "Controls system commodities",  /* -- " -- */
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
	ec->ec_cxPopKey = "ctrl alt help";
    }

    FreeArgs(rda);

    return TRUE;
}


BOOL readWBArgs(int argc, char **argv, struct ExchangeState *ec)
{
    STRPTR    popUp;
    UBYTE   **tt;

    IconBase = OpenLibrary("icon.library", 40);
    
    if (IconBase == NULL)
    {
	return FALSE;
    }
    
    tt = ArgArrayInit(argc, (UBYTE **)argv);
    
    ec->ec_cxPri    = ArgInt(tt, "CX_PRIORITY", 0);
    ec->ec_cxPopKey = ArgString(tt, "CX_POPKEY", "ctrl alt help");
    popUp = ArgString(tt, "CX_POPUP", "YES");

    if (Stricmp(popUp, "YES") == 0)
    {
	ec->ec_cxPopup = TRUE;
    }

    ArgArrayDone();
    CloseLibrary(IconBase);

    return TRUE;
}


enum { menu_Quit };

static struct NewMenu nm[] =
{
    {NM_TITLE, "Project" },
    {NM_ITEM,  "Quit", "Q", 0, 0, (APTR)menu_Quit },
    {NM_END}
};


BOOL getResources(struct ExchangeState *ec)
{
    struct Screen    *screen;
    struct DrawInfo  *drawInfo;
    LONG              topOffset;
    LONG    	      fontHeight;
    LONG    	      winHeight;
    
    memset(ec, 0, sizeof(struct ExchangeState));

    /* First, open necessary libraries */
    
    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 40);
    
    if (UtilityBase == NULL)
    {
	return FALSE;
    }
    
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",
							40);
    
    if (IntuitionBase == NULL)
    {
	return FALSE;
    }
    
    CxBase = OpenLibrary("commodities.library", 40);
    
    if (CxBase == NULL)
    {
	return FALSE;
    }
    

    //    LocaleBase = OpenLibrary("locale.library", 40);
    
    //    if(LocaleBase == NULL)
    //        return FALSE;
    
    GadToolsBase = OpenLibrary("gadtools.library", 40);
    
    if (GadToolsBase == NULL)
    {
	return FALSE;
    }
    

    P(kprintf("Libraries opened!"));
 
    /* Then, allocate the rest of the resources */
    
    ec->ec_msgPort = CreateMsgPort();
    
    if (ec->ec_msgPort == NULL)
    {
	return FALSE;
    }

    // ec->ec_hoykeyPort = CreateMsgPort();
    
    // if(ec->ec_hotkeyPort == NULL)
    //     return FALSE;

    
    /* TODO: BuiltInLanguage should be NULL, but AROS' locale is not
             complete */
    //    ec->ec_catalog = OpenCatalog(NULL, "Sys/commodities.catalog",
    //			         OC_BuiltInLanguage, "english", TAG_DONE);
    
    //    if(ec->ec_catalog == NULL)
    //        return FALSE;
    
    NEWLIST(&ec->ec_brokerList);

    exBroker.nb_Port = ec->ec_msgPort;
    ec->ec_broker = CxBroker(&exBroker, NULL);

    if (ec->ec_broker == NULL)
    {
	return FALSE;
    }
    
    P(kprintf("Broker: %s Flags: %i\n", ec->ec_broker->co_Ext.co_BExt->bext_Name,
	      ec->ec_broker->co_Flags));


    // AttachCxObj(ec->ec_broker, Hotkey("ctrl alt help", ec->ec_hotkeyPort));

    if (CxObjError(ec->ec_broker) != 0)
    {
	return FALSE;
    }

    /* Start our broker */
    ActivateCxObj(ec->ec_broker, TRUE);

    screen = LockPubScreen(NULL);

    if (screen == NULL)
    {
	return FALSE;
    }

    ec->ec_visualInfo = GetVisualInfoA(screen, NULL);

    if (ec->ec_visualInfo == NULL)
    {
	return FALSE;
    }

    drawInfo = GetScreenDrawInfo(screen);
    fontHeight = drawInfo->dri_Font->tf_YSize;
    topOffset = fontHeight + screen->WBorTop;
    FreeScreenDrawInfo(screen, drawInfo);

    if (!initGadgets(ec, screen, fontHeight))
    {
	return FALSE;
    }

    ec->ec_menus = CreateMenusA(nm, NULL);

    if (ec->ec_menus == NULL)
    {
	return FALSE;
    }

    if (!LayoutMenusA(ec->ec_menus, ec->ec_visualInfo, NULL))
    {
	return FALSE;
    }
	
    winHeight = BORDERY * 2 + fontHeight + LABELSPACEY +
    	    	(fontHeight + EXTRAHEIGHT) * 4 + 
		SPACEY * 3 +
		screen->WBorTop + fontHeight + 1 +
		screen->WBorBottom;
    
    ec->ec_window = OpenWindowTags(NULL,
				   WA_PubScreen,    NULL,
				   WA_Gadgets,      ec->ec_context,
				   WA_Left,         0,
				   WA_Top,          0,
				   WA_Width,        500,
				   WA_Height,       winHeight,
				   WA_Title,        "Exchange:", /* TODO */
				   WA_IDCMP,        BUTTONIDCMP | CYCLEIDCMP |
				                    LISTVIEWIDCMP | 
				                    IDCMP_CLOSEWINDOW |
				                    IDCMP_GADGETUP |
				                    IDCMP_MENUPICK,
				   WA_DragBar,      TRUE,
				   WA_CloseGadget,  TRUE,
				   WA_DepthGadget,  TRUE,
				   WA_SmartRefresh, TRUE,
				   TAG_DONE);
    
    if (ec->ec_window == NULL)
    {
	return FALSE;
    }

    SetMenuStrip(ec->ec_window, ec->ec_menus);

    UnlockPubScreen(NULL, screen);

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
    "Available commodities",	/* TODO: Use the catalog */
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
    "Remove",                	/* TODO: Use the catalog */
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
    "Show",                	/* TODO: Use the catalog */
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
    "Hide",                	/* TODO: Use the catalog */
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
    "Information",             	/* TODO: Use the catalog */
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
STRPTR  strings[] = { "Active", "Inactive", NULL };

BOOL initGadgets(struct ExchangeState *ec, struct Screen *scr, LONG fontHeight)
{
    LONG y, h;
    
    listView.ng_VisualInfo = ec->ec_visualInfo;
    cycleBut.ng_VisualInfo = ec->ec_visualInfo;
    textGad.ng_VisualInfo  = ec->ec_visualInfo;
    textGad2.ng_VisualInfo = ec->ec_visualInfo;
    showBut.ng_VisualInfo  = ec->ec_visualInfo;
    hideBut.ng_VisualInfo  = ec->ec_visualInfo;
    killBut.ng_VisualInfo  = ec->ec_visualInfo;

    CreateContext(&ec->ec_context);

    y = scr->WBorTop + fontHeight + 1 + BORDERY + fontHeight + LABELSPACEY;
    h = fontHeight + EXTRAHEIGHT;
    
    listView.ng_TopEdge = y;
    listView.ng_Height  = h * 4 + SPACEY * 3;
    
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
    
    if (ec->ec_context == NULL)
    {
	return FALSE;
    }

    if ((ec->ec_listView = CreateGadget(LISTVIEW_KIND, ec->ec_context,
					&listView,
					GTLV_Labels, NULL,
					GTLV_ShowSelected, NULL,
					TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    if ((ec->ec_showBut = CreateGadget(BUTTON_KIND, ec->ec_context,
				       &showBut,
				       GA_Disabled, TRUE,
				       TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    if ((ec->ec_hideBut = CreateGadget(BUTTON_KIND, ec->ec_context,
				       &hideBut,
				       GA_Disabled, TRUE,
				       TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    if ((ec->ec_killBut = CreateGadget(BUTTON_KIND, ec->ec_context,
				       &killBut,
				       GA_Disabled, TRUE,
				       TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    if ((ec->ec_cycle = CreateGadget(CYCLE_KIND, ec->ec_context,
				     &cycleBut,
				     GA_Disabled, TRUE,
				     GTCY_Labels, strings,  /* Temporary */
				     TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    /* NOTE! GadTools bug: The disabled state for cycle gadgets is not
       changed when doing a GT_SetGadgetAttrs() */

    /* Information window */
    if ((ec->ec_textGad = CreateGadget(TEXT_KIND, ec->ec_context,
				       &textGad,
				       GTTX_Text    , "Exchange",
				       GTTX_CopyText, TRUE,
				       GTTX_Border  , TRUE,
				       TAG_DONE)) == NULL)
    {
	return FALSE;
    }
    
    if ((ec->ec_textGad2 = CreateGadget(TEXT_KIND, ec->ec_context,
					&textGad2,
					GTTX_Text    , "Test message",
					GTTX_CopyText, TRUE,
					GTTX_Border  , TRUE,
					TAG_DONE)) == NULL)
    {
	return FALSE;
    }

    /* NOTE! GadTools bug: If GTTX_CopyText is not set, the gadget will not
             be displayed */

    return TRUE;
}



void freeResources(struct ExchangeState *ec)
{
    ClearMenuStrip(ec->ec_window);
    CloseWindow(ec->ec_window);

    P(kprintf("Closed window\n"));

    FreeMenus(ec->ec_menus);
    FreeGadgets(ec->ec_context);
    FreeVisualInfo(ec->ec_visualInfo);

    P(kprintf("Freed visualinfo\n"));

    FreeBrokerList(&ec->ec_brokerList);

    P(kprintf("Freed brokerlist\n"));

    // CloseCatalog(ec->ec_catalog);

    P(kprintf("Closed catalog\n"));

    DeleteCxObjAll(ec->ec_broker);

    P(kprintf("Deleted broker\n"));

    // DeleteMsgPort(ec->hotkeyPort);
    DeleteMsgPort(ec->ec_msgPort);

    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)LocaleBase);
    CloseLibrary(CxBase);
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
		P(kprintf("Got win signal %i\n", msg->Class));

		switch (msg->Class)
		{
		case IDCMP_CLOSEWINDOW:
		    quitNow = TRUE;
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
			
			GT_ReplyIMsg(msg);

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
				
				P(kprintf("Menu selection: %i",
					  (int)GTMENUITEM_USERDATA(item)));

				switch ((LONG)GTMENUITEM_USERDATA(item))
				{
				case menu_Quit:
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
		} /* switch(msg->Class) */
	    } /* while(GT_GetIMsg()) */
	} /* if(signals & winSig) */
	
	if (signals & cxSig)
	{
	    CxMsg *cxm;
	    
	    while ((cxm = (CxMsg *)GetMsg(ec->ec_msgPort)) != NULL)
	    {
		P(kprintf("Got cx signal %i\n", CxMsgID(cxm)));

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
		      GTCY_Active, &whichCycle,
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
    IPTR    whichGad;
    CxObj  *brok;

    GT_GetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Selected, &whichGad,
		      TAG_DONE);

    brok = getNth(&ec->ec_brokerList, whichGad);

    P(kprintf("Informing the broker <%s>. Reason %d\n", brok->co_Node.ln_Name,
	      (int)command));

    /* We don't care about errors for now */
    CxNotify(brok->co_Node.ln_Name, command);
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
		      GTLV_Selected, &whichGad,
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

	P(kprintf("Broker: %s Flags: %i\n", brokerCopy->bc_Name,
		  brokerCopy->bc_Flags));

	setText(ec->ec_textGad,  (STRPTR)&brokerCopy->bc_Title, ec);
	setText(ec->ec_textGad2, (STRPTR)&brokerCopy->bc_Descr,	ec);

	P(kprintf("%s show/hide gadgets.\n", showHide ? "Disabling" : "Enabling"));
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
		      GTTX_Text, text,
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
    LONG n;

    n = GetBrokerList(&ec->ec_brokerList);

    P(kprintf("Antal %i\n", n));

    P(kprintf("First broker: %s\n", GetHead(&ec->ec_brokerList)->ln_Name));

    P(kprintf("Calling GT_SetGadgetAttrs()\n"));
    GT_SetGadgetAttrs(ec->ec_listView, ec->ec_window, NULL,
		      GTLV_Labels, (IPTR)&ec->ec_brokerList,
		      TAG_DONE);
}
