#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include <stdio.h>


/************************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define ARG_PRI   0
#define NUM_ARGS  1

struct IntuitionBase *IntuitionBase;
struct Library *LayersBase;
struct Library *CxBase;

static struct NewBroker nb =
{
   NB_VERSION,
   "Opaque", 
   "Opaque 0.01", 
   "Realtime moving of windows", 
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   0,
   NULL,                             
   0 
};

static struct MsgPort *cxport;
static struct Window *actionwin;
static struct Task *maintask;

static struct RDArgs *myargs;
static CxObj *cxbroker, *cxcust;
static ULONG cxmask, actionmask;
static WORD  winoffx, winoffy;
static UBYTE actionsig;
static BOOL quitme, disabled;

static LONG args[NUM_ARGS];
static char s[256];

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message *cxmsg;
    
    if (msg)
    {
	printf("Opaque: %s\n",msg);
    }

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
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    if (actionsig) FreeSignal(actionsig);
    
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
    maintask = FindTask(0);
    actionsig = AllocSignal(-1);
    actionmask = 1L << actionsig;
}

/************************************************************************************/

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	Cleanup("Can't open intuition.library V39!");
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

static void OpaqueAction(CxMsg *msg,CxObj *obj)
{
    static BOOL dragging = FALSE;
    
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);
    struct Screen *scr;
    
    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        switch(ie->ie_Code)
	{
	    case SELECTDOWN:
	    	if (IntuitionBase->ActiveWindow)
		{
		    scr = IntuitionBase->ActiveWindow->WScreen;
		} else {
		    scr = IntuitionBase->ActiveScreen;
		}
		
	        if (!dragging && scr)
		{
		    struct Layer *lay = WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);
		    struct Window *win = NULL;
		    
		    if (lay) win = (struct Window *)lay->Window;
		
		    if (win && !(ie->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)))
		    {
		        struct Gadget *gad;
			struct Window *newwin = NULL;
			
			for(gad = win->FirstGadget; gad; gad = gad->NextGadget)
			{
			    /* FIXME: does not handle app made dragging gadgets in
			       GZZ innerlayer or boopsi gadgets with special GM_HITTEST
			       method correctly! */
			       
			    if ((!(gad->Flags & GFLG_DISABLED)) &&
			        ((gad->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING))
			    {
				WORD x = gad->LeftEdge;
				WORD y = gad->TopEdge;
				WORD w = gad->Width;
				WORD h = gad->Height;
			    
			        if (gad->Flags & GFLG_RELRIGHT)  x += win->Width  - 1;
				if (gad->Flags & GFLG_RELBOTTOM) y += win->Height - 1;
				if (gad->Flags & GFLG_RELWIDTH)  w += win->Width;
				if (gad->Flags & GFLG_RELHEIGHT) h += win->Height;
				
				if ((win->MouseX >= x) &&
				    (win->MouseY >= y) &&
				    (win->MouseX < x + w) &&
				    (win->MouseY < y + h))
				{
				    /* found dragging gadget */
				    newwin = win;
				    break;
				}
			    }
			    
			} /* for(gad = win->FirstGadget; gad; gad = gad->NextGadget) */
			
			win = newwin;
			
		    } /* if (win && !(ie->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))) */
		    
		    if (win)
		    {				   
			dragging = TRUE;
			if (IntuitionBase->ActiveWindow != win) ActivateWindow(win);
			actionwin = win;
			winoffx = win->WScreen->MouseX - win->LeftEdge;
			winoffy = win->WScreen->MouseY - win->TopEdge;
			DisposeCxMsg(msg);
		    }
		    
		} /* if (!dragging && scr) */
		break;
		
	    case SELECTUP:
	        if (dragging)
		{
		    dragging = FALSE;
		    DisposeCxMsg(msg);
		}
		break;
		
	    case IECODE_NOBUTTON:
	        if (dragging)
		{
		    Signal(maintask, actionmask);
		}
		break;
		
	} /* switch(ie->ie_Code) */
	
    } /* if (ie->ie_Class == IECLASS_RAWMOUSE) */
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
    
    if (!(cxcust = CxCustom(OpaqueAction, 0)))
    {
        Cleanup("Can't create CxCustom object!\n");
    }
    
    AttachCxObj(cxbroker, cxcust);
    ActivateCxObj(cxbroker, 1);
    
}

/************************************************************************************/

static void HandleAction(void)
{
    WORD newx = actionwin->WScreen->MouseX - winoffx; 
    WORD newy = actionwin->WScreen->MouseY - winoffy;
    
    MoveWindow(actionwin, newx - actionwin->LeftEdge, newy - actionwin->TopEdge);
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

static void HandleAll(void)
{
    ULONG sigs;
    
    while(!quitme)
    {
        sigs = Wait(cxmask | actionmask | SIGBREAKF_CTRL_C);
	
	if (sigs & cxmask) HandleCx();
	if (sigs & actionmask) HandleAction();
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
    HandleAll();
    Cleanup(0);
    return 0;
}


/************************************************************************************/
