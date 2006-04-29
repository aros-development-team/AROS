/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    ClickToFront commodity -- puts windows to front when clicked in.
*/

/******************************************************************************

    NAME

        ClickToFront

    SYNOPSIS

        CX_PRIORITY/N/K, QUALIFIER/K, DOUBLE/S

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Automatically raises and activates a window when clicking in it.

    INPUTS

        CX_PRIORITY  --  The priority of the commodity

	QUALIFIER    --  Qualifier to match the double click (LEFT_ALT,
	                 RIGHT_ALT, CTRL or NONE).

	DOUBLE       --  If specified, clicking means double-clicking.

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

UBYTE version[] = "$VER: ClickToFront 0.3 (15.04.2006)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K,QUALIFIER/K,DOUBLE/S"

#define  ARG_PRI        0
#define  ARG_QUALIFIER  1
#define  ARG_DOUBLE     2
#define  NUM_ARGS       3

struct Device         *InputBase = NULL;
struct Catalog        *catalog;
struct IOStdReq       *inputIO;


/* The ClickToFront broker */
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


typedef struct _CFState
{
    CxObj          *cs_broker;
    struct MsgPort *cs_msgPort;
} CFState;


typedef struct CF
{
    struct Window *ci_thisWindow;
    struct Window *ci_lastWindow;
    UWORD          ci_qualifiers;       /* Qualifiers that must match */
    BOOL           ci_mouseHasMoved;
    BOOL           ci_doubleClick;
    ULONG          ci_lcSeconds;        /* Time stamp for the last click */
    ULONG          ci_lcMicros;
} CF;


static CF cfInfo = 
{
    NULL,
    NULL,
    0,
    FALSE,
    FALSE,
    0,
    0
};

/************************************************************************************/

static void freeResources(CFState *cs);
static BOOL initiate(int argc, char **argv, CFState *cs);
static void getQualifier(STRPTR qualString);
static void clicktoFront(CxMsg *msg, CxObj *co);
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
    easyStruct.es_Title		= _(MSG_CLICK2FNT_CXNAME);
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

static BOOL initiate(int argc, char **argv, CFState *cs)
{
    CxObj *customObj;

    memset(cs, 0, sizeof(CFState));

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

	    getQualifier((STRPTR)args[ARG_QUALIFIER]);

	    cfInfo.ci_doubleClick = args[ARG_DOUBLE];

	    if (cfInfo.ci_doubleClick)
	    {
		D(bug("Using the double clicking method.\n"));
	    }
	}

	FreeArgs(rda);
    }
    else
    {
	UBYTE  **array = ArgArrayInit(argc, (UBYTE **)argv);

	nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
	cfInfo.ci_doubleClick = ArgString(array, "DOUBLE", NULL) != NULL;

	getQualifier(ArgString(array, "QUALIFIER", NULL));

	ArgArrayDone();
    }

    nb.nb_Name = _(MSG_CLICK2FNT_CXNAME);
    nb.nb_Title = _(MSG_CLICK2FNT_CXTITLE);
    nb.nb_Descr = _(MSG_CLICK2FNT_CXDESCR);

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

    customObj = CxCustom(clicktoFront, 0);

    if (customObj == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_MSGPORT));

	return FALSE;
    }

    AttachCxObj(cs->cs_broker, customObj);
    ActivateCxObj(cs->cs_broker, TRUE);

    cfInfo.ci_thisWindow = IntuitionBase->ActiveWindow;

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

static void getQualifier(STRPTR qualString)
{
    if (qualString == NULL)
    {
	return;
    }

    if (strcmp("CTRL", qualString) == 0)
    {
	cfInfo.ci_qualifiers = IEQUALIFIER_CONTROL;
    }

    if (strcmp("LEFT_ALT", qualString) == 0)
    {
	cfInfo.ci_qualifiers = IEQUALIFIER_LALT;
    }

    if (strcmp("RIGHT_ALT", qualString) == 0)
    {
	cfInfo.ci_qualifiers = IEQUALIFIER_RALT;
    }

    /* Default is NONE */
}

/************************************************************************************/

static void freeResources(CFState *cs)
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

/* Currently we use single click, not double click... */
static void clicktoFront(CxMsg *cxm, CxObj *co)
{
    /* NOTE! Should use arbitration for IntuitionBase... */

    struct InputEvent *ie = (struct InputEvent *)CxMsgData(cxm);

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
	if (ie->ie_Code == SELECTDOWN)
	{
	    struct Screen *screen;
	    struct Layer  *layer;

	    /* Mask relvant qualifiers (key qualifiers) */
	    if ((PeekQualifier() & 0xff) != cfInfo.ci_qualifiers)
	    {
		D(bug("Qualifiers: %i, Wanted qualifiers: %i\n",
			    (int)PeekQualifier(),
			    (int)cfInfo.ci_qualifiers | IEQUALIFIER_LEFTBUTTON));

		return;
	    }

	    cfInfo.ci_lastWindow = cfInfo.ci_thisWindow;

	    if (IntuitionBase->ActiveWindow != NULL)
	    {
		screen = IntuitionBase->ActiveWindow->WScreen;
	    }
	    else
	    {
		screen = IntuitionBase->ActiveScreen;
	    }

	    layer = WhichLayer(&screen->LayerInfo,
		    screen->MouseX, screen->MouseY);

	    if (layer == NULL)
	    {
		return;
	    }

	    cfInfo.ci_thisWindow = (layer != NULL) ?
		(struct Window *)layer->Window : NULL;

	    /* Error: IB->ActiveWindow is non-NULL even if there is no
	       active window! */
	    if (layer->front != NULL)
	    {
		if (cfInfo.ci_doubleClick)
		{
		    if (!DoubleClick(cfInfo.ci_lcSeconds, cfInfo.ci_lcMicros,
				ie->ie_TimeStamp.tv_secs,
				ie->ie_TimeStamp.tv_micro))
		    {
			cfInfo.ci_lcSeconds = ie->ie_TimeStamp.tv_secs;
			cfInfo.ci_lcMicros  = ie->ie_TimeStamp.tv_micro;

			return;
		    }

		    D(bug("Time %i %i, last time %i %i\n",
				ie->ie_TimeStamp.tv_secs,
				ie->ie_TimeStamp.tv_micro,
				cfInfo.ci_lcSeconds,
				cfInfo.ci_lcMicros));

		    cfInfo.ci_lcSeconds = ie->ie_TimeStamp.tv_secs;
		    cfInfo.ci_lcMicros  = ie->ie_TimeStamp.tv_micro;

		    /* Both clicks better have been in the same window */
		    if (cfInfo.ci_lastWindow != cfInfo.ci_thisWindow)
		    {
			return;
		    }
		}

		WindowToFront(cfInfo.ci_thisWindow);

		if (cfInfo.ci_thisWindow != IntuitionBase->ActiveWindow)
		{
		    ActivateWindow(cfInfo.ci_thisWindow);
		}

		//DisposeCxMsg(cxm);

		D(bug("Put window %s to front.\n",
			    cfInfo.ci_thisWindow->Title));
	    }
	    else
	    {
		D(bug("New: %p Old: %p\n", cfInfo.ci_thisWindow,
			    IntuitionBase->ActiveWindow));
	    }
	}
    }
}

/************************************************************************************/

static void handleCx(CFState *cs)
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
    CFState cState;
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

