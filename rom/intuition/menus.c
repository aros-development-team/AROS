/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>
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
#include "maybe_boopsi.h"
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

UWORD FindMenuShortCut(struct Menu *menu, UBYTE key, BOOL do_click_op,
		       struct IntuitionBase *IntuitionBase)
{
    struct MenuItem *item, *sub;     
    UWORD menunum, itemnum, subnum;
    BOOL found = FALSE;

    key = ToUpper(key);
    
    for(menunum = 0; menu; menu = menu->NextMenu, menunum ++)
    {
        for(item = menu->FirstItem, itemnum = 0; item; item = item->NextItem, itemnum++)
	{
	    if ((sub = item->SubItem))
	    {
	        for(subnum = 0; sub; sub = sub->NextItem, subnum++)
		{
		    if ((sub->Flags & COMMSEQ) &&
		        (ToUpper((UBYTE)sub->Command) == key))
		    {
		        if (do_click_op)
			{
			    if (sub->Flags & CHECKIT) CheckMenuItemWasClicked(sub, subnum, item);
			    sub->NextSelect = MENUNULL;
			}
		        found = TRUE;
			break;
		    }
		}
		if (found) break;
		
	    } else {
	        if ((item->Flags & COMMSEQ) &&
		    (ToUpper((UBYTE)item->Command) == key))
		{
		    if (do_click_op)
		    {
		        if (item->Flags & CHECKIT) CheckMenuItemWasClicked(item, itemnum, menu->FirstItem);
		        item->NextSelect = MENUNULL;
		    }
		    found = TRUE;
		    subnum = NOSUB;
		    break;
		}
	    }
	    
	} /* for each item */

	if (found) break;
	
    } /* for each menu */

    return found ? FULLMENUNUM(menunum, itemnum, subnum) : MENUNULL;
}

/**************************************************************************************************/

void CheckMenuItemWasClicked(struct MenuItem *item, UWORD itemnum, struct MenuItem *parentitem)
{
    /* Note: If you change something here, you probably must also change
             menutask.c/HandleCheckItem() which is used when the
	     user uses the menus with the mouse! */

    if (item->Flags & MENUTOGGLE)
    {
	item->Flags ^= CHECKED;
    } else {
	item->Flags |= CHECKED;
    }

    if (item->MutualExclude)
    {
	WORD i;

	for(i = 0; (i < 32) && parentitem; i++, parentitem = parentitem->NextItem)
	{
	    if ((i != itemnum) && (item->MutualExclude & (1L << i)) &&
		((parentitem->Flags & (CHECKED | CHECKIT)) == (CHECKIT | CHECKED)))
	    {
		parentitem->Flags &= ~CHECKED;
	    }
	}

    } /* if (item->MutualExclude) */
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
