#ifndef MENUS_H
#define MENUS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Message structure the MenuHandler gets from Intuition.
*/

struct MenuMessage
{
    struct Message  	 msg;
    WORD            	 code;
    struct Window   	*win;
    struct InputEvent    ie;
};

#define MMCODE_START 	    1
#define MMCODE_EVENT 	    2
#define MMCODE_STARTCLOCK   4

#define MENUS_ACTIVE 	    (GetPrivIBase(IntuitionBase)->MenusActive)

#define IECLASS_MENU        IECLASS_EVENT
#define IESUBCLASS_MENUSTOP 21

struct MenuMessage *AllocMenuMessage(struct IntuitionBase *IntuitionBase);
void FreeMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase);
void SendMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase);
struct MenuMessage *GetMenuMessage(struct MsgPort *port, struct IntuitionBase *IntuitionBase);
void ReplyMenuMessage(struct MenuMessage *msg, struct IntuitionBase *IntuitionBase);

void MH2Int_MakeMenusInactive(struct Window *win, UWORD menupick, struct IntuitionBase *IntuitionBase);

void GetMenuBox(struct Window *win, struct MenuItem *item,
                WORD *xmin, WORD *ymin, WORD *xmax, WORD *ymax);

UWORD FindMenuShortCut(struct Menu *menu, UBYTE key, BOOL do_click_op,
                       struct IntuitionBase *IntuitionBase);
void CheckMenuItemWasClicked(struct MenuItem *item, UWORD itemnum, struct MenuItem *parentitem);

#endif /* MENUS_H */
