/*
    Copyright © 2007, The AROS Development Team. All rights reserved.
    $Id$

    DepthMenu commodity -- shows list of window/screens in popupmenu.
*/

/******************************************************************************

    NAME

        DepthMenu

    SYNOPSIS

        CX_PRIORITY/N/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Shows popup menu with list of screens and windows after user clicks on
	depth gadget of screen or window.

    INPUTS

        CX_PRIORITY  --  The priority of the commodity

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <aros/symbolsets.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/input.h>
#include <proto/alib.h>
#include <proto/locale.h>

#include <stdio.h>
#include <string.h>

#define  DEBUG 0
#include <aros/debug.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  2


/***************************************************************************/

UBYTE version[] = "$VER: DepthMenu 0.1 (03.04.2007)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define  ARG_PRI        0
#define  NUM_ARGS       1

#define DM_WINDOW       1
#define DM_SCREEN       2

#define MAXENTRY        15
#define MAXSTRLEN       30

#define BORDERWIDTH     8

struct Device         *InputBase = NULL;
struct Catalog        *catalog;
struct IOStdReq       *inputIO;


/* The Depth broker */
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


typedef struct _DMState
{
    CxObj          *cs_broker;
    struct MsgPort *cs_msgPort;
} DMState;


typedef struct _DMData
{
    struct Window *popupwindow;
    WORD entries;
    WORD mode;
    void *address[MAXENTRY]; // struct Window * or struct Screen *
    char title[MAXENTRY][MAXSTRLEN];
    ULONG ibaselock;
    BOOL locked;
} DMData;

static DMData dmdata;

/************************************************************************************/

static void freeResources(DMState *cs);
static BOOL initiate(int argc, char **argv, DMState *cs);
static void depthMenu(CxMsg *msg, CxObj *co);
static void handleMenuUp(CxMsg *cxm);
static void handleMenuDown(CxMsg *cxm);
static void showPopup(WORD mode, struct Screen *screen, WORD xpos, WORD ypos);
static void lockIBaseSave(void);
static void unlockIBaseSave(void);
static CONST_STRPTR _(ULONG id);
static BOOL Locale_Initialize(VOID);
static VOID Locale_Deinitialize(VOID);
static void showSimpleMessage(CONST_STRPTR msgString);

/************************************************************************************/

static CONST_STRPTR _(ULONG id)
{
    if (LocaleBase != NULL && catalog != NULL)
    {
	return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
	return CatCompArray[id].cca_Str;
    }
}

/************************************************************************************/

static BOOL Locale_Initialize(VOID)
{
    if (LocaleBase != NULL)
    {
	catalog = OpenCatalog
	    ( 
	     NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE 
	    );
    }
    else
    {
	catalog = NULL;
    }

    return TRUE;
}

/************************************************************************************/

static VOID Locale_Deinitialize(VOID)
{
    if(LocaleBase != NULL && catalog != NULL) CloseCatalog(catalog);
}

/************************************************************************************/

static void showSimpleMessage(CONST_STRPTR msgString)
{
    struct EasyStruct easyStruct;

    easyStruct.es_StructSize	= sizeof(easyStruct);
    easyStruct.es_Flags		= 0;
    easyStruct.es_Title		= _(MSG_DEPTHMENU_CXNAME);
    easyStruct.es_TextFormat	= msgString;
    easyStruct.es_GadgetFormat	= _(MSG_OK);		

    if (IntuitionBase != NULL && !Cli() )
    {
	EasyRequestArgs(NULL, &easyStruct, NULL, NULL);
    }
    else
    {
	PutStr(msgString);
    }
}

/************************************************************************************/

static BOOL initiate(int argc, char **argv, DMState *cs)
{
    CxObj *customObj;

    memset(cs, 0, sizeof(DMState));

    if (Cli() != NULL)
    {
	struct RDArgs *rda;
	IPTR           args[] = { (IPTR) NULL, (IPTR) NULL, FALSE };

	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
	if (rda != NULL)
	{
	    if (args[ARG_PRI] != (IPTR) NULL)
	    {
		nb.nb_Pri = *(LONG *)args[ARG_PRI];
	    }
	}
	FreeArgs(rda);
    }
    else
    {
	UBYTE  **array = ArgArrayInit(argc, (UBYTE **)argv);
	nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
	ArgArrayDone();
    }

    nb.nb_Name = _(MSG_DEPTHMENU_CXNAME);
    nb.nb_Title = _(MSG_DEPTHMENU_CXTITLE);
    nb.nb_Descr = _(MSG_DEPTHMENU_CXDESCR);

    cs->cs_msgPort = CreateMsgPort();
    if (cs->cs_msgPort == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_MSGPORT));
	return FALSE;
    }

    nb.nb_Port = cs->cs_msgPort;
    cs->cs_broker = CxBroker(&nb, 0);
    if (cs->cs_broker == NULL)
    {
	return FALSE;
    }

    customObj = CxCustom(depthMenu, 0);
    if (customObj == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_MSGPORT));
	return FALSE;
    }

    AttachCxObj(cs->cs_broker, customObj);
    ActivateCxObj(cs->cs_broker, TRUE);

    inputIO = (struct IOStdReq *)CreateIORequest(cs->cs_msgPort,
	    sizeof(struct IOStdReq));

    if (inputIO == NULL)
    {
	showSimpleMessage(_(MSG_CANT_ALLOCATE_MEM));
	return FALSE;
    }

    if ((OpenDevice("input.device", 0, (struct IORequest *)inputIO, 0)) != 0)
    {
	showSimpleMessage(_(MSG_CANT_OPEN_INPUTDEVICE));
	return FALSE;
    }

    InputBase = (struct Device *)inputIO->io_Device;

    return TRUE;
}

/************************************************************************************/

static void freeResources(DMState *cs)
{
    struct Message *cxm;

    if (cs->cs_broker != NULL)
    {
	DeleteCxObjAll(cs->cs_broker);
    }

    if (cs->cs_msgPort != NULL)
    {
	while ((cxm = GetMsg(cs->cs_msgPort)))
	{
	    ReplyMsg(cxm);
	}
	DeleteMsgPort(cs->cs_msgPort);
    }

    if (inputIO != NULL)
    {
	CloseDevice((struct IORequest *)inputIO);
	DeleteIORequest((struct IORequest *)inputIO);
    }
}

/************************************************************************************/

static void depthMenu(CxMsg *cxm, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(cxm);
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
	if (ie->ie_Code == MENUDOWN)
	{
	    handleMenuDown(cxm);
	}
	else if (ie->ie_Code == MENUUP)
	{
	    handleMenuUp(cxm);
	}
    }
}

/************************************************************************************/

static void handleMenuDown(CxMsg *cxm)
{
    struct Screen *screen;
    struct Layer  *layer;
    struct Window *window;

    lockIBaseSave();
    screen = IntuitionBase->FirstScreen; // frontmost screen
    layer = WhichLayer(&screen->LayerInfo, screen->MouseX, screen->MouseY);

    if (layer)
    {
	if (layer == screen->BarLayer) // Title bar of screen
	{
	    D(bug("DepthMenu: BarLayer\n"));
	    if (
		    screen->Title
		    && (screen->MouseX > screen->Width - 25) // FIXME real width/pos. of depth gadget
		    && (screen->MouseX < screen->Width)
		    && (screen->MouseY > 0)
		    && (screen->MouseY < screen->BarHeight)
	       )
	    {
		showPopup(DM_SCREEN, screen, screen->MouseX, screen->MouseY);
		DisposeCxMsg(cxm);
	    }
	}
	else
	{
	    D(bug("DepthMenu: other Layer\n"));
	    window = layer->Window;
	    if (
		    window
		    && window->Title
		    && (screen->MouseX > window->LeftEdge + window->Width - 25)  // FIXME real width/pos. of depth gadget
		    && (screen->MouseX < window->LeftEdge + window->Width)
		    && (screen->MouseY > window->TopEdge)
		    && (screen->MouseY < window->TopEdge + window->BorderTop)
	       )
	    {
		showPopup(DM_WINDOW, screen, screen->MouseX, screen->MouseY);
		DisposeCxMsg(cxm);
	    }
	}
    }
    unlockIBaseSave();
}

/************************************************************************************/

static void handleMenuUp(CxMsg *cxm)
{
    if (dmdata.popupwindow)
    {
	struct Screen *screen = dmdata.popupwindow->WScreen;
	// have we released right mouse within window?
	if (
		(screen->MouseX > dmdata.popupwindow->LeftEdge)
		&& (screen->MouseX < dmdata.popupwindow->LeftEdge + dmdata.popupwindow->Width)
		&& (screen->MouseY > dmdata.popupwindow->TopEdge)
		&& (screen->MouseY < dmdata.popupwindow->TopEdge + dmdata.popupwindow->Height)
	   )
	{
	    LONG entry = (screen->MouseY - dmdata.popupwindow->TopEdge - BORDERWIDTH)
		/ dmdata.popupwindow->RPort->TxHeight;

	    CloseWindow(dmdata.popupwindow);
	    dmdata.popupwindow = NULL;

	    D(bug("DepthMenu: selected entry %d\n", entry));
	    if ((entry >= 0) && (entry < MAXENTRY))
	    {
		if (dmdata.mode == DM_WINDOW)
		{
		    // does selected window still exist?
		    lockIBaseSave();
		    struct Window *checkwin = screen->FirstWindow;
		    while (checkwin)
		    {
			if (checkwin == dmdata.address[entry])
			{
			    unlockIBaseSave();
			    WindowToFront(dmdata.address[entry]);
			    ActivateWindow(dmdata.address[entry]);
			    break;
			}
			checkwin = checkwin->NextWindow;
		    }
		}
		else
		{
		    // does selected screen still exist?
		    lockIBaseSave();
		    struct Screen *checkscreen = IntuitionBase->FirstScreen;
		    while (checkscreen)
		    {
			if (checkscreen == dmdata.address[entry])
			{
			    unlockIBaseSave();
			    ScreenToFront(dmdata.address[entry]);
			    break;
			}
			checkscreen = checkscreen->NextScreen;
		    }
		}
	    }	    
	}
	// when we release mouse outside window it's still open
	if (dmdata.popupwindow)
	{
	    CloseWindow(dmdata.popupwindow);
	    dmdata.popupwindow = NULL;
	}
    }
    unlockIBaseSave();
}

/************************************************************************************/

static void showPopup(WORD mode, struct Screen *screen, WORD xpos, WORD ypos)
{
    lockIBaseSave();

    // screen for popupmenu, check if it's a public screen
    struct Screen *popupscreen = screen;
    if ((popupscreen->Flags & (PUBLICSCREEN | WBENCHSCREEN)) == 0)
    {
	bug("DepthMenu: Frontmost screen isn't a public screen\n");
	return;
    }

    if (mode == DM_WINDOW)
    {
	(bug("DepthMenu: window\n"));
	struct Window *window = screen->FirstWindow;
	dmdata.entries = 0;
	while (window && (dmdata.entries < MAXENTRY))
	{
	    if (window->Title && (window->WScreen == screen))
	    {
		dmdata.mode = DM_WINDOW;
		dmdata.address[dmdata.entries] = window;
		strncpy(dmdata.title[dmdata.entries], window->Title, MAXSTRLEN);
		dmdata.title[dmdata.entries][MAXSTRLEN - 1] = 0;
		dmdata.entries++;
	    }
	    window = window->NextWindow;
	}
    }
    else
    {
	D(bug("DepthMenu: Screen\n"));
	dmdata.entries = 0;
	screen = screen->NextScreen; // Don't add frontmost screen, start with second screen
	while (screen && (dmdata.entries < MAXENTRY))
	{
	    if (screen->Title)
	    {
		dmdata.mode = DM_SCREEN;
		dmdata.address[dmdata.entries] = screen;
		strncpy(dmdata.title[dmdata.entries], screen->Title, MAXSTRLEN);
		dmdata.title[dmdata.entries][MAXSTRLEN - 1] = 0;
		dmdata.entries++;
	    }
	    screen = screen->NextScreen;
	}
    }

    if (dmdata.entries > 0)
    {
	// calculate max. text length
	LONG maxtextwidth = 0;
	LONG textwidth;
	int i;
	for (i=0 ; i < dmdata.entries ; i++)
	{
	    //FIXME is it correct to calculate TextLength with screen's RastPort?
	    textwidth = TextLength(&popupscreen->RastPort, dmdata.title[i], strlen(dmdata.title[i]));
	    if (textwidth > maxtextwidth) maxtextwidth = textwidth;
	}

	LONG width = maxtextwidth + 2 * BORDERWIDTH;
	LONG height = (dmdata.entries + 1) * popupscreen->RastPort.TxHeight + 2 * BORDERWIDTH;
	LONG left = xpos - width / 2;
	LONG top = ypos - height / 2;
	if (left < 0) left = 0;
	if (left + width > popupscreen->Width) left = popupscreen->Width - width;
	if (top < 0) top = 0;
	if (top + height > popupscreen->Height) top = popupscreen->Height - height;

	unlockIBaseSave();

	dmdata.popupwindow = OpenWindowTags
	    (
	     NULL,
	     WA_Left,       left,
	     WA_Top,        top,
	     WA_InnerWidth, width,
	     WA_InnerHeight,height,
	     WA_Borderless, TRUE,
	     WA_Activate,   TRUE,
	     WA_PubScreen,  popupscreen,
	     TAG_DONE
	    );
	if (dmdata.popupwindow)
	{
	    struct RastPort *rp = dmdata.popupwindow->RPort;
	    SetAPen(rp, 1);
	    int i;
	    for(i=0; i<dmdata.entries; i++)
	    {
		Move(rp, BORDERWIDTH, (i+1) * rp->TxHeight + BORDERWIDTH);
		Text(rp, dmdata.title[i], strlen(dmdata.title[i]));
		D(bug("DepthMenu: i %d title %s address %x\n", i, dmdata.title[i], dmdata.address[i]));
	    }
	}
	else
	{
	    bug("DepthMenu: Can't open popup window\n");
	}
    }
}


/************************************************************************************/
static void lockIBaseSave(void)
{
    if ( ! dmdata.locked)
    {
	dmdata.ibaselock = LockIBase(0);
	dmdata.locked = TRUE;
    }
}

/************************************************************************************/

static void unlockIBaseSave(void)
{
    if (dmdata.locked)
    {
	UnlockIBase(dmdata.ibaselock);
	dmdata.locked = FALSE;
    }
}

/************************************************************************************/

static void handleCx(DMState *cs)
{
    CxMsg *msg;
    BOOL   quit = FALSE;
    LONG   signals;

    while (!quit)
    {
	signals = Wait((1 << nb.nb_Port->mp_SigBit)  | SIGBREAKF_CTRL_C);

	if (signals & (1 << nb.nb_Port->mp_SigBit))
	{
	    while ((msg = (CxMsg *)GetMsg(cs->cs_msgPort)))
	    {
		switch (CxMsgType(msg))
		{
		    case CXM_COMMAND:
			switch (CxMsgID(msg))
			{
			    case CXCMD_DISABLE:
				ActivateCxObj(cs->cs_broker, FALSE);
				break;

			    case CXCMD_ENABLE:
				ActivateCxObj(cs->cs_broker, TRUE);
				break;

			    case CXCMD_UNIQUE:
				/* Running the program twice is the same as shutting
				   down the existing program... */
				/* Fall through */

			    case CXCMD_KILL:
				quit = TRUE;
				break;

			} /* switch (CxMsgID(msg)) */
			break;
		} /* switch (CxMsgType(msg))*/

		ReplyMsg((struct Message *)msg);

	    } /* while ((msg = (CxMsg *)GetMsg(cs->cs_msgPort))) */
	}	    

	if (signals & SIGBREAKF_CTRL_C)
	{
	    quit = TRUE;
	}

    } /* while(!quit) */
}

/************************************************************************************/

int main(int argc, char **argv)
{
    DMState cState;
    int     error = RETURN_OK;

    if (initiate(argc, argv, &cState))
    {
	handleCx(&cState);
    }
    else
    {
	error = RETURN_FAIL;
    }

    freeResources(&cState);

    return error;
}

/************************************************************************************/

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);

