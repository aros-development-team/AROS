/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: AutoPoint commodity -- activates windows under the mouse pointer
    Lang: English
*/

/******************************************************************************

    NAME

        AutoPoint

    SYNOPSIS

        CX_PRIORITY/N/K

    LOCATION

        Workbench:Tools/Commodities

    FUNCTION

        Automatically activates the window under the mouse pointer.

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
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/alib.h>

#define  DEBUG 0
#include <aros/debug.h>


/***************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define ARG_PRI   0
#define NUM_ARGS  1


struct IntuitionBase *IntuitionBase;
struct Library       *LayersBase;
struct Library       *CxBase;
struct Library       *IconBase;


static struct NewBroker nb =
{
    NB_VERSION,
    "AutoPoint", 
    "Automatic window activator", 
    "Activates the window under the mouse",
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
    BOOL           ai_mouseHasMoved;
} AP;


static AP apInfo = 
{
    NULL,
    FALSE
};


static void freeResources(APState *as);
static BOOL initiate(int argc, char **argv, APState *as);
static void autoActivate(CxMsg *msg, CxObj *co);


static BOOL initiate(int argc, char **argv, APState *as)
{
    CxObj *customObj;

    memset(as, 0, sizeof(APState));

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",
							39);
    if(IntuitionBase == NULL)
	return FALSE;

    LayersBase = OpenLibrary("layers.library", 39);
    if(LayersBase == NULL)
	return FALSE;
    
    CxBase = OpenLibrary("commodities.library", 39);
    if(CxBase == NULL)
	return FALSE;


    as->as_msgPort = CreateMsgPort();
    if(as->as_msgPort == NULL)
	return FALSE;
    
    nb.nb_Port = as->as_msgPort;
    
    as->as_broker = CxBroker(&nb, 0);
    if(as->as_broker == NULL)
	return FALSE;
    
    customObj = CxCustom(autoActivate, 0);
    if(customObj == NULL)
	return FALSE;

    AttachCxObj(as->as_broker, customObj);
    ActivateCxObj(as->as_broker, TRUE);

    apInfo.ai_thisWindow = IntuitionBase->ActiveWindow;

    if(Cli() != NULL)
    {
	struct RDArgs *rda;
	IPTR          *args[] = { NULL };

	rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);

	if(rda != NULL)
	{
	    if(args[0] != NULL)
		nb.nb_Pri = *args[ARG_PRI];
	}

	FreeArgs(rda);
    }
    else
    {
	IconBase = OpenLibrary("icon.library", 39);

	if(IconBase != NULL)
	{
	    UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);

	    nb.nb_Pri = ArgInt(array, "CX_PRIORITY", 0);
	    ArgArrayDone();
	}
	
	CloseLibrary(IconBase);
    }

    return TRUE;
}


static void freeResources(APState *as)
{
    struct Message *cxm;
    
    if(as->as_broker != NULL)
	DeleteCxObjAll(as->as_broker);

    if(as->as_msgPort != NULL)
    {
        while((cxm = GetMsg(as->as_msgPort)))
	    ReplyMsg(cxm);

        DeleteMsgPort(as->as_msgPort);
    }
    

    if(IntuitionBase != NULL)
	CloseLibrary((struct Library *)IntuitionBase);

    if(LayersBase != NULL)
	CloseLibrary(LayersBase);

    if(CxBase != NULL)
	CloseLibrary(CxBase);
}


static void autoActivate(CxMsg *msg, CxObj *co)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);

    if(ie->ie_Class == IECLASS_TIMER)
    {
	struct Screen *screen;
	struct Layer  *layer;

	if(IntuitionBase->ActiveWindow != NULL)
	    screen = IntuitionBase->ActiveWindow->WScreen;
	else
	    screen = IntuitionBase->ActiveScreen;
	
	layer = WhichLayer(&screen->LayerInfo, screen->MouseX, screen->MouseY);
	
	apInfo.ai_thisWindow = (layer != NULL) ?
	    (struct Window *)layer->Window : NULL;

	if(apInfo.ai_mouseHasMoved ||
	   IntuitionBase->ActiveWindow == apInfo.ai_thisWindow)
	{
	    apInfo.ai_mouseHasMoved = FALSE;
	    return;
	}

	/* Two timer events and mouse hasn't moved in between. */

	apInfo.ai_mouseHasMoved = FALSE;

	/* Should be possible to use ActivateWindow(NULL)? Else, we must
	   hack... */
	if(apInfo.ai_thisWindow != NULL)
	    ActivateWindow(apInfo.ai_thisWindow);

	if(apInfo.ai_thisWindow != NULL)
	    D(bug("Activated window %s\n", apInfo.ai_thisWindow->Title));
	else
	    D(bug("No window active. %s\n",
		  IntuitionBase->ActiveWindow->Title));
	
	return;
    }

    if(ie->ie_Class == IECLASS_RAWMOUSE)
    {
	if(ie->ie_Code == IECODE_NOBUTTON)
	    apInfo.ai_mouseHasMoved = TRUE;
    }
}


static void handleCx(APState *as)
{
    CxMsg *msg;
    BOOL   quit = FALSE;
    LONG   signals;
        
    while(!quit)
    {
	signals = Wait((1 << nb.nb_Port->mp_SigBit) | SIGBREAKF_CTRL_C);
	
	if(signals & (1 << nb.nb_Port->mp_SigBit))
	{
	    while((msg = (CxMsg *)GetMsg(as->as_msgPort)))
	    {
		switch(CxMsgType(msg))
		{
		case CXM_COMMAND:
		    switch(CxMsgID(msg))
		    {
		    case CXCMD_DISABLE:
			ActivateCxObj(as->as_broker, FALSE);
			break;
			
		    case CXCMD_ENABLE:
			ActivateCxObj(as->as_broker, TRUE);
			break;
			
		    case CXCMD_KILL:
			quit = TRUE;
			break;
			
		    } /* switch(CxMsgID(msg)) */
		    break;
		} /* switch (CxMsgType(msg))*/
		
		ReplyMsg((struct Message *)msg);
		
	    } /* while((msg = (CxMsg *)GetMsg(cs->cs_msgPort))) */
	}	    
	
	if(signals & SIGBREAKF_CTRL_C)
	    quit = TRUE;
	
    } /* while(!quit) */
}


int main(int argc, char **argv)
{
    APState aState;
    int     error = RETURN_OK;

    if(initiate(argc, argv, &aState))
    {
	handleCx(&aState);
    }
    else
	error = RETURN_FAIL;
    
    freeResources(&aState);

    return error;
}
