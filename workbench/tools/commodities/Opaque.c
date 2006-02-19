/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/commodities.h>
#include <proto/locale.h>
#include <proto/alib.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/icontrol.h>

#define  DEBUG 0
#include <aros/debug.h>

#include <stdio.h>
#include <stdlib.h>


#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "strings.h"

#define ACCELERATOR_THRESH      2
#define ACCELERATOR_MULTI       2

/************************************************************************************/

/* Using ChangeWindowBox has the advantage, that one can specify absolute window
   coords, instead of relative ones as in case of MoveWindow. OTOH it has the
   disadvantage that it also generates IDCMP_NEWSIZE IntuiMessages. */
   
#define USE_CHANGEWINDOWBOX 0

#define CALL_WINDOWFUNCS_IN_INPUTHANDLER 0

/************************************************************************************/

UBYTE version[] = "$VER: Opaque 0.3 (19.02.2006)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K"

#define ARG_PRI   0
#define NUM_ARGS  1

#define ACTIONTYPE_DRAGGING 1
#define ACTIONTYPE_RESIZING 2

#define SYSGADTYPE(gad) ((gad)->GadgetType & GTYP_SYSTYPEMASK)

/* Libraries to open */
struct LibTable
{
 APTR   lT_Library;
 STRPTR lT_Name;
 ULONG  lT_Version;
}
libTable[] =
{
 { &IntuitionBase,      "intuition.library",    39L},
 { &LayersBase,         "layers.library",       39L},
 { &CxBase,             "commodities.library",  39L},
 { NULL }
};

struct IntuitionBase *IntuitionBase = NULL;
struct Library *LayersBase = NULL;
struct Library *CxBase = NULL;

static struct NewBroker nb =
{
   NB_VERSION,
   NULL,
   NULL,
   NULL,
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   -120,
   NULL,                             
   0 
};

static struct Catalog *catalogPtr;
static struct MsgPort *cxport;
static struct Window *actionwin;
static struct Task *maintask;

static struct RDArgs *myargs;
static CxObj *cxbroker, *cxcust;
static ULONG cxmask, actionmask, icontrolmask;
static WORD  winoffx, winoffy, winwidth, winheight;
static LONG  newWindowX, newWindowY;
static LONG  resizeOffsetX, resizeOffsetY;
static LONG  trackMouseX, trackMouseY;
static LONG  mouseBottom, mouseRight;
static LONG  mouseTop, mouseLeft;
static LONG  gadgetLeft, gadgetTop;
static LONG  gadgetWidth, gadgetHeight;
static BYTE  actionsig, icontrolsig;
static UBYTE actiontype;
static BOOL quitme, disabled;
static BOOL offScreenLayersFlag;
static struct NotifyRequest *IControlChangeNR;

static LONG args[NUM_ARGS];
static char s[256];

static void HandleAction(void);
static void HandleIControl(void);

/**********************************************************************************************/

#define ARRAY_TO_LONG(x) ( ((x)[0] << 24UL) + ((x)[1] << 16UL) + ((x)[2] << 8UL) + ((x)[3]) )
#define ARRAY_TO_WORD(x) ( ((x)[0] << 8UL) + ((x)[1]) )

#define CONFIGNAME_ENV	    	"ENV:Sys/icontrol.prefs"

struct FileIControlPrefs
{
    UBYTE   ic_Reserved0[4];
    UBYTE   ic_Reserved1[4];
    UBYTE   ic_Reserved2[4];
    UBYTE   ic_Reserved3[4];
    UBYTE   ic_TimeOut[2];
    UBYTE   ic_MetaDrag[2];
    UBYTE   ic_Flags[4];
    UBYTE   ic_WBtoFront;
    UBYTE   ic_FrontToBack;
    UBYTE   ic_ReqTrue;
    UBYTE   ic_ReqFalse;
};

#define MINWINDOWWIDTH (5)
#define MINWINDOWHEIGHT (5)

void SetMouseBounds()
{
    struct Screen *scr;
    struct Layer *lay;
    struct Window *win;

    WORD minheight, minwidth, maxheight, maxwidth;

    if (IntuitionBase->ActiveWindow)
        scr = IntuitionBase->ActiveWindow->WScreen;
    else
        scr = IntuitionBase->ActiveScreen;

    lay = WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);

    if (lay)
        win = (struct Window *)lay->Window;
    else
        win = NULL;
		
    if (win) {
        if (actiontype == ACTIONTYPE_DRAGGING) {
            if (offScreenLayersFlag) {
                mouseLeft = 0; /* as left as you want */
                mouseTop = winoffy; /* keep the titlebar visible */
                mouseRight = win->WScreen->Width; /* as far right as you want */
                mouseBottom = win->WScreen->Height - (gadgetHeight - (winoffy + 1));
            }
            else { /* bounds such that the window never goes offscreen */
                mouseLeft = winoffx;
                mouseTop = winoffy;
                mouseRight = (win->WScreen->Width - winwidth) + winoffx;
                mouseBottom = (win->WScreen->Height - winheight) + winoffy;
            }
        }
        else {  /* actiontype == ACTIONTYPE_RESIZING) */
	    /* force legal min/max values */
            minwidth = win->MinWidth;
            maxwidth = win->MaxWidth; 
            minheight = win->MinHeight;
            maxheight = win->MaxHeight; 

	    if (maxwidth <= 0) maxwidth = win->WScreen->Width;
	    if (maxheight <= 0) maxheight = win->WScreen->Height;

	    if ((minwidth < MINWINDOWWIDTH) || (minheight < MINWINDOWHEIGHT) || /* if any dimention too small, or */
	     (minwidth > maxwidth) || (minheight > maxheight) || /* either min/max value pairs are inverted, or */
	     (minwidth > win->Width) || (minheight > win->Height) || /* the window is already smaller than minwidth/height, or */
	     (maxwidth < win->Width) || (maxheight < win->Height)) { /* the window is already bigger than maxwidth/height */
	        minwidth = MINWINDOWWIDTH; /* then put sane values in */
	        minheight = MINWINDOWHEIGHT;
                maxwidth = win->WScreen->Width;
                maxheight = win->WScreen->Height;
	    }
            
	    /* set new mouse bounds */
            mouseLeft = win->LeftEdge + minwidth - (win->Width - winoffx);
            mouseTop = win->TopEdge + minheight - (win->Height - winoffy);
            mouseRight = (win->LeftEdge + maxwidth) - (win->Width - winoffx);
            mouseBottom = (win->TopEdge + maxheight) - (win->Height - winoffy);
            if ((win->WScreen->Width - (win->Width - winoffx)) < mouseRight)
                mouseRight = (win->WScreen->Width - (win->Width - winoffx));
            if ((win->WScreen->Height - (win->Height - winoffy)) < mouseBottom) 
                mouseBottom = (win->WScreen->Height - (win->Height - winoffy));
        }
    }
}

BOOL GetOFFSCREENLAYERSPref()
{
    static struct FileIControlPrefs loadprefs;
    struct IFFHandle 	    	    *iff;    
    BOOL                      retval = TRUE;

    if ((iff = AllocIFF()))
    {
    	if ((iff->iff_Stream = (IPTR)Open(CONFIGNAME_ENV, MODE_OLDFILE)))
	{
	    InitIFFasDOS(iff);
	    
	    if (!OpenIFF(iff, IFFF_READ))
	    {
	    	if (!StopChunk(iff, ID_PREF, ID_ICTL))
		{
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
			struct ContextNode *cn;
			
			cn = CurrentChunk(iff);

			if (cn->cn_Size == sizeof(loadprefs))
			{
		    	    if (ReadChunkBytes(iff, &loadprefs, sizeof(loadprefs)) == sizeof(loadprefs))
			    {
				/*
    	    	    	    	icontrolprefs.ic_Reserved[0] = ARRAY_TO_LONG(loadprefs.ic_Reserved0);
    	    	    	    	icontrolprefs.ic_Reserved[1] = ARRAY_TO_LONG(loadprefs.ic_Reserved1);
    	    	    	    	icontrolprefs.ic_Reserved[2] = ARRAY_TO_LONG(loadprefs.ic_Reserved2);
    	    	    	    	icontrolprefs.ic_Reserved[3] = ARRAY_TO_LONG(loadprefs.ic_Reserved3);
				icontrolprefs.ic_TimeOut = ARRAY_TO_WORD(loadprefs.ic_TimeOut);
				icontrolprefs.ic_MetaDrag = ARRAY_TO_WORD(loadprefs.ic_MetaDrag);
				return icontrolprefs.ic_Flags = ARRAY_TO_LONG(loadprefs.ic_Flags);
				*/
				if ( ! (ARRAY_TO_LONG(loadprefs.ic_Flags) & ICF_OFFSCREENLAYERS) ) retval = FALSE;
				/*
				icontrolprefs.ic_WBtoFront = loadprefs.ic_WBtoFront;
				icontrolprefs.ic_FrontToBack = loadprefs.ic_FrontToBack;
				icontrolprefs.ic_ReqTrue = loadprefs.ic_ReqTrue;
				icontrolprefs.ic_ReqFalse = loadprefs.ic_ReqFalse;
				*/
				
				/*
				retval = TRUE;
				*/
			    }
			}
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!StopChunk(iff, ID_PREF, ID_INPT)) */
		
	    	CloseIFF(iff);
				
	    } /* if (!OpenIFF(iff, IFFF_READ)) */
	    
	    Close((BPTR)iff->iff_Stream);
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(CONFIGNAME_ENV, MODE_OLDFILE))) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */
    
    return retval;
}

/************************************************************************************/

STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
    STRPTR string;

    if(catalogPtr)
        string = (STRPTR)GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
    else
        string = CatCompArray[id].cca_Str;

    return(string);
}

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message *cxmsg;
    struct LibTable *tmpLibTable = libTable;
    
    if (msg)
    {
	printf("%s", msg);
    }

    if(CxBase)
    {
	if (cxbroker) DeleteCxObjAll(cxbroker);
	if (cxport)
	{
	    while((cxmsg = GetMsg(cxport)))
	    {
		ReplyMsg(cxmsg);
	    }

	    DeleteMsgPort(cxport);
	}
    }

    if (myargs) FreeArgs(myargs);

    if(LocaleBase)
    {
	CloseCatalog(catalogPtr);
	CloseLibrary((struct Library *)LocaleBase); /* Passing NULL is valid */
	D(bug("Closed locale.library!\n"));
    }
    
    while(tmpLibTable->lT_Name) /* Check for name rather than pointer */
    {
	if((*(struct Library **)tmpLibTable->lT_Library))
	{
	    CloseLibrary((*(struct Library **)tmpLibTable->lT_Library));
	    D(bug("Closed %s!\n", tmpLibTable->lT_Name));
	}

	tmpLibTable++;
    }

    if (actionsig) FreeSignal(actionsig);
    if (icontrolsig) FreeSignal(icontrolsig);

    if (IControlChangeNR != NULL) {
    	EndNotify(IControlChangeNR);
	FreeMem(IControlChangeNR, sizeof(struct NotifyRequest));
    }
    
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

    /* create "action" signal, to handle window dragging and resize events */
    maintask = FindTask(0);
    if((actionsig = AllocSignal(-1L)) != -1)
    {
    actionmask = 1L << actionsig;

    /* create "IControl pref changes" signal */
    if((icontrolsig = AllocSignal(-1L)) != -1)
    {
    icontrolmask = 1L << icontrolsig;
    if ((IControlChangeNR = AllocMem(sizeof(struct NotifyRequest), MEMF_CLEAR)))
        {
        IControlChangeNR->nr_Name = CONFIGNAME_ENV;
        IControlChangeNR->nr_Flags = NRF_SEND_SIGNAL;
        IControlChangeNR->nr_stuff.nr_Signal.nr_Task = maintask;
        IControlChangeNR->nr_stuff.nr_Signal.nr_SignalNum = icontrolsig;
    
        StartNotify(IControlChangeNR);
    
        /* set inital value for offscreenlayers */
        offScreenLayersFlag = GetOFFSCREENLAYERSPref();
        } else {
            printf("Not enough memory for NotifyRequest.\n");
        }
      }
    }
}

/************************************************************************************/

static void OpenLibs(void)
{
    struct LibTable *tmpLibTable = libTable;
    UBYTE tmpString[128]; /* petah: What if library name plus error message exceeds 128 bytes? */

    if((LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 40)))
    {
	catalogPtr = OpenCatalog
        (
            NULL, "System/Tools/Commodities.catalog",
            OC_BuiltInLanguage, (IPTR) "english",
            TAG_DONE
        );
    }
    else
	D(bug("Warning: Can't open locale.library V40!\n"));

    while(tmpLibTable->lT_Library)
    {
	if(!((*(struct Library **)tmpLibTable->lT_Library = OpenLibrary(tmpLibTable->lT_Name, tmpLibTable->lT_Version))))
        {
	    sprintf(tmpString, getCatalog(catalogPtr, MSG_CANT_OPEN_LIB), tmpLibTable->lT_Name, tmpLibTable->lT_Version);
	    Cleanup(tmpString);
        }
	else
	    D(bug("Library %s opened!\n", tmpLibTable->lT_Name));

	tmpLibTable++;
    }
}

#define ABS(x) (((x)<0)?(-(x)):(x))
inline WORD WITHACCEL(WORD raw) { if (ABS(raw) > ACCELERATOR_THRESH) return(raw << 2); else return(raw << 1);}
inline WORD WITHOUTACCEL(WORD raw) { if (ABS(raw) > ACCELERATOR_THRESH) return(raw >> 2); else return(raw >> 1);}

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
    static BOOL opaque_active = FALSE;

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
		
	        if (!opaque_active && scr)
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
			    /* FIXME: does not handle app made dragging/resize gadgets in
			       GZZ innerlayer or boopsi gadgets with special GM_HITTEST
			       method correctly! */
			       
			    if (!(gad->Flags & GFLG_DISABLED))
			    {
				WORD x = gad->LeftEdge;
				WORD y = gad->TopEdge;
				WORD w = gad->Width;
				WORD h = gad->Height;
				gadgetLeft = gad->LeftEdge;
				gadgetTop = gad->TopEdge;
				gadgetWidth = gad->Width;
				gadgetHeight = gad->Height;
			    
			        if (gad->Flags & GFLG_RELRIGHT)  x += win->Width  - 1;
				if (gad->Flags & GFLG_RELBOTTOM) y += win->Height - 1;
				if (gad->Flags & GFLG_RELWIDTH)  w += win->Width;
				if (gad->Flags & GFLG_RELHEIGHT) h += win->Height;
				
				if ((win->MouseX >= x) &&
				    (win->MouseY >= y) &&
				    (win->MouseX < x + w) &&
				    (win->MouseY < y + h))
				{
				    if ((SYSGADTYPE(gad) == GTYP_WDRAGGING) || (SYSGADTYPE(gad) == GTYP_SIZING))
    	    	    	    	    {
				    	/* found dragging or resize gadget */
				    	newwin = win;
				    	actiontype = (SYSGADTYPE(gad) == GTYP_WDRAGGING) ? ACTIONTYPE_DRAGGING :
				    	    	    	    	    	    	           ACTIONTYPE_RESIZING;
				    }
				    break;
				}
			    }
			    
			} /* for(gad = win->FirstGadget; gad; gad = gad->NextGadget) */
			
			win = newwin;
			
		    } /* if (win && !(ie->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))) */
		    
		    if (win)
		    {				   
			opaque_active = TRUE;
			if (IntuitionBase->ActiveWindow != win) ActivateWindow(win);
			actionwin = win;
			winoffx   = win->WScreen->MouseX - win->LeftEdge;
			winoffy   = win->WScreen->MouseY - win->TopEdge;
			winwidth  = win->Width;
			winheight = win->Height;
			/* set mouse boundaries appropriately */
			/* these need to be set initially to the window, they represent where Opaque
			thinks that the window should be, sometimes it takes the window some time to 
			actually move there however */
		        newWindowX = win->LeftEdge;
		        newWindowY = win->TopEdge;
			resizeOffsetX = winwidth - win->WScreen->MouseX;
			resizeOffsetY = winheight - win->WScreen->MouseY;
		        trackMouseX = actionwin->WScreen->MouseX;
		        trackMouseY = actionwin->WScreen->MouseY;
			DisposeCxMsg(msg);
			/* reset mouse bounds */
		        SetMouseBounds();
			//Signal(maintask, icontrolmask);
		    }
		    
		} /* if (!opaque_active && scr) */
		break;
		
	    case SELECTUP:
	        if (opaque_active)
		{
		    opaque_active = FALSE;
		    DisposeCxMsg(msg);
		}
		break;
		
	    case IECODE_NOBUTTON:
	        if (opaque_active)
		{ 
		    if (IEQUALIFIER_RELATIVEMOUSE & ie->ie_Qualifier) { /* relative */
		        trackMouseX = actionwin->WScreen->MouseX;
		        trackMouseY = actionwin->WScreen->MouseY;
		        WORD mouseshiftX = ie->ie_X;
		        WORD mouseshiftY = ie->ie_Y;
			WORD newX = trackMouseX + WITHACCEL(mouseshiftX);
			WORD newY = trackMouseY + WITHACCEL(mouseshiftY);

			/* predict if the mouse move will take the pointer "out-of-bounds", clip it */
		        if (newX < mouseLeft) {  /* mouse pointer will be out-of bounds x-wise to the left */
			    mouseshiftX = WITHOUTACCEL(mouseLeft - trackMouseX);
			    newX = trackMouseX + WITHACCEL(mouseshiftX);
			}
		        else {
			    if (newX > mouseRight) {  /* mouse pointer will be out-of bounds x-wise to the right */
			    	mouseshiftX = WITHOUTACCEL(mouseRight - trackMouseX);
			        newX = trackMouseX + WITHACCEL(mouseshiftX);
			    }
			}

			if (newY < mouseTop) {  /* mouse pointer will be out-of bounds y-wise to the top */
			    mouseshiftY = WITHOUTACCEL(mouseTop - trackMouseY);
			    newY = trackMouseY + WITHACCEL(mouseshiftY);
			}
		        else {
			    if (newY > mouseBottom) {  /* mouse pointer will be out-of bounds y-wise to the bottom */
			    	mouseshiftY = WITHOUTACCEL(mouseBottom - trackMouseY);
			        newY = trackMouseY + WITHACCEL(mouseshiftY);
			    }
			}

		        if ((mouseshiftX == 0) && (mouseshiftY == 0)) {
			    DisposeCxMsg(msg);
			}
			else {
			    /* new proposed window position */
    			    if (actiontype == ACTIONTYPE_DRAGGING) {
		            	newWindowX = newX - winoffx;
		            	newWindowY = newY - winoffy;
			    }
			    else {
		                newWindowX = newX + resizeOffsetX;
		                newWindowY = newY + resizeOffsetY;
			    }

			    /* new mouse relative coords */
			    ie->ie_X = mouseshiftX;
			    ie->ie_Y = mouseshiftY;

		            #if CALL_WINDOWFUNCS_IN_INPUTHANDLER
	    	                HandleAction();
		            #else
		                Signal(maintask, actionmask);
	    	            #endif
			}
		    }
		    else { /* absolute */
		        WORD mouseX = actionwin->WScreen->MouseX;
		        WORD mouseY = actionwin->WScreen->MouseY;
			WORD newX = ie->ie_X;
			WORD newY = ie->ie_Y;
			WORD mouseshiftX, mouseshiftY;

		        if (newX < mouseLeft) newX = mouseLeft;
		        else if (newX > mouseRight) newX = mouseRight;
		        if (newY < mouseTop) newY = mouseTop;
		        else if (newY > mouseBottom) newY = mouseBottom;

			/* reduce mouseshift if it goes too far */
			mouseshiftX = newX - mouseX;
			mouseshiftY = newY - mouseY;

		        if ((mouseshiftX == 0) && (mouseshiftY == 0)) {
			    DisposeCxMsg(msg);
			}
			else {
			    /* new proposed window position */
    			    if (actiontype == ACTIONTYPE_DRAGGING) {
		                newWindowX = newX - winoffx;
		                newWindowY = newY - winoffy;
			    }
			    else {
		                newWindowX = newX + resizeOffsetX;
		                newWindowY = newY + resizeOffsetY;
			    }
			    /* new mouse relative coords */
			    ie->ie_X = mouseshiftX;
			    ie->ie_Y = mouseshiftY;
		            #if CALL_WINDOWFUNCS_IN_INPUTHANDLER
	    	                HandleAction();
		            #else
		                Signal(maintask, actionmask);
	    	            #endif
			}
		    }
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
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_MSGPORT));
    }
    
    nb.nb_Port = cxport;
    
    cxmask = 1L << cxport->mp_SigBit;
    
    if (!(cxbroker = CxBroker(&nb, 0)))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_BROKER));
    }
    
    if (!(cxcust = CxCustom(OpaqueAction, 0)))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_CUSTOM));
    }
    
    AttachCxObj(cxbroker, cxcust);
    ActivateCxObj(cxbroker, 1);
    
}

/************************************************************************************/

/* Handle the case where the IControl Prefs have changed */
static void HandleIControl(void)
{
    D(bug("[Opaque] notified of icontrol.prefs change\n"));
    offScreenLayersFlag = GetOFFSCREENLAYERSPref();
    //SetMouseBounds();
}

/* Move window to absolute position newWindowX, newWindowY */
static void HandleAction(void)
{
    if (actiontype == ACTIONTYPE_DRAGGING)
    {
    	WORD newx = newWindowX; 
    	WORD newy = newWindowY;

    	/* MoveWindow(actionwin, newx - actionwin->LeftEdge, newy - actionwin->TopEdge); */
    #if USE_CHANGEWINDOWBOX
     	ChangeWindowBox(actionwin, newx, newy, actionwin->Width, actionwin->Height);
    #else
    	MoveWindow(actionwin, newx - actionwin->LeftEdge, newy - actionwin->TopEdge);
    #endif
    }
    else
    {
    	LONG neww = newWindowX;
    	LONG newh = newWindowY;
	
	if ((neww != actionwin->Width) || (newh != actionwin->Height))
	{
	    /* SizeWindow(actionwin, neww - actionwin->Width, newh - actionwin->Height); */
    	    ChangeWindowBox(actionwin, actionwin->LeftEdge, actionwin->TopEdge, neww, newh);
	}
    }
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
        sigs = Wait(cxmask | actionmask | icontrolmask | SIGBREAKF_CTRL_C);

	if (sigs & cxmask) HandleCx();
	if (sigs & actionmask) HandleAction(); /* "Action" == window moving or resizing */
	if (sigs & icontrolmask) HandleIControl();
	if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
    } /* while(!quitme) */
    
}

/************************************************************************************/

int main(void)
{
    Init();
    OpenLibs();

    nb.nb_Name = getCatalog(catalogPtr, MSG_OPAQUE_CXNAME);
    nb.nb_Title = getCatalog(catalogPtr, MSG_OPAQUE_CXTITLE);
    nb.nb_Descr = getCatalog(catalogPtr, MSG_OPAQUE_CXDESCR);

    GetArguments();
    InitCX();
    HandleAll();
    Cleanup(0);
    return 0;
}


/************************************************************************************/
