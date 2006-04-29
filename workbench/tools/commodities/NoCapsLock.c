/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    NoCapsLock commodity -- Renders the CAPS LOCK key ineffective
*/

/******************************************************************************

    NAME

        NoCapsLock

    SYNOPSIS

        CX_PRIORITY/N/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Renders the CAPS LOCK key ineffective.

    INPUTS

        CX_PRIORITY  --  The priority of the commodity

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

UBYTE version[] = "$VER: NoCapsLock 1.1 (15.04.2006)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define ARG_PRI   0
#define NUM_ARGS  1


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


static struct Catalog       *catalog;

/***************************************************************************/

static void freeResources(APState *as);
static BOOL initiate(int argc, char **argv, APState *as);
static void killCapsLockCustomFunc(CxMsg *msg, CxObj *co);
static CONST_STRPTR _(ULONG id);
static BOOL Locale_Initialize(VOID);
static VOID Locale_Deinitialize(VOID);
static void showSimpleMessage(CONST_STRPTR msgString);

/***************************************************************************/

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

/***************************************************************************/

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
    easyStruct.es_Title		= _(MSG_NOCAPSLOCK_CXNAME);
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

/***************************************************************************/

static BOOL initiate(int argc, char **argv, APState *as)
{
    CxObj *customObj, *filterObj;

    memset(as, 0, sizeof(APState));

    if (Cli() != NULL)
    {
	struct RDArgs *rda;
	IPTR          *args[] = { NULL };

	rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);

	if (rda != NULL)
	{
	    if (args[ARG_PRI] != NULL)
	    {
		nb.nb_Pri = (BYTE)(*args[ARG_PRI]);
	    }
	}

	FreeArgs(rda);
    }
    else
    {
	UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);

	nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
	ArgArrayDone();
    }

    nb.nb_Name = _(MSG_NOCAPSLOCK_CXNAME);
    nb.nb_Title = _(MSG_NOCAPSLOCK_CXTITLE);
    nb.nb_Descr = _(MSG_NOCAPSLOCK_CXDESCR);

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

    filterObj = CxFilter(NULL);
    if (filterObj == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_CUSTOM));

	return FALSE;
    }
    else
    {
	static IX ix =
	{
	    IX_VERSION,
	    IECLASS_RAWKEY,
	    0,
	    0,
	    IEQUALIFIER_CAPSLOCK,
	    IEQUALIFIER_CAPSLOCK,
	    0
	};

	SetFilterIX(filterObj, &ix);
	AttachCxObj(as->as_broker, filterObj);
    }


    customObj = CxCustom(killCapsLockCustomFunc, 0);

    if (customObj == NULL)
    {
	showSimpleMessage(_(MSG_CANT_CREATE_CUSTOM));

	return FALSE;
    }

    AttachCxObj(filterObj, customObj);
    ActivateCxObj(as->as_broker, TRUE);

    return TRUE;
}

/***************************************************************************/

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

/***************************************************************************/

/* Our CxCustom() function that is invoked everytime an imputevent is
   routed to our broker */
static void killCapsLockCustomFunc(CxMsg *msg, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    ie->ie_Qualifier &= ~IEQUALIFIER_CAPSLOCK;
}

/***************************************************************************/

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

/***************************************************************************/

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

/***************************************************************************/

ADD2INIT(Locale_Initialize,   90);
ADD2EXIT(Locale_Deinitialize, 90);

