/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    AutoPoint commodity -- activates windows under the mouse pointer.
*/

/******************************************************************************

    NAME

        AutoPoint

    SYNOPSIS

        CX_PRIORITY/N/K, LAG/S

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Automatically activates the window under the mouse pointer.

    INPUTS

        CX_PRIORITY  --  The priority of the commodity

        LAG  --  Wait for the next timer event to activate the window

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    05.03.2000  SDuvan   implemented

******************************************************************************/

#include <aros/symbolsets.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/locale.h>
#include <proto/alib.h>

#include <stdio.h>
#include <string.h>

#define  DEBUG 0
#include <aros/debug.h>

#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/Tools/Commodities.catalog"
#define CATALOG_VERSION  2


/***************************************************************************/

UBYTE version[] = "$VER: AutoPoint 0.3 (15.04.2006)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K,LAG/S"

#define ARG_PRI   0
#define ARG_LAG   1
#define NUM_ARGS  2


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


typedef struct _APState
{
    CxObj          *as_broker;
    struct MsgPort *as_msgPort;
} APState;


typedef struct AP
{
    struct Window *ai_thisWindow;
    struct Window *ai_lastActivatedWindow;
    BOOL           ai_mouseHasMoved;
} AP;


static AP apInfo = 
{
    NULL,
    FALSE
};


static struct Catalog *catalog;

/************************************************************************************/

static void freeResources(APState *as);
static BOOL initiate(int argc, char **argv, APState *as);
static void autoActivateLag(CxMsg *msg, CxObj *co);
static void autoActivate(CxMsg *msg, CxObj *co);
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
    easyStruct.es_Title		= _(MSG_AUTOPOINT_CXNAME);
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

static BOOL initiate(int argc, char **argv, APState *as)
{
    CxObj *customObj;
    void (*activateFunc)(CxMsg *msg, CxObj *co);

    memset(as, 0, sizeof(APState));

    activateFunc = autoActivate;

    if (Cli() != NULL)
    {
	struct RDArgs *rda;
	IPTR          *args[] = { NULL, (IPTR)FALSE };

	rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);

	if (rda != NULL)
	{
	    if (args[ARG_PRI] != NULL)
	    {
		nb.nb_Pri = *args[ARG_PRI];
	    }
	    if (args[ARG_LAG])
	    {
		activateFunc = autoActivateLag;
	    }
	}
	FreeArgs(rda);
    }
    else
    {
	UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);

	nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);

	if (ArgString(array, "LAG", 0))
	{
	    activateFunc = autoActivateLag;
	}

	ArgArrayDone();
    }

    nb.nb_Name = _(MSG_AUTOPOINT_CXNAME);
    nb.nb_Title = _(MSG_AUTOPOINT_CXTITLE);
    nb.nb_Descr = _(MSG_AUTOPOINT_CXDESCR);

    as->as_msgPort = CreateMsgPort();

    if (as->as_msgPort == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_MSGPORT));

	return FALSE;
    }

    nb.nb_Port = as->as_msgPort;

    as->as_broker = CxBroker(&nb, 0);

    if (as->as_broker == NULL)
    {
	return FALSE;
    }

    customObj = CxCustom(activateFunc, 0);

    if (customObj == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_CUSTOM));

	return FALSE;
    }

    AttachCxObj(as->as_broker, customObj);
    ActivateCxObj(as->as_broker, TRUE);

    apInfo.ai_thisWindow = IntuitionBase->ActiveWindow;

    return TRUE;
}

/************************************************************************************/

static void freeResources(APState *as)
{
    struct Message  *cxm;

    if (CxBase != NULL)
    {
	if (as->as_broker != NULL)
	{
	    DeleteCxObjAll(as->as_broker);
	}
    }

    if (as->as_msgPort != NULL)
    {
	while ((cxm = GetMsg(as->as_msgPort)))
	{
	    ReplyMsg(cxm);
	}

	DeleteMsgPort(as->as_msgPort);
    }
}

/************************************************************************************/

/* Our CxCustom() function that is invoked everytime an imputevent is
   routed to our broker */
static void autoActivateLag(CxMsg *msg, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    if (ie->ie_Class == IECLASS_TIMER)
    {
	struct Screen *screen;
	struct Layer  *layer;

	if (IntuitionBase->ActiveWindow != NULL)
	{
	    screen = IntuitionBase->ActiveWindow->WScreen;
	}
	else
	{
	    screen = IntuitionBase->ActiveScreen;
	}

	layer = screen ? WhichLayer(&screen->LayerInfo, screen->MouseX, screen->MouseY) : NULL;

	apInfo.ai_thisWindow = (layer != NULL) ?
	    (struct Window *)layer->Window : NULL;

	if (apInfo.ai_mouseHasMoved ||
		(IntuitionBase->ActiveWindow == apInfo.ai_thisWindow) ||
		(apInfo.ai_lastActivatedWindow == apInfo.ai_thisWindow))
	{
	    apInfo.ai_mouseHasMoved = FALSE;
	    return;
	}

	/* Two timer events and mouse hasn't moved in between. */

	apInfo.ai_mouseHasMoved = FALSE;

	/* Should be possible to use ActivateWindow(NULL)? Else, we must
	   hack... */
	if (apInfo.ai_thisWindow != NULL)
	{
	    apInfo.ai_lastActivatedWindow = apInfo.ai_thisWindow;
	    ActivateWindow(apInfo.ai_thisWindow);
	}

	if (apInfo.ai_thisWindow != NULL)
	{
	    D(bug("Activated window %s\n", apInfo.ai_thisWindow->Title));
	}
	else
	{
	    D(bug("No window active. %s\n",
			IntuitionBase->ActiveWindow->Title));
	}

	return;
    }

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
	if (ie->ie_Code == IECODE_NOBUTTON)
	{
	    apInfo.ai_mouseHasMoved = TRUE;
	}
    }
}

/************************************************************************************/

static void autoActivate(CxMsg *msg, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
	struct Screen *screen;
	struct Layer  *layer;

	if (ie->ie_Code != IECODE_NOBUTTON)
	{
	    return;
	}

	if (IntuitionBase->ActiveWindow != NULL)
	{
	    screen = IntuitionBase->ActiveWindow->WScreen;
	}
	else
	{
	    screen = IntuitionBase->ActiveScreen;
	}

	layer = screen ? WhichLayer(&screen->LayerInfo, screen->MouseX, screen->MouseY) : NULL;

	apInfo.ai_thisWindow = (layer != NULL) ?
	    (struct Window *)layer->Window : NULL;

	if (apInfo.ai_thisWindow != NULL &&
		apInfo.ai_thisWindow != IntuitionBase->ActiveWindow)
	{
	    ActivateWindow(apInfo.ai_thisWindow);
	    D(bug("Activated window %s\n", apInfo.ai_thisWindow->Title));
	}

	return;
    }
}

/************************************************************************************/

/* React on command messages sent by commodities.library */
static void handleCx(APState *as)
{
    CxMsg *msg;
    BOOL   quit = FALSE;
    LONG   signals;

    while (!quit)
    {
	signals = Wait((1 << nb.nb_Port->mp_SigBit) | SIGBREAKF_CTRL_C);

	if (signals & (1 << nb.nb_Port->mp_SigBit))
	{
	    while ((msg = (CxMsg *)GetMsg(as->as_msgPort)))
	    {
		switch (CxMsgType(msg))
		{
		    case CXM_COMMAND:
			switch (CxMsgID(msg))
			{
			    case CXCMD_DISABLE:
				ActivateCxObj(as->as_broker, FALSE);
				break;

			    case CXCMD_ENABLE:
				ActivateCxObj(as->as_broker, TRUE);
				break;

			    case CXCMD_UNIQUE:
				/* Running the program twice <=> quit */
				/* Fall through */

			    case CXCMD_KILL:
				quit = TRUE;
				break;

			} /* switch(CxMsgID(msg)) */

			break;
		} /* switch (CxMsgType(msg))*/

		ReplyMsg((struct Message *)msg);

	    } /* while((msg = (CxMsg *)GetMsg(cs->cs_msgPort))) */
	}	    

	if (signals & SIGBREAKF_CTRL_C)
	{
	    quit = TRUE;
	}

    } /* while (!quit) */
}

/************************************************************************************/

int main(int argc, char **argv)
{
    APState aState;
    int     error = RETURN_OK;

    if (initiate(argc, argv, &aState))
    {
	handleCx(&aState);
    }
    else
    {
	error = RETURN_FAIL;
    }

    freeResources(&aState);

    return error;
}

/************************************************************************************/

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);

