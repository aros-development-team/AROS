#define AROS_ALMOST_COMPATIBLE 1 /* NEWLIST macro */
#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include "inputhandler.h"

#include "intuition_intern.h"
#include "menus.h"

/**************************************************************************************************/

struct MenuMessage *AllocMenuMessage(struct IntuitionBase *IntuitionBase)
{
    struct MenuMessage *msg;
    
    msg = AllocVec(sizeof(struct MenuMessage), MEMF_PUBLIC | MEMF_CLEAR);
    
    return msg;
}

/**************************************************************************************************/

void FreeMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase)
{
    if (msg) FreeVec(msg);
}


/**************************************************************************************************/

void SendMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase)
{
    PutMsg(GetPrivIBase(IntuitionBase)->MenuHandlerPort, &msg->msg);
}

/**************************************************************************************************/

struct MenuMessage *GetMenuMessage(struct MsgPort *port, struct IntuitionBase *IntuitionBase)
{
    return (struct MenuMessage *)GetMsg(port);
}


/**************************************************************************************************/

void ReplyMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase)
{
    FreeMenuMessage(msg, IntuitionBase);
}

/**************************************************************************************************/



void MH2Int_MakeMenusInactive(struct Window *win, UWORD menupick, struct IntuitionBase *IntuitionBase)
{
    struct InputEvent ie;
    struct IOStdReq ior;
    struct MsgPort replyport;
    
    ie.ie_NextEvent 	= 0;
    ie.ie_Class     	= IECLASS_MENU;
    ie.ie_SubClass  	= IESUBCLASS_MENUSTOP;
    ie.ie_Code	    	= menupick;
    ie.ie_EventAddress  = win;
    
    replyport.mp_Node.ln_Type = NT_MSGPORT;
    replyport.mp_Flags 	      = PA_SIGNAL;
    replyport.mp_SigBit       = SIGF_INTUITION;
    replyport.mp_SigTask      = FindTask(NULL);
    NEWLIST(&replyport.mp_MsgList);
    
    ior = *(GetPrivIBase(IntuitionBase)->InputIO);
    ior.io_Message.mn_ReplyPort = &replyport;
    
    ior.io_Command = IND_WRITEEVENT;
    ior.io_Data    = &ie;
    ior.io_Length  = sizeof(struct InputEvent);
    
    SetSignal(0, SIGF_INTUITION);    
    DoIO((struct IORequest *)&ior);
}

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

void GetMenuBox(struct Window *win, struct MenuItem *item,
	        WORD *xmin, WORD *ymin, WORD *xmax, WORD *ymax)
{

    WORD left, right, top, bottom;

    left  = top    =  0x7fff;
    right = bottom = -0x7fff;

    while(item != NULL)
    {
        left   = min(left,   item->LeftEdge);
	top    = min(top,    item->TopEdge);
	right  = max(right,  item->LeftEdge + item->Width  - 1);
	bottom = max(bottom, item->TopEdge  + item->Height - 1);
		
	item = item->NextItem;
    }

    if (xmin) *xmin = left   - win->WScreen->MenuHBorder;
    if (ymin) *ymin = top    - win->WScreen->MenuVBorder;
    if (xmax) *xmax = right  + win->WScreen->MenuHBorder;
    if (ymax) *ymax = bottom + win->WScreen->MenuVBorder;
    
}

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
