/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#define MENUTASK_NAME "« Menu Handler »"
#define MENUTASK_STACKSIZE AROS_STACKSIZE
#define MENUTASK_PRIORITY 0

#define ITEM_ITEM    1
#define ITEM_SUBITEM 2

#define AMIGAKEY_KEY_SPACING     4 /* GadTools assumes this, too */
#define AMIGAKEY_BORDER_SPACING  2

/* Structure passed to the DefaultMenuHandler task when it's initialized */

struct MenuTaskParams
{
    struct IntuitionBase 	*IntuitionBase;
    struct Task			*Caller;
    struct MsgPort		*MenuHandlerPort; /* filled in by MenuHandler task */
    BOOL			success;
};

struct MenuHandlerData
{
    struct Window 	*win;
    struct Screen 	*scr;
    struct DrawInfo	*dri;
    struct Window	*menubarwin;
    struct Window 	*menuwin;
    struct Window	*submenuwin;
    struct Menu		*menu;
    struct Menu		*activemenu;
    struct MenuItem	*activeitem;
    struct MenuItem	*activesubitem;
    struct Rectangle	submenubox;
    struct Image	*checkmark;
    struct Image	*amigakey;
    WORD		menubarwidth;
    WORD		menubarheight;
    WORD		menubaritemwidth;
    WORD		menubaritemheight;
    WORD		nummenubaritems;
    WORD		activemenunum;
    WORD		activeitemnum;
    WORD		activesubitemnum;
    WORD		maxcommkeywidth_menu;
    WORD		maxcommkeywidth_submenu;
    WORD		scrmousex;
    WORD		scrmousey;
    UWORD		firstmenupick;
    UWORD		lastmenupick;
    BOOL 		active;
};

BOOL InitDefaultMenuHandler(struct IntuitionBase *IntuitionBase);
struct Task *CreateMenuHandlerTask(APTR taskparams, struct IntuitionBase *IntuitionBase);
void DefaultMenuHandler(struct MenuTaskParams *taskparams);

