/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "strings.h"


/***************************************************************************/

UBYTE version[] = "$VER: NoCapsLock 1.0 (17.08.2003)";

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


/* Libraries to open */
struct LibTable
{
    APTR   lT_Library;
    STRPTR lT_Name;
    ULONG  lT_Version;
}
libTable[] =
{
    { &CxBase,             "commodities.library", 39L},
    { NULL,                NULL,                   0 }
};


struct Catalog       *catalogPtr;
struct Library       *CxBase = NULL;
struct Library       *IconBase = NULL;


static void freeResources(APState *as);
static BOOL initiate(int argc, char **argv, APState *as);
static void killCapsLockCustomFunc(CxMsg *msg, CxObj *co);
CONST_STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id);


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


static BOOL initiate(int argc, char **argv, APState *as)
{
    CxObj *customObj, *filterObj;
    struct LibTable *tmpLibTable = libTable;
    
    memset(as, 0, sizeof(APState));

    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 40);

    if (LocaleBase != NULL)
    {
	catalogPtr = OpenCatalog(NULL, "System/Tools/Commodities.catalog",
				 OC_BuiltInLanguage, (IPTR)"english", TAG_DONE);
	D(bug("Library locale.library opened!\n"));
    }
    else
    {
	D(bug("Warning: Can't open locale.library V40!\n"));
    }

    while (tmpLibTable->lT_Library != NULL)
    {
	*(struct Library **)tmpLibTable->lT_Library =
	    OpenLibrary(tmpLibTable->lT_Name, tmpLibTable->lT_Version);

	if (*(struct Library **)tmpLibTable->lT_Library == NULL)
	{
	    printf("%s %s %i\n", getCatalog(catalogPtr, MSG_CANT_OPEN_LIB),
		   tmpLibTable->lT_Name, (int)tmpLibTable->lT_Version);

	    return FALSE;
	}
        else
	{
	    D(bug("Library %s opened!\n", tmpLibTable->lT_Name));
	}
	
	tmpLibTable++;
    }

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
	IconBase = OpenLibrary("icon.library", 39);

	if (IconBase != NULL)
	{
	    UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);
	    
	    nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
	    ArgArrayDone();
	}
	else
	{
	    printf("%s %s %i\n", getCatalog(catalogPtr, MSG_CANT_OPEN_LIB),
		   "icon.library", 39);
	}

	CloseLibrary(IconBase);
    }
    
    nb.nb_Name = getCatalog(catalogPtr, MSG_NOCAPSLOCK_CXNAME);
    nb.nb_Title = getCatalog(catalogPtr, MSG_NOCAPSLOCK_CXTITLE);
    nb.nb_Descr = getCatalog(catalogPtr, MSG_NOCAPSLOCK_CXDESCR);

    as->as_msgPort = CreateMsgPort();

    if (as->as_msgPort == NULL)
    {
	printf(getCatalog(catalogPtr, MSG_CANT_CREATE_MSGPORT));

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
	printf(getCatalog(catalogPtr, MSG_CANT_CREATE_CUSTOM));

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
	printf(getCatalog(catalogPtr, MSG_CANT_CREATE_CUSTOM));

	return FALSE;
    }

    AttachCxObj(filterObj, customObj);
    ActivateCxObj(as->as_broker, TRUE);
    
    return TRUE;
}


static void freeResources(APState *as)
{
    struct Message  *cxm;
    struct LibTable *tmpLibTable = libTable;

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

    if (LocaleBase != NULL)
    {
	CloseCatalog(catalogPtr);
	CloseLibrary((struct Library *)LocaleBase); /* Passing NULL is valid */
	D(bug("Closed locale.library!\n"));
    }

    
    while (tmpLibTable->lT_Name != NULL) /* Check for name rather than pointer */
    {
	if (*(struct Library **)tmpLibTable->lT_Library != NULL)
	{
	    CloseLibrary((*(struct Library **)tmpLibTable->lT_Library));
	    D(bug("Closed %s!\n", tmpLibTable->lT_Name));
	}
	
	tmpLibTable++;
    }
}


/* Our CxCustom() function that is invoked everytime an imputevent is
   routed to our broker */
static void killCapsLockCustomFunc(CxMsg *msg, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    ie->ie_Qualifier &= ~IEQUALIFIER_CAPSLOCK;
}


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
