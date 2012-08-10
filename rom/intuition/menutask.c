/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/imageclass.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>
#include "inputhandler.h"
#include "intuition_intern.h"
#include <string.h>

#ifdef SKINS
#include "intuition_customize.h"
#endif

#include "menus.h"
#include "menutask.h"
#include <cybergraphx/cybergraphics.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

extern IPTR HookEntry();

/**************************************************************************************************/

/* this #defines are taken from workbench/libs/gadtools/menus.c!! */

#define TEXT_AMIGAKEY_SPACING  6

#define ITEXT_EXTRA_LEFT   2
#define ITEXT_EXTRA_RIGHT  2
#define ITEXT_EXTRA_TOP    1
#define ITEXT_EXTRA_BOTTOM 1

/**************************************************************************************************/

static void HandleMouseMove(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void HandleMouseClick(struct InputEvent *ie, struct MenuHandlerData *mhd,
                             struct IntuitionBase *IntuitionBase);
static void HandleCheckItem(struct Window *win, struct MenuItem *item,
                            struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);

static void HighlightMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd,
                               struct IntuitionBase *IntuitionBase);

static struct Menu *FindMenu(WORD *var, struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static struct MenuItem *FindItem(WORD *var, struct MenuHandlerData *mhd);
static struct MenuItem *FindSubItem(WORD *var, struct MenuHandlerData *mhd);

static void MakeMenuBarWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void KillMenuBarWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void RenderMenuBar(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);

static void MakeMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void KillMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void RenderMenu(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void RenderMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd,
                            struct IntuitionBase *IntuitionBase);

static void MakeSubMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void KillSubMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void RenderSubMenu(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);

static void RenderItem(struct MenuItem *item, WORD itemtype,  struct Rectangle *box,
                       struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);

static void RenderMenuBG(struct Window *win, struct MenuHandlerData *mhd,
                         struct IntuitionBase *IntuitionBase);
static void RenderCheckMark(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                            struct IntuitionBase *IntuitionBase);
static void RenderAmigaKey(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                           struct IntuitionBase *IntuitionBase);
static void RenderDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
                                  struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void RenderFrame(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD state,
                        struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
static void HighlightItem(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                          struct IntuitionBase *IntuitionBase);
static WORD CalcMaxCommKeyWidth(struct Window *win, struct MenuHandlerData *mhd,
                                struct IntuitionBase *IntuitionBase);
static void AddToSelection(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);

/**************************************************************************************************/

/***************************
**  DefaultMenuHandler()  **
***************************/
void DefaultMenuHandler(struct MenuTaskParams *taskparams)
{
    struct IntuitionBase    *IntuitionBase = taskparams->intuitionBase;
    struct GfxBase          *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct MenuHandlerData  *mhd = NULL;
    UBYTE                   *mem;
    struct MsgPort          *port = NULL;
    BOOL                     success = FALSE;

    if ((mem = AllocMem(sizeof(struct MsgPort) +
                        sizeof(struct MenuHandlerData), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        port = (struct MsgPort *)mem;

        port->mp_Node.ln_Type   = NT_MSGPORT;
        port->mp_Flags          = PA_SIGNAL;
        port->mp_SigBit         = AllocSignal(-1);
        port->mp_SigTask        = FindTask(0);
        NEWLIST(&port->mp_MsgList);

        mhd = (struct MenuHandlerData *)(mem + sizeof(struct MsgPort));

        success = TRUE;

    } /* if ((mem = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR))) */

    if (success)
    {
        taskparams->MenuHandlerPort = port;
        taskparams->success = TRUE;
    }

    Signal(taskparams->Caller, SIGF_INTUITION);

    if (!success)
    {
        D(bug("DefaultMenuHandler: initialization failed. waiting for parent task to kill me.\n"));
        Wait(0);
    }

    D(bug("DefaultMenuHandler: initialization ok. Now waiting for messages from Intuition.\n"));

    for(;;)
    {
        struct MenuMessage *msg;

        WaitPort(port);
        while((msg = GetMenuMessage(port, IntuitionBase)))
        {
            switch(msg->code)
            {
            case MMCODE_START:
                mhd->win                = msg->win;
                mhd->scr                = mhd->win->WScreen;
                mhd->dri                = GetScreenDrawInfo(mhd->scr);
                mhd->menu               = msg->win->MenuStrip;
                mhd->activemenunum      = -1;
                mhd->activeitemnum      = -1;
                mhd->activesubitemnum   = -1;
                mhd->checkmark          = ((struct IntWindow *)mhd->win)->Checkmark;
                mhd->amigakey           = ((struct IntWindow *)mhd->win)->AmigaKey;
                mhd->submenuimage          = ((struct IntWindow *)mhd->win)->SubMenuImage;
                mhd->scrmousex          = mhd->scr->MouseX;
                mhd->scrmousey          = mhd->scr->MouseY;
                mhd->firstmenupick      = MENUNULL;
        mhd->keepmenuup         = TRUE;
                mhd->TrueColor          = GetBitMapAttr(&mhd->scr->BitMap, BMA_DEPTH) > 8 ? TRUE: FALSE;

                /* close windows in the back first because
                   this is faster */
                MakeMenuBarWin(mhd, IntuitionBase);
                HandleMouseMove(mhd, IntuitionBase);
                mhd->active = TRUE;
                break;

            case MMCODE_EVENT:
                /* there might come additional messages from Intuition
                   even when we have already told it to make the menus
                   inactive, but since everything is async, this cannot
                   be avoided, so check if we are really active */

                if (mhd->active)
                {
                    switch(msg->ie.ie_Class)
                    {
                        case IECLASS_RAWMOUSE:
                            if (msg->ie.ie_Code == IECODE_NOBUTTON)
                            {   
                                HandleMouseMove(mhd, IntuitionBase);
                            }
                            else
                            {
                                HandleMouseClick(&msg->ie, mhd, IntuitionBase);
                            }
                            break;
                    }

                } /* if (mhd->active) */
                break;

            } /* switch(msg->code) */

            ReplyMenuMessage(msg, IntuitionBase);

        } /* while((msg = (struct MenuMessage *)GetMsg(port))) */

    } /* for(;;) */
}

/**************************************************************************************************/

/*******************************
**  InitDefaultMenuHandler()  **
*******************************/
BOOL InitDefaultMenuHandler(struct IntuitionBase *IntuitionBase)
{
    struct MenuTaskParams   params;
    struct Task            *task;
    BOOL                    result = FALSE;

    params.intuitionBase = IntuitionBase;
    params.Caller    = FindTask(NULL);
    params.success   = FALSE;

    SetSignal(0, SIGF_INTUITION);

    task = NewCreateTask(TASKTAG_NAME, "Intuition menu handler",
                 TASKTAG_PC  , DefaultMenuHandler,
                 TASKTAG_ARG1, &params,
                 TAG_DONE);

    if (task)
    {
        Wait(SIGF_INTUITION);

        if (params.success)
        {
            result = TRUE;
            GetPrivIBase(IntuitionBase)->MenuHandlerPort = params.MenuHandlerPort;
        }
        else
        {
            RemTask(task);
        }

    } /* if ((task = CreateMenuHandlerTask(&params, IntuitionBase))) */

    return result;
}


/**************************************************************************************************/

static void HandleMouseMove(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Layer    *lay;
    struct Window   *win = NULL;
    struct Menu     *menu;
    struct MenuItem *item;
    WORD             new_activemenunum = mhd->activemenunum;
    WORD             new_activeitemnum = mhd->activeitemnum;
    WORD             new_activesubitemnum = mhd->activesubitemnum;

    mhd->scrmousex = mhd->scr->MouseX;
    mhd->scrmousey = mhd->scr->MouseY;

    if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey)))
    {
        win = (struct Window *)lay->Window;

        if (win && (win == mhd->submenuwin))
        {
            /* Mouse over submenu box */
            item = FindSubItem(&new_activesubitemnum, mhd);

            if (new_activesubitemnum != mhd->activesubitemnum)
            {
                if (mhd->activesubitemnum != -1)
                {
                    HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd, IntuitionBase);
                }

                mhd->activesubitemnum = new_activesubitemnum;
                mhd->activesubitem = item;

                if (item)
                {
                    HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd, IntuitionBase);
                }
            }

        }
        else if (win && (win == mhd->menuwin))
        {
            item = FindItem(&new_activeitemnum, mhd);

            if (new_activeitemnum != mhd->activeitemnum)
            {
                if (mhd->activeitemnum != -1)
                {
                    HighlightItem(mhd->activeitem, ITEM_ITEM, mhd, IntuitionBase);
                    KillSubMenuWin(mhd, IntuitionBase);
                }

                mhd->activeitemnum = new_activeitemnum;
                mhd->activeitem = item;

                if (item)
                {
                    HighlightItem(mhd->activeitem, ITEM_ITEM, mhd, IntuitionBase);

                    if (item->SubItem)
                    {
                        MakeSubMenuWin(mhd, IntuitionBase);
                    }
                }
            }
        } /* if (win && (win == mhd->menuwin)) */
        else if (win && (win == mhd->menubarwin))
        {
            /* Mouse over menu box */

            menu = FindMenu(&new_activemenunum, mhd, IntuitionBase);

            if (new_activemenunum != mhd->activemenunum)
            {

                if (mhd->activemenunum != -1)
                {
                    HighlightMenuTitle(mhd->activemenu, mhd, IntuitionBase);
                    KillMenuWin(mhd, IntuitionBase);
                    KillSubMenuWin(mhd, IntuitionBase);
                }

                mhd->activemenunum = new_activemenunum;
                mhd->activemenu = menu;

                if (menu)
                {
                    HighlightMenuTitle(mhd->activemenu, mhd, IntuitionBase);
                    MakeMenuWin(mhd, IntuitionBase);
                }
            }

            if ((mhd->activeitemnum != -1) && (!mhd->submenuwin))
            {
                HighlightItem(mhd->activeitem, ITEM_ITEM, mhd, IntuitionBase);
                mhd->activeitemnum = -1;
                mhd->activeitem = NULL;
            }

        } /* if (win && (win == mhd->menubarwin)) */
        else
        {
            win = NULL;
        }
    } /* if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey))) */

    if (!win)
    {
        /* mouse outside any menu window */

        if ((mhd->activeitemnum != -1) && (!mhd->submenuwin))
        {
            HighlightItem(mhd->activeitem, ITEM_ITEM, mhd, IntuitionBase);
            mhd->activeitemnum = -1;
            mhd->activeitem = NULL;
        }
        else if (mhd->activesubitemnum != -1)
        {
            HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd, IntuitionBase);
            mhd->activesubitemnum = -1;
            mhd->activesubitem = NULL;
        }
    }

}

/**************************************************************************************************/

static void HandleSelection(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Layer *lay;

    LockLayerInfo(&mhd->scr->LayerInfo);
    lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey);
    UnlockLayerInfo(&mhd->scr->LayerInfo);

    if (lay)
    {
        struct Window   *win = (struct Window *)lay->Window;
        struct MenuItem *item = NULL;

        if (win && (win == mhd->submenuwin) && (mhd->activesubitemnum != -1))
            item = mhd->activesubitem;
        else if (win && (win == mhd->menuwin) && (mhd->activeitemnum != -1))
            item = mhd->activeitem;

        if (item) {
        if (item->Flags & CHECKIT)
                HandleCheckItem(win, item, mhd, IntuitionBase);
    }

        AddToSelection(mhd, IntuitionBase);
    }
}

/**************************************************************************************************/

static void HandleMouseClick(struct InputEvent *ie, struct MenuHandlerData *mhd,
                             struct IntuitionBase *IntuitionBase)
{
    BOOL  die = FALSE;
    ULONG sticky = GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_STICKYMENUS; /* ic_Flags is ULONG */

    switch(ie->ie_Code)
    {
    case MENUUP:
        sticky = sticky && (DoubleClick(
            GetPrivIBase(IntuitionBase)->LastMenuDownSecs,
            GetPrivIBase(IntuitionBase)->LastMenuDownMicro,
            ie->ie_TimeStamp.tv_secs, ie->ie_TimeStamp.tv_micro)
            || mhd->keepmenuup);
        if (sticky)
	    break;

    case SELECTDOWN:
        if (!sticky)
        {
        HandleSelection(mhd, IntuitionBase);

            if (ie->ie_Code == MENUUP)
            die = TRUE;
    }

        break;

    case SELECTUP:
        if (sticky)
        {
            HandleSelection(mhd, IntuitionBase);
            die = TRUE;
        }
        break;

    case MENUDOWN:
    if (sticky)
    {
        if (mhd->keepmenuup)
        mhd->keepmenuup = FALSE;
        else {
        HandleSelection(mhd, IntuitionBase);
        die = TRUE;
        }
        GetPrivIBase(IntuitionBase)->LastMenuDownSecs =
            ie->ie_TimeStamp.tv_secs;
        GetPrivIBase(IntuitionBase)->LastMenuDownMicro =
            ie->ie_TimeStamp.tv_micro;
    }
    break;
    } /* switch(ie->ie_Code) */

    if (die)
    {
        KillMenuBarWin(mhd, IntuitionBase);
    KillMenuWin(mhd, IntuitionBase);
    KillSubMenuWin(mhd, IntuitionBase);

    if (mhd->dri)
    {
            FreeScreenDrawInfo(mhd->scr, mhd->dri);
            mhd->dri = 0;
    }
    MH2Int_MakeMenusInactive(mhd->win, mhd->firstmenupick, IntuitionBase);
    mhd->active = FALSE;
    }
}

/**************************************************************************************************/

static void HandleCheckItem(struct Window *win, struct MenuItem *item, 
                            struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    /* Note: If you change something here, you probably must also change
             menus.c/CheckMenuItemWasClicked() which is used when the
      user uses the menu key shortcuts! */

    WORD itemtype = ((win == mhd->menuwin) ? ITEM_ITEM : ITEM_SUBITEM);

    BOOL re_render = FALSE;

    if (item->Flags & MENUTOGGLE)
    {
        item->Flags ^= CHECKED;
        re_render = TRUE;
    }
    else
    {
        if (!(item->Flags & CHECKED))
        {
            item->Flags |= CHECKED;
            re_render = TRUE;
        }
    }

    if (re_render)
    {
        BOOL toggle_hi = FALSE;

        if ((item->Flags & HIGHITEM) &&
                ((item->Flags & HIGHFLAGS) == HIGHCOMP)) toggle_hi = TRUE;

        if (toggle_hi) HighlightItem(item, itemtype, mhd, IntuitionBase);
        RenderCheckMark(item, itemtype, mhd, IntuitionBase);
        if (toggle_hi) HighlightItem(item, itemtype, mhd, IntuitionBase);

    }

    if (item->MutualExclude)
    {
        struct MenuItem *checkitem = (itemtype == ITEM_ITEM) ? mhd->activemenu->FirstItem :
                                                 mhd->activeitem->SubItem;
        BOOL        toggle_hi = FALSE;
        WORD        i;

        if ((item->Flags & HIGHITEM) &&
                ((item->Flags & HIGHFLAGS) == HIGHBOX)) toggle_hi = TRUE;

        if (toggle_hi) HighlightItem(item, itemtype, mhd, IntuitionBase);

        for(i = 0; (i < 32) && checkitem; i++, checkitem = checkitem->NextItem)
{
            if ((item->MutualExclude & (1L << i)) &&
                    ((checkitem->Flags & (CHECKED | CHECKIT)) == (CHECKIT | CHECKED)))
            {
                checkitem->Flags &= ~CHECKED;
                RenderCheckMark(checkitem, itemtype, mhd, IntuitionBase);
            }
        }

        if (toggle_hi) HighlightItem(item, itemtype, mhd, IntuitionBase);

    } /* if (item->MutualExclude) */
}

/**************************************************************************************************/

static inline BOOL CustomDrawBackground(struct RastPort *rp, struct Window *win,
        LONG itemleft, LONG itemtop, LONG itemwidth, LONG itemheight, UWORD flags,
        struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct mdpDrawBackground  msg;

    msg.MethodID = MDM_DRAWBACKGROUND;
    msg.mdp_RPort = rp;
    msg.mdp_TrueColor = mhd->TrueColor;
    msg.mdp_X = 0;
    msg.mdp_Y = 0;
    msg.mdp_Width = win->Width - 1;
    msg.mdp_Height = win->Height - 1;
    msg.mdp_ItemLeft = itemleft;
    msg.mdp_ItemTop = itemtop;
    msg.mdp_ItemWidth = itemwidth;
    msg.mdp_ItemHeight = itemheight;
    msg.mdp_Flags = flags;

    msg.mdp_MenuDecorFlags = (MENUS_UNDERMOUSE(IntuitionBase)) ? MDP_MDF_MENUS_UNDERMOUSE : 0;
    if (win == mhd->submenuwin) { msg.mdp_UserBuffer = mhd->SubDecorUserBuffer; msg.mdp_MenuDecorFlags |= MDP_MDF_SUBITEM; }
    else if (win == mhd->menuwin) { msg.mdp_UserBuffer = mhd->DecorUserBuffer; msg.mdp_MenuDecorFlags |= MDP_MDF_ITEM; }
    else { msg.mdp_UserBuffer = mhd->BarDecorUserBuffer; msg.mdp_MenuDecorFlags |= MDP_MDF_MENU; }

    return DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);
}

/**************************************************************************************************/

static void HighlightMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    if ((menu->Flags & MENUENABLED) && mhd->menubarwin)
    {
        struct RastPort *rp = mhd->menubarwin->RPort;
        WORD x1, x2, y1, y2;
        
        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            WORD i;
            struct Menu *m = mhd->menu;
        
            x1 = mhd->innerleft;

            x2 = x1 + mhd->menubaritemwidth - 1;
            
            for(i = 0; m != menu; m = m->NextMenu) i++;
    
            y1 = mhd->innertop + i * mhd->menubaritemheight; 
            y2 = y1 + mhd->menubaritemheight - 1;

        }
        else
        {
            x1 = menu->LeftEdge + mhd->scr->BarHBorder - mhd->scr->MenuHBorder;
            if (x1 < 0) x1 = 0;
            y1 = 0;
            x2 = x1 + menu->Width - 1;
            y2 = mhd->scr->BarHeight - 1;
        }

        menu->Flags ^= HIGHITEM;
        if (CustomDrawBackground(rp, mhd->win, x1, y1, x2 - x1 + 1, y2 - y1 + 1, menu->Flags, mhd, IntuitionBase))
        {
            RenderMenuTitle(menu, mhd, IntuitionBase);
            return;
        }

        if (MENUS_AMIGALOOK(IntuitionBase))
        {
            SetDrMd(rp, COMPLEMENT);
            RectFill(rp, x1, y1, x2, y2);
        }
        else
        {
            if (!MENUS_UNDERMOUSE(IntuitionBase)) y1++;

            SetDrMd(rp, JAM1);
            SetAPen(rp, mhd->dri->dri_Pens[(menu->Flags & HIGHITEM) ? FILLPEN : BACKGROUNDPEN]);
            RectFill(rp, x1, y1, x2, y2);
            RenderMenuTitle(menu, mhd, IntuitionBase);
    
            if ((menu->Flags & HIGHITEM))
            {
                if (MENUS_UNDERMOUSE(IntuitionBase))
                {
                    RenderFrame(rp, x1, y1, x2, y2, IDS_SELECTED, mhd, IntuitionBase);
                }
                else
                {
                    SetAPen(rp, mhd->dri->dri_Pens[SHINEPEN]);
                    RectFill(rp, x1, y1, x1, y2);
                    SetAPen(rp, mhd->dri->dri_Pens[SHADOWPEN]);
                    RectFill(rp, x2, y1, x2, y2);
                }
            }
        }
    }
}

/**************************************************************************************************/

static struct Menu *FindMenu(WORD *var, struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct Menu *menu = NULL;
    WORD mouse_x, mouse_y, i;

    if(mhd->menubarwin)
    {
        mouse_x = mhd->scrmousex - mhd->menubarwin->LeftEdge;
        mouse_y = mhd->scrmousey - mhd->menubarwin->TopEdge;

        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            menu = NULL;
    
            mouse_x -= mhd->innerleft;
            mouse_y -= mhd->innertop;
    
            if ((mouse_x >= 0) && (mouse_x < mhd->menubaritemwidth) && (mouse_y >= 0))
            {
                i = mouse_y / mhd->menubaritemheight;
    
                if ((i >= 0) && (i < mhd->nummenubaritems))
                {
                    WORD i2 = i;
    
                    menu = mhd->menu;
                    while(i && menu)
                    {
                        i--;
                        menu = menu->NextMenu;
                    }
    
                    if (menu && (i == 0))
                    {
                        *var = i2;
                    }
                }
            }
        }
        else
        {
            for(menu = mhd->menu, i = 0; menu; menu = menu->NextMenu, i++)
            {
                if ((mouse_x >= menu->LeftEdge) &&
                        (mouse_x < menu->LeftEdge + menu->Width) &&
                        (mouse_y >= 0) &&
                        (mouse_y <= mhd->scr->BarHeight))
                {
                    *var = i;
                    break;
                }
            }
        }
    }
    return menu;
}

/**************************************************************************************************/

static struct MenuItem *FindItem(WORD *var, struct MenuHandlerData *mhd)
{
    struct MenuItem *item = NULL;
    WORD             mouse_x, mouse_y, i;

    if (mhd->menuwin)
    {
        mouse_x = mhd->scrmousex - mhd->menuwin->LeftEdge
            + mhd->activemenu->JazzX - mhd->innerleft - mhd->menuinnerleft;
        mouse_y = mhd->scrmousey - mhd->menuwin->TopEdge
            + mhd->activemenu->JazzY - mhd->innertop - mhd->menuinnertop;

        for(item = mhd->activemenu->FirstItem, i = 0; item; item = item->NextItem, i++)
        {
            if ((mouse_x >= item->LeftEdge) &&
                    (mouse_x < item->LeftEdge + item->Width) &&
                    (mouse_y >= item->TopEdge) &&
                    (mouse_y < item->TopEdge + item->Height))
            {
                *var = i;
                break;
            }
        }
    } /* if (mhd->menuwin) */

    if ((item == NULL) && !mhd->submenuwin) *var = -1;

    return item;
}

/**************************************************************************************************/

static struct MenuItem *FindSubItem(WORD *var, struct MenuHandlerData *mhd)
{
    struct MenuItem *item = NULL;
    WORD             mouse_x, mouse_y, i;

    if (mhd->submenuwin)
    {
        mouse_x = mhd->scrmousex - mhd->submenuwin->LeftEdge + mhd->submenubox.MinX - mhd->menuinnerleft;
        mouse_y = mhd->scrmousey - mhd->submenuwin->TopEdge  + mhd->submenubox.MinY - mhd->menuinnertop;

        *var = -1;

        for(item = mhd->activeitem->SubItem, i = 0; item; item = item->NextItem, i++)
        {
            if ((mouse_x >= item->LeftEdge) &&
                    (mouse_x < item->LeftEdge + item->Width) &&
                    (mouse_y >= item->TopEdge) &&
                    (mouse_y < item->TopEdge + item->Height))
            {
                *var = i;
                break;
            }
        }

    } /* if (mhd->menuwin) */

    return item;
}

/**************************************************************************************************/

static void MakeMenuBarWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TagItem win_tags[] =
    {
        {WA_Left        , 0                       },
        {WA_Top         , 0                       },
        {WA_Width       , mhd->scr->Width        },
        {WA_Height      , mhd->scr->BarHeight + 1},
        {WA_AutoAdjust  , TRUE                    },
        {WA_Borderless  , TRUE                    },
        {WA_CustomScreen, (IPTR)mhd->scr         },
        {WA_BackFill    , (IPTR)LAYERS_NOBACKFILL},
        {TAG_DONE       , 0                          }
    };
    struct Menu *menu;

    mhd->menubarwin = NULL;
    /* No entry to draw ? */
    if(mhd->menu == NULL) return;

    if (MENUS_UNDERMOUSE(IntuitionBase))
    {
        struct RastPort *temprp;
        WORD w, maxw = 0;
    
        if (!(temprp = CloneRastPort(&mhd->scr->RastPort))) return;
    
        mhd->nummenubaritems = 0;
        for(menu = mhd->menu; menu; menu = menu->NextMenu)
        {
            w = TextLength(temprp, menu->MenuName, strlen(menu->MenuName));
            if (w > maxw) maxw = w;
            mhd->nummenubaritems++;
        }
    
        mhd->menubaritemwidth  = maxw + mhd->submenuimage->Width +
                                 TEXT_AMIGAKEY_SPACING +
                                 ITEXT_EXTRA_LEFT +
                                 ITEXT_EXTRA_RIGHT;
    
        if (temprp->TxHeight > mhd->submenuimage->Height)
        {
            mhd->menubaritemheight = temprp->TxHeight;
        }
        else
        {
            mhd->menubaritemheight = mhd->submenuimage->Height;
        }

        mhd->menubaritemheight += (ITEXT_EXTRA_TOP + ITEXT_EXTRA_BOTTOM);

        struct mdpGetMenuSpaces  msg;

        msg.MethodID        = MDM_GETMENUSPACES;
        msg.mdp_TrueColor  = mhd->TrueColor;
        msg.mdp_InnerLeft  = mhd->scr->MenuHBorder;
        msg.mdp_InnerTop   = mhd->scr->MenuVBorder;
        msg.mdp_InnerRight = mhd->scr->MenuHBorder;
        msg.mdp_InnerBottom   = mhd->scr->MenuVBorder;
        msg.mdp_ItemInnerLeft = 0;
        msg.mdp_ItemInnerTop = 0;
        msg.mdp_ItemInnerRight = 0;
        msg.mdp_ItemInnerBottom = 0;
        msg.mdp_MinWidth = 0;
        msg.mdp_MinHeight = 0;
        DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);    
        mhd->innerleft = msg.mdp_InnerLeft;
        mhd->innerright = msg.mdp_InnerRight;
        mhd->innertop = msg.mdp_InnerTop;
        mhd->innerbottom = msg.mdp_InnerBottom;
        mhd->iteminnerleft = msg.mdp_ItemInnerLeft;
        mhd->iteminnerright = msg.mdp_ItemInnerRight;
        mhd->iteminnertop = msg.mdp_ItemInnerTop;
        mhd->iteminnerbottom = msg.mdp_ItemInnerBottom;
        mhd->menubaritemwidth += (mhd->iteminnerleft + mhd->iteminnerright);
        mhd->menubaritemheight += (mhd->iteminnertop + mhd->iteminnerbottom);
 
        win_tags[2].ti_Data = mhd->menubaritemwidth + mhd->innerleft + mhd->innerright;
        win_tags[3].ti_Data = mhd->menubaritemheight * mhd->nummenubaritems + mhd->innertop + mhd->innerbottom;

        if (win_tags[2].ti_Data < msg.mdp_MinWidth)
        {
            mhd->menubaritemwidth += (msg.mdp_MinWidth - win_tags[2].ti_Data);
            win_tags[2].ti_Data = msg.mdp_MinWidth;
        }

        if (win_tags[3].ti_Data < msg.mdp_MinHeight)
        {
            win_tags[3].ti_Data = msg.mdp_MinHeight;
        }

        WORD PosX = mhd->scr->MouseX - win_tags[2].ti_Data / 2;
        win_tags[1].ti_Data = mhd->scr->MouseY;

        if ((PosX + win_tags[2].ti_Data) > mhd->scr->Width) PosX = mhd->scr->Width - win_tags[2].ti_Data;
        if ((win_tags[1].ti_Data + win_tags[3].ti_Data) > mhd->scr->Height) win_tags[1].ti_Data = mhd->scr->Height - win_tags[3].ti_Data;
        if (PosX < 0) PosX = 0;
        if (win_tags[1].ti_Data > 32000) win_tags[1].ti_Data = 0;

        win_tags[0].ti_Data = PosX;

        FreeRastPort(temprp);
    }

    {
        IPTR userbuffersize;
        struct mdpInitMenu  msg;

        GetAttr(MDA_UserBuffer, ((struct IntScreen *)(mhd->scr))->MenuDecorObj, &userbuffersize);

        if (userbuffersize)
        {
            mhd->DecorUserBufferSize = userbuffersize;
            mhd->BarDecorUserBuffer = (IPTR) AllocMem(userbuffersize, MEMF_ANY | MEMF_CLEAR);
            if (0 == mhd->BarDecorUserBuffer) return;
        }

        msg.MethodID = MDM_INITMENU;
        msg.mdp_TrueColor = mhd->TrueColor;
        msg.mdp_RPort = &mhd->scr->RastPort;
        msg.mdp_Left = win_tags[0].ti_Data;
        msg.mdp_Top = win_tags[1].ti_Data;
        msg.mdp_Width = win_tags[2].ti_Data;
        msg.mdp_Height = win_tags[3].ti_Data;
        msg.mdp_UserBuffer = mhd->BarDecorUserBuffer;
        msg.mdp_ScreenUserBuffer = ((struct IntScreen *) mhd->scr)->DecorUserBuffer;
        msg.mdp_MenuDecorFlags = (MENUS_UNDERMOUSE(IntuitionBase)) ? MDP_MDF_MENUS_UNDERMOUSE : 0;
        msg.mdp_MenuDecorFlags |= MDP_MDF_MENU;

        msg.mdp_Screen = mhd->scr;

        DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);
    }

    
    D(bug("MakeMenuBarWin: mhd 0x%lx\n", mhd));

    mhd->menubarwin = OpenWindowTagList(0, win_tags);

    for(menu = mhd->menu; menu; menu = menu->NextMenu)
    {
        menu->Flags &= ~HIGHITEM;
    }

    RenderMenuBar(mhd, IntuitionBase);
}

/**************************************************************************************************/

static void KillMenuBarWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    if (mhd->menubarwin)
    {
        struct mdpExitMenu  msg;

        CloseWindow(mhd->menubarwin);
        mhd->menubarwin = NULL;

        msg.MethodID = MDM_EXITMENU;
        msg.mdp_UserBuffer = mhd->BarDecorUserBuffer;
        msg.mdp_TrueColor = mhd->TrueColor;
        DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);

        if (mhd->BarDecorUserBuffer)
        {
            FreeMem((void *)mhd->BarDecorUserBuffer, mhd->DecorUserBufferSize);
        }
        mhd->BarDecorUserBuffer = 0;
    }

}

/**************************************************************************************************/

static void RenderMenuBar(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    if (mhd->menubarwin)
    {
        struct Menu *menu = mhd->menu;
        struct Window *win = mhd->menubarwin;
        struct RastPort *rp = win->RPort;

        SetFont(rp, mhd->dri->dri_Font);

        /* Bar renders using different pens in Amiga mode than rest of menu */
        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            RenderMenuBG(win, mhd, IntuitionBase);
        }
        else
        {
            if (!CustomDrawBackground(rp, win, 0, 0, win->Width - 1, win->Height - 1, 0, mhd, IntuitionBase))
            {
                if (MENUS_AMIGALOOK(IntuitionBase))
                {
                    SetABPenDrMd(rp, mhd->dri->dri_Pens[BARBLOCKPEN], 0, JAM1);
                }
                else
                {
                    SetABPenDrMd(rp, mhd->dri->dri_Pens[BACKGROUNDPEN], 0, JAM1);
                }
    
                RectFill(rp, 0, 0, win->Width - 1, win->Height - 2);

                SetAPen(rp, mhd->dri->dri_Pens[BARTRIMPEN]);
                RectFill(rp, 0, win->Height - 1, win->Width - 1, win->Height - 1);

                if (!MENUS_AMIGALOOK(IntuitionBase))
                {
                    SetAPen(rp, mhd->dri->dri_Pens[SHINEPEN]);
                    RectFill(rp, 0, 0, 0, win->Height - 2);
                    RectFill(rp, 1, 0, win->Width - 1, 0);
                    SetAPen(rp, mhd->dri->dri_Pens[SHADOWPEN]);
                    RectFill(rp, win->Width - 1, 1, win->Width - 1, win->Height - 2);
                }
            }
        }
        
        for(; menu; menu = menu->NextMenu)
        {
            RenderMenuTitle(menu, mhd, IntuitionBase);
        }
    }
}

/**************************************************************************************************/

static void RenderMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd,
                            struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort *rp;
    WORD len = strlen(menu->MenuName);
    WORD x, y;

    if(mhd->menubarwin)
    {
        rp = mhd->menubarwin->RPort;
        SetDrMd(rp, JAM1);

        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            struct Menu *m;
            WORD         yoff;

            yoff = 0;
            for(m = mhd->menu; m && (m != menu);m = m ->NextMenu)
            {
                yoff++;
            }

            x = mhd->innerleft + ITEXT_EXTRA_LEFT;
            y = mhd->innertop + ITEXT_EXTRA_TOP + yoff * mhd->menubaritemheight;
        }
        else
        {
            x = mhd->scr->BarHBorder + menu->LeftEdge;
            y = mhd->scr->BarVBorder + ((mhd->scr->BarHeight - rp->Font->tf_YSize) / 2);
        }

        if (MENUS_AMIGALOOK(IntuitionBase))
        {
            SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
        }
        else
        {
            SetAPen(rp, mhd->dri->dri_Pens[(menu->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN]);
        }

        Move(rp, x, y + rp->TxBaseline);
        Text(rp, menu->MenuName, len);

        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            if (menu->FirstItem)
            {
                WORD x2 = mhd->scr->MenuHBorder + mhd->menubaritemwidth - ITEXT_EXTRA_RIGHT - mhd->submenuimage->Width;

            DrawImageState(rp, mhd->submenuimage, x2, y + ((mhd->menubaritemheight - mhd->submenuimage->Height) >> 1), IDS_NORMAL, mhd->dri);
            }
        }

        if (!(menu->Flags & MENUENABLED))
        {
            WORD y2 = y + rp->TxHeight - 1;
            WORD x2;

            if (MENUS_UNDERMOUSE(IntuitionBase))
            {
                x2 = mhd->scr->MenuHBorder + mhd->menubaritemwidth - 1;
            }
            else
            {
                x2 = x + TextLength(rp, menu->MenuName, len) - 1;
            }

            RenderDisabledPattern(rp, x, y, x2, y2, mhd, IntuitionBase);
        }
    }
}

/**************************************************************************************************/

static void MakeMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct MenuItem *item;

    WORD width  = mhd->activemenu->BeatX - mhd->activemenu->JazzX + 1;
    WORD height = mhd->activemenu->BeatY - mhd->activemenu->JazzY + 1;
    WORD xpos,ypos;

    struct mdpGetMenuSpaces  msg;

    msg.MethodID = MDM_GETMENUSPACES;
    msg.mdp_TrueColor = mhd->TrueColor;
    msg.mdp_InnerLeft  = 0;
    msg.mdp_InnerTop   = 0;
    msg.mdp_InnerRight = 0;
    msg.mdp_InnerBottom   = 0;
    msg.mdp_ItemInnerLeft = 0;
    msg.mdp_ItemInnerTop = 0;
    msg.mdp_ItemInnerRight = 0;
    msg.mdp_ItemInnerBottom = 0;
    DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);    
    mhd->menuinnerleft = msg.mdp_InnerLeft;
    mhd->menuinnerright = msg.mdp_InnerRight;
    mhd->menuinnertop = msg.mdp_InnerTop;
    mhd->menuinnerbottom = msg.mdp_InnerBottom;

    width += (mhd->menuinnerleft + mhd->menuinnerright);
    height += (mhd->menuinnertop + mhd->menuinnerbottom);

    if (MENUS_UNDERMOUSE(IntuitionBase))
    {
        xpos = mhd->menubarwin->LeftEdge + mhd->menubarwin->Width - 16;
        ypos = mhd->menubarwin->TopEdge;
    }
    else
    {
        xpos = mhd->activemenu->LeftEdge + mhd->scr->BarHBorder + mhd->activemenu->JazzX;
    
        if (MENUS_AMIGALOOK(IntuitionBase))
        {
            ypos = mhd->scr->BarHeight + 1 + mhd->activemenu->JazzY;
        }
        else
        {
            ypos = mhd->scr->BarHeight + 1;
        }
    }

    {
        struct TagItem win_tags[] =
        {
            {WA_Left         , xpos             },
            {WA_Top         , ypos             },
            {WA_Width       , width            },
            {WA_Height      , height           },
            {WA_AutoAdjust  , TRUE             },
            {WA_Borderless  , TRUE             },
            {WA_CustomScreen, (IPTR)mhd->scr   },
            {WA_BackFill    , (IPTR)LAYERS_NOBACKFILL},
            {TAG_DONE       , 0                }
        };

        if (MENUS_UNDERMOUSE(IntuitionBase))
        {
            win_tags[1].ti_Data += (mhd->menubaritemheight * mhd->activemenunum + mhd->scr->MenuVBorder) -
                                   height / 2;
            if (xpos + width > mhd->scr->Width)
            {
                win_tags[0].ti_Data = mhd->menubarwin->LeftEdge - width + 16;
            }
        }

     if ((win_tags[0].ti_Data + win_tags[2].ti_Data) > mhd->scr->Width) win_tags[0].ti_Data = mhd->scr->Width - win_tags[2].ti_Data;
     if ((win_tags[1].ti_Data + win_tags[3].ti_Data) > mhd->scr->Height) win_tags[1].ti_Data = mhd->scr->Height - win_tags[3].ti_Data;
        if (((LONG) win_tags[0].ti_Data) < 0) win_tags[0].ti_Data = 0;
        if (((LONG) win_tags[1].ti_Data) < 0) win_tags[1].ti_Data = 0;

        if ((item = mhd->activemenu->FirstItem))
        {

            while(item)
            {
                item->Flags &= ~HIGHITEM;
                item = item->NextItem;
            }

            IPTR    userbuffersize;

        GetAttr(MDA_UserBuffer, ((struct IntScreen *)(mhd->scr))->MenuDecorObj, &userbuffersize);
    
        if (userbuffersize)
        {
        mhd->DecorUserBufferSize = userbuffersize;
        mhd->DecorUserBuffer = (IPTR) AllocMem(userbuffersize, MEMF_ANY | MEMF_CLEAR);
        if (0 == mhd->DecorUserBuffer) return;
        }

            struct mdpInitMenu  msg;

            msg.MethodID        = MDM_INITMENU;
            msg.mdp_TrueColor   = mhd->TrueColor;
            msg.mdp_RPort       = &mhd->scr->RastPort;
            msg.mdp_Left        = win_tags[0].ti_Data;
            msg.mdp_Top         = win_tags[1].ti_Data;
            msg.mdp_Width       = width;
            msg.mdp_Height      = height;
            msg.mdp_UserBuffer  = mhd->DecorUserBuffer;
            msg.mdp_ScreenUserBuffer   = ((struct IntScreen *) mhd->scr)->DecorUserBuffer;
            msg.mdp_Screen             = mhd->scr;
            msg.mdp_MenuDecorFlags = (MENUS_UNDERMOUSE(IntuitionBase)) ? MDP_MDF_MENUS_UNDERMOUSE : 0;
            msg.mdp_MenuDecorFlags |= MDP_MDF_ITEM;

            DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);

            mhd->menuwin = OpenWindowTagList(0, win_tags);

            mhd->maxcommkeywidth_menu = CalcMaxCommKeyWidth(mhd->menuwin, mhd, IntuitionBase);

            RenderMenu(mhd, IntuitionBase);

            mhd->activemenu->Flags |= MIDRAWN;
        }
    }
}

/**************************************************************************************************/

static void KillMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    if (mhd->menuwin)
    {
        struct MenuItem *item;

        CloseWindow(mhd->menuwin);
        mhd->menuwin = NULL;

        TimeDelay(UNIT_VBLANK,0,20000);

        for(item = mhd->activemenu->FirstItem; item; item = item->NextItem)
        {
            item->Flags &= ~ISDRAWN;
        }
           struct mdpExitMenu  msg;

        msg.MethodID       = MDM_EXITMENU;
        msg.mdp_TrueColor  = mhd->TrueColor;
        msg.mdp_UserBuffer = mhd->DecorUserBuffer;
        DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);

        if (mhd->DecorUserBuffer)
        {
            FreeMem((void *)mhd->DecorUserBuffer, mhd->DecorUserBufferSize);
        }
        mhd->DecorUserBuffer = 0;

        mhd->activemenu->Flags &= ~MIDRAWN;

        mhd->activeitemnum = -1;
        mhd->activeitem = NULL;
    }
}

/**************************************************************************************************/

static void RenderMenu(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    if (mhd->menuwin)
    {
        struct MenuItem *item;

        RenderMenuBG(mhd->menuwin, mhd, IntuitionBase);

        SetFont(mhd->menuwin->RPort, mhd->dri->dri_Font);

        for(item = mhd->activemenu->FirstItem; item; item = item->NextItem)
        {
            RenderItem(item, ITEM_ITEM, (struct Rectangle *)(&mhd->activemenu->JazzX), mhd, IntuitionBase);
        }

    } /* if (mhd->menuwin) */
}

/**************************************************************************************************/

static void MakeSubMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct MenuItem *item = mhd->activeitem->SubItem;

    struct TagItem  win_tags[] =
    {
        {WA_Left        , 0                         },
        {WA_Top         , 0                         },
        {WA_Width       , 0                         },
        {WA_Height      , 0                         },
        {WA_AutoAdjust  , TRUE                      },
        {WA_Borderless  , TRUE                      },
        {WA_CustomScreen, (IPTR)mhd->scr           },
        {WA_BackFill    , (IPTR)LAYERS_NOBACKFILL  },
        {TAG_DONE                                   }
    };

    if(mhd->menubarwin)
    {
        GetMenuBox(mhd->menubarwin, item, &mhd->submenubox.MinX,
                   &mhd->submenubox.MinY,
                   &mhd->submenubox.MaxX,
                   &mhd->submenubox.MaxY);

        {
            struct mdpGetMenuSpaces  msg;

            msg.MethodID       = MDM_GETMENUSPACES;
            msg.mdp_TrueColor  = mhd->TrueColor;
            msg.mdp_InnerLeft  = 0;
            msg.mdp_InnerTop   = 0;
            msg.mdp_InnerRight = 0;
            msg.mdp_InnerBottom   = 0;

            DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);
            mhd->menuinnerleft = msg.mdp_InnerLeft;
            mhd->menuinnerright = msg.mdp_InnerRight;
            mhd->menuinnertop = msg.mdp_InnerTop;
            mhd->menuinnerbottom = msg.mdp_InnerBottom;
        }

        win_tags[0].ti_Data = mhd->menuwin->LeftEdge +
                              mhd->activeitem->LeftEdge - mhd->activemenu->JazzX +
                              mhd->submenubox.MinX;

        win_tags[1].ti_Data = mhd->menuwin->TopEdge +
                              mhd->activeitem->TopEdge - mhd->activemenu->JazzY +
                              mhd->submenubox.MinY;

        win_tags[2].ti_Data = mhd->submenubox.MaxX - mhd->submenubox.MinX + 1 + mhd->menuinnerleft + mhd->menuinnerright;
        win_tags[3].ti_Data = mhd->submenubox.MaxY - mhd->submenubox.MinY + 1 + mhd->menuinnertop + mhd->menuinnerbottom;

        if ((win_tags[0].ti_Data + win_tags[2].ti_Data) > mhd->scr->Width) win_tags[0].ti_Data = mhd->scr->Width - win_tags[2].ti_Data;
        if ((win_tags[1].ti_Data + win_tags[3].ti_Data) > mhd->scr->Height) win_tags[1].ti_Data = mhd->scr->Height - win_tags[3].ti_Data;
        if (((LONG) win_tags[0].ti_Data) < 0) win_tags[0].ti_Data = 0;
        if (((LONG) win_tags[1].ti_Data) < 0) win_tags[1].ti_Data = 0;

        while(item)
        {
            item->Flags &= ~HIGHITEM;
            item = item->NextItem;
        }

        IPTR    userbuffersize;

        GetAttr(MDA_UserBuffer, ((struct IntScreen *)(mhd->scr))->MenuDecorObj, &userbuffersize);

        if (userbuffersize)
        {
        mhd->DecorUserBufferSize = userbuffersize;
        mhd->SubDecorUserBuffer = (IPTR) AllocMem(userbuffersize, MEMF_ANY | MEMF_CLEAR);
        if (0 == mhd->SubDecorUserBuffer) return;
        }

        {
            struct mdpInitMenu  msg;
            msg.MethodID             = MDM_INITMENU;
            msg.mdp_TrueColor = mhd->TrueColor;
            msg.mdp_RPort         = &mhd->scr->RastPort;
            msg.mdp_Left = win_tags[0].ti_Data;
            msg.mdp_Top = win_tags[1].ti_Data;
            msg.mdp_Width = win_tags[2].ti_Data;
            msg.mdp_Height = win_tags[3].ti_Data;
            msg.mdp_UserBuffer = mhd->SubDecorUserBuffer;
            msg.mdp_ScreenUserBuffer   = ((struct IntScreen *) mhd->scr)->DecorUserBuffer;
            msg.mdp_TrueColor = mhd->TrueColor;
            msg.mdp_Screen             = mhd->scr;
            msg.mdp_MenuDecorFlags = (MENUS_UNDERMOUSE(IntuitionBase)) ? MDP_MDF_MENUS_UNDERMOUSE : 0;
            msg.mdp_MenuDecorFlags |= MDP_MDF_SUBITEM;

            DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);
        }
        mhd->submenuwin = OpenWindowTagList(0, win_tags);

        mhd->maxcommkeywidth_submenu = CalcMaxCommKeyWidth(mhd->submenuwin, mhd, IntuitionBase);

        RenderSubMenu(mhd, IntuitionBase);
    }
}

/**************************************************************************************************/

static void KillSubMenuWin(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    if (mhd->submenuwin)
    {
        CloseWindow(mhd->submenuwin);

        TimeDelay(UNIT_VBLANK,0,20000);
        struct mdpExitMenu  msg;

        msg.MethodID       = MDM_EXITMENU;
        msg.mdp_TrueColor  = mhd->TrueColor;
        msg.mdp_UserBuffer = mhd->SubDecorUserBuffer;
        DoMethodA(((struct IntScreen *)(mhd->scr))->MenuDecorObj, (Msg)&msg);    

        if (mhd->SubDecorUserBuffer)
        {
            FreeMem((void *)mhd->SubDecorUserBuffer, mhd->DecorUserBufferSize);
        }
        mhd->SubDecorUserBuffer = 0;

        mhd->submenuwin = NULL;

        mhd->activesubitemnum = -1;
        mhd->activesubitem = NULL;
    }
}

/**************************************************************************************************/

static void RenderSubMenu(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    if (mhd->submenuwin)
    {
        struct MenuItem *item;

        RenderMenuBG(mhd->submenuwin, mhd, IntuitionBase);

        SetFont(mhd->submenuwin->RPort, mhd->dri->dri_Font);

        for(item = mhd->activeitem->SubItem; item; item = item->NextItem)
        {
            RenderItem(item, ITEM_SUBITEM, (struct Rectangle *)(&mhd->submenubox), mhd, IntuitionBase);
        }

    } /* if (mhd->submenuwin) */
}

/**************************************************************************************************/

static void RenderItem(struct MenuItem *item, WORD itemtype,  struct Rectangle *box,
                       struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase  *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window   *win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort *rp = win->RPort;
    WORD             offx = -box->MinX + mhd->menuinnerleft;
    WORD             offy = -box->MinY + mhd->menuinnertop;
    BOOL             enabled = ((item->Flags & ITEMENABLED) &&
                               (mhd->activemenu->Flags & MENUENABLED) &&
                               ((itemtype == ITEM_ITEM) || (mhd->activeitem->Flags & ITEMENABLED)));
    BOOL             item_supports_disable = FALSE;

    SetDrMd(rp, JAM1);

    if (item->ItemFill)
    {

        if (item->Flags & ITEMTEXT)
        {
        struct IntuiText *save = ((struct IntuiText*) item->ItemFill)->NextText;

        if (item->SubItem)
            {
            Forbid();
            ((struct IntuiText*) item->ItemFill)->NextText = NULL;
        }

            struct IntuiText *it = (struct IntuiText *)item->ItemFill;
    
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                PrintIText(rp, it, offx + item->LeftEdge, offy + item->TopEdge);
            }
            else
            {
                it->FrontPen = mhd->dri->dri_Pens[(item->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN];
                it->DrawMode = JAM1;
    
                PrintIText(rp, it, offx + item->LeftEdge, offy + item->TopEdge);
            }
        if (item->SubItem)
        { 
        DrawImageState(rp, mhd->submenuimage, offx + item->Width - mhd->submenuimage->Width, offy + item->TopEdge + ((item->Height - mhd->submenuimage->Height) >> 1), IDS_NORMAL, mhd->dri);
            ((struct IntuiText*) item->ItemFill)->NextText = save;
            Permit();
        }
        }
        else
        {
            struct Image *im = (struct Image *)item->ItemFill;
            LONG           state = IDS_NORMAL;

            if (!enabled && (im->Depth == CUSTOMIMAGEDEPTH))
            {
                IPTR val = 0;

                GetAttr(IA_SupportsDisable, (Object *)im, &val);
                if (val)
                {
                    item_supports_disable = TRUE;
                    state = IDS_DISABLED;
                }
            }

            DrawImageState(rp, im, offx + item->LeftEdge, offy + item->TopEdge, state, mhd->dri);
        }

    } /* if (item->ItemFill) */

    RenderCheckMark(item, itemtype, mhd, IntuitionBase);
    RenderAmigaKey(item, itemtype, mhd, IntuitionBase);

    if (!enabled && !item_supports_disable)
    {
        RenderDisabledPattern(rp, offx + item->LeftEdge,
                              offy + item->TopEdge,
                              offx + item->LeftEdge + item->Width - 1,
                              offy + item->TopEdge + item->Height - 1,
                              mhd,
                              IntuitionBase);
    }

}

/**************************************************************************************************/

static void RenderMenuBG(struct Window *win, struct MenuHandlerData *mhd,
                         struct IntuitionBase *IntuitionBase)
{

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort *rp = win->RPort;
    WORD             borderx, bordery;

    if (CustomDrawBackground(rp, win, 0, 0, win->Width - 1, win->Height - 1, 0, mhd, IntuitionBase)) return;

    if (MENUS_AMIGALOOK(IntuitionBase))
    {
        borderx = mhd->scr->MenuHBorder / 2;
        bordery = mhd->scr->MenuVBorder / 2;
    }
    else
    {
        borderx = 1;
        bordery = 1;
    }

    /* White background */

    if (MENUS_AMIGALOOK(IntuitionBase))
    {
        SetABPenDrMd(rp, mhd->dri->dri_Pens[BARBLOCKPEN], 0, JAM1);
    }
    else
    {
        SetABPenDrMd(rp, mhd->dri->dri_Pens[BACKGROUNDPEN], 0, JAM1);
    }
    
    RectFill(rp, borderx,
             bordery,
             win->Width - 1 - borderx,
             win->Height - 1 - bordery);
    
    /* Black border frame */

    if (MENUS_AMIGALOOK(IntuitionBase))
    {
        SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
        RectFill(rp, 0, 0, win->Width - 1, bordery - 1);
        RectFill(rp, 0, bordery, borderx - 1, win->Height - 1 - bordery);
        RectFill(rp, win->Width - borderx, bordery, win->Width - 1, win->Height - 1);
        RectFill(rp, 0, win->Height - bordery, win->Width - 1 - borderx, win->Height - 1);
    }
    else
    {
        RenderFrame(rp, 0, 0, win->Width - 1, win->Height - 1, IDS_NORMAL, mhd, IntuitionBase);
    }
}

/**************************************************************************************************/

static void RenderCheckMark(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                            struct IntuitionBase *IntuitionBase)
{
    struct GfxBase      *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window       *win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort     *rp = win->RPort;
    struct Rectangle    *box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    WORD                 offx = -box->MinX + mhd->menuinnerleft;
    WORD                 offy = -box->MinY + mhd->menuinnertop;
    WORD                 state = ((item->Flags & HIGHITEM) &&
                                 ((item->Flags & HIGHFLAGS) == HIGHCOMP)) ? IDS_SELECTED : IDS_NORMAL;

    if (item->Flags & CHECKIT)
    {
        WORD x1, y1, x2, y2;

        x1 = item->LeftEdge + offx;
        y1 = item->TopEdge  + offy + (item->Height - mhd->checkmark->Height) / 2;
        x2 = x1 + mhd->checkmark->Width  - 1;
        y2 = y1 + mhd->checkmark->Height - 1;

        SetDrMd(rp, JAM1);

        if (item->Flags & CHECKED)
        {
            DrawImageState(rp, mhd->checkmark, x1, y1, state, mhd->dri);
        }
        else
        {
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetAPen(rp, mhd->dri->dri_Pens[BARBLOCKPEN]);
            }
            else
            {
                SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
            }

            if (!CustomDrawBackground(rp, win, x1, y1, x2 - x1 + 1, y2 - y1 + 1, item->Flags, mhd, IntuitionBase)) RectFill(rp, x1, y1, x2, y2);
        }
    }

}

/**************************************************************************************************/

static void RenderAmigaKey(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                           struct IntuitionBase *IntuitionBase)
{
    struct GfxBase      *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window       *win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort     *rp = win->RPort;
    struct Rectangle    *box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    WORD                 commkeywidth = ((itemtype == ITEM_ITEM) ? mhd->maxcommkeywidth_menu : mhd->maxcommkeywidth_submenu);
    WORD                 offx = -box->MinX + mhd->menuinnerleft;
    WORD                 offy = -box->MinY + mhd->menuinnertop;
    WORD                 state = ((item->Flags & HIGHITEM) &&
                                 ((item->Flags & HIGHFLAGS) == HIGHCOMP)) ? IDS_SELECTED : IDS_NORMAL;

    if (item->Flags & COMMSEQ)
    {
        struct TextFont *oldfont = rp->Font;
        struct TextFont *newfont = NULL;

        WORD x1, y1;

        if (item->Flags & ITEMTEXT)
        {
            struct IntuiText *it = (struct IntuiText *)item->ItemFill;

            if (it->ITextFont)
            {
                if ((newfont = OpenFont(it->ITextFont)))
                {
                    SetFont(rp, newfont);
                }
            }
        }

        x1 = item->LeftEdge + offx + item->Width - AMIGAKEY_BORDER_SPACING -
             mhd->amigakey->Width - AMIGAKEY_KEY_SPACING - commkeywidth;
        y1 = item->TopEdge  + offy + (item->Height - mhd->amigakey->Height + 1) / 2;

        SetDrMd(rp, JAM1);

        DrawImageState(rp, mhd->amigakey, x1, y1, state, mhd->dri);

        x1 += mhd->amigakey->Width + AMIGAKEY_KEY_SPACING;

        if (MENUS_AMIGALOOK(IntuitionBase))
        {
            SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
        }
        else
        {
            SetAPen(rp, mhd->dri->dri_Pens[(item->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN]);
        }
        
        Move(rp, x1, item->TopEdge + offy + (item->Height - rp->TxHeight) / 2 +
             rp->TxBaseline);
        Text(rp, &item->Command, 1);

        if (newfont)
        {
            CloseFont(newfont);
            SetFont(rp, oldfont);
        }

    } /* if (item->Flags & COMMSEQ) */
}

/**************************************************************************************************/

static void RenderDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
                                  struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    static CONST UWORD pattern [] = {0x8888, 0x2222};

    SetDrMd(rp, JAM1);
    
    if (MENUS_AMIGALOOK(IntuitionBase))
    {
        SetAPen(rp, mhd->dri->dri_Pens[BARBLOCKPEN]);
    }
    else
    {
        SetAPen(rp, mhd->dri->dri_Pens[BACKGROUNDPEN]);
    }

    SetAfPt(rp, pattern, 1);

    RectFill(rp, x1, y1, x2, y2);

    SetAfPt(rp, NULL, 0);

}

/**************************************************************************************************/

static void RenderFrame(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD state,
                        struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;

    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? SHADOWPEN : SHINEPEN]);

    RectFill(rp, x1, y1, x2, y1);
    RectFill(rp, x1, y1 + 1, x1, y2);

    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? SHINEPEN : SHADOWPEN]);
    RectFill(rp, x2, y1 + 1, x2, y2);
    RectFill(rp, x1 + 1, y2, x2 - 1, y2);
}
/**************************************************************************************************/

static void HighlightItem(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd,
                          struct IntuitionBase *IntuitionBase)
{
    struct GfxBase      *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Window       *win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort     *rp = win->RPort;
    struct Rectangle    *box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    APTR                 fill;
    WORD                 offx = -box->MinX + mhd->menuinnerleft;
    WORD                 offy = -box->MinY + mhd->menuinnertop;
    WORD                 x1, y1, x2, y2;
    BOOL                 enabled;

    enabled = (item->Flags & ITEMENABLED) ? TRUE : FALSE;
    if (!(mhd->activemenu->Flags & MENUENABLED)) enabled = FALSE;
    if ((itemtype == ITEM_SUBITEM) && !(mhd->activeitem->Flags & ITEMENABLED)) enabled = FALSE;

    if (enabled)
    {
        item->Flags ^= HIGHITEM;

        fill = item->ItemFill;
        if ((item->Flags & HIGHITEM) && (item->SelectFill)) fill = item->SelectFill;

        x1 = offx + item->LeftEdge;
        y1 = offy + item->TopEdge;
        x2 = x1 + item->Width - 1;
        y2 = y1 + item->Height - 1;

    if (CustomDrawBackground(rp, win, x1, y1, x2 - x1 + 1, y2 - y1 + 1, item->Flags, mhd, IntuitionBase)) {
            SetDrMd(rp, JAM1);

            if(item->Flags & ITEMTEXT)
            {
            struct IntuiText *save = ((struct IntuiText*) fill)->NextText;

            if (item->SubItem)
                {
            Forbid();
            ((struct IntuiText*) fill)->NextText = NULL;
            }

                if (MENUS_AMIGALOOK(IntuitionBase))
                {
                    PrintIText(rp, (struct IntuiText *)fill, x1, y1);
                }
                else
                {
                    struct IntuiText *it = (struct IntuiText *)fill;
    
                    it->FrontPen = mhd->dri->dri_Pens[TEXTPEN];
                    it->DrawMode = JAM1;
    
                    PrintIText(rp, it, x1, y1);
                }
            if (item->SubItem)
            { 
            DrawImageState(rp, mhd->submenuimage, offx + item->Width - mhd->submenuimage->Width, offy + item->TopEdge + ((item->Height - mhd->submenuimage->Height) >> 1), IDS_NORMAL, mhd->dri);
                ((struct IntuiText*) fill)->NextText = save;
                Permit();
            }
            }
            else
            {
                EraseImage(rp, (struct Image *)fill, x1, y1);
                DrawImageState(rp, (struct Image *)fill, x1, y1, IDS_SELECTED, mhd->dri);
            }
        RenderItem(item, itemtype, box, mhd, IntuitionBase);
        return;
    }

        switch(item->Flags & HIGHFLAGS)
        {
        case HIGHIMAGE:
            SetDrMd(rp, JAM1);

            if(item->Flags & ITEMTEXT)
            {
            struct IntuiText *save = ((struct IntuiText*) fill)->NextText;

            if (item->SubItem)
                {
            Forbid();
            ((struct IntuiText*) fill)->NextText = NULL;
            }
                if (MENUS_AMIGALOOK(IntuitionBase))
                {
                    PrintIText(rp, (struct IntuiText *)fill, x1, y1);
                }
                else
                {
                    struct IntuiText *it = (struct IntuiText *)fill;
    
                    it->FrontPen = mhd->dri->dri_Pens[TEXTPEN];
                    it->DrawMode = JAM1;
    
                    PrintIText(rp, it, x1, y1);
                }
            if (item->SubItem)
            { 
            DrawImageState(rp, mhd->submenuimage, offx + item->Width - mhd->submenuimage->Width, offy + item->TopEdge + ((item->Height - mhd->submenuimage->Height) >> 1), IDS_NORMAL, mhd->dri);
                ((struct IntuiText*) fill)->NextText = save;
                Permit();
            }
            }
            else
            {
                EraseImage(rp, (struct Image *)fill, x1, y1);
                DrawImageState(rp, (struct Image *)fill, x1, y1, IDS_SELECTED, mhd->dri);
            }
            break;

        case HIGHCOMP:
            if (MENUS_AMIGALOOK(IntuitionBase))
            {
                SetDrMd(rp, COMPLEMENT);
                RectFill(rp, x1, y1, x2, y2);
            }
            else
            {
                WORD state = (item->Flags & HIGHITEM) ? IDS_SELECTED : IDS_NORMAL;
    
                SetDrMd(rp, JAM1);
                SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
                RectFill(rp, x1, y1, x2, y2);
    
                RenderItem(item, itemtype, box, mhd, IntuitionBase);
    
                if (state == IDS_SELECTED)
                {
                    RenderFrame(rp, x1, y1, x2, y2, state, mhd, IntuitionBase);
                }
            }
            break;

        case HIGHBOX:
            SetDrMd(rp, COMPLEMENT);
            offx = mhd->scr->MenuHBorder;
            offy = mhd->scr->MenuVBorder;

            x1 -= offx;
            x2 += offx;
            y1 -= offy;
            y2 += offy;

            RectFill(rp, x1, y1, x2, y1 + offy - 1);
            RectFill(rp, x2 - offx + 1, y1 + offy, x2, y2);
            RectFill(rp, x1, y2 - offy + 1, x2 - offx, y2);
            RectFill(rp, x1, y1 + offy, x1 + offx - 1,y2 - offy);
            break;

        case HIGHNONE:
            /* Do nothing */
            break;

        } /* switch(item->Flags & HIGHFLAGS) */

    } /* if (enabled) */

}

/**************************************************************************************************/

static WORD CalcMaxCommKeyWidth(struct Window *win, struct MenuHandlerData *mhd,
                                struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TextExtent te;
    WORD maxwidth;

    FontExtent(mhd->dri->dri_Font, &te);
    maxwidth = te.te_Width;
    
    if (win)
    {
        struct MenuItem *item;

        if ((win == mhd->menuwin))
        {
            item = mhd->activemenu->FirstItem;
        }
        else
        {
            item = mhd->activeitem->SubItem;
        }

        for(; item; item = item->NextItem)
        {
            if (item->Flags & ITEMTEXT)
            {
                struct IntuiText *it = (struct IntuiText *)item->ItemFill;

                if (it->ITextFont)
                {
                    struct TextFont *font;

                    if ((font = OpenFont(it->ITextFont)))
                    {
                FontExtent(font, &te);
                        if (te.te_Width > maxwidth) maxwidth = te.te_Width;

                        CloseFont(font);
                    }
                }

            } /* if (item->Flags & ITEMTEXT) */

        } /* for(; item; item = item->NextItem); */

    } /* if (win) */

    return maxwidth;
}

/**************************************************************************************************/

static void AddToSelection(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    if ((mhd->activemenunum != -1) && (mhd->activemenu->Flags & MENUENABLED) &&
            (mhd->activeitemnum != -1) && (mhd->activeitem->Flags & ITEMENABLED))
    {
        struct MenuItem *item = NULL;
        UWORD              men = FULLMENUNUM(mhd->activemenunum, mhd->activeitemnum, mhd->activesubitemnum);

        if (mhd->activesubitemnum != -1)
        {
            if (mhd->activesubitem->Flags & ITEMENABLED) item = mhd->activesubitem;
        }
        else if (!mhd->activeitem->SubItem)
        {
            item = mhd->activeitem;
        }

        if (item && (ItemAddress(mhd->menu, men) == item))
        {
            UWORD men = FULLMENUNUM(mhd->activemenunum, mhd->activeitemnum, mhd->activesubitemnum);

            if (mhd->firstmenupick == MENUNULL)
            {
                mhd->firstmenupick = men;
            }
            else if (men != mhd->lastmenupick)
            {
                struct MenuItem *checkitem, *prevcheckitem = NULL;
                UWORD              checkmen = mhd->firstmenupick;

                /* Remove men from pick queue, if it was already in there
                   and then add it at the end of the pick queue */

                while(checkmen != MENUNULL)
                {
                    checkitem = ItemAddress(mhd->menu, checkmen);

                    if (checkmen == men)
                    {
                        if (prevcheckitem == NULL)
                        {
                            mhd->firstmenupick = checkitem->NextSelect;
                        }
                        else
                        {
                            prevcheckitem->NextSelect = checkitem->NextSelect;
                        }
                    }

                    checkmen = checkitem->NextSelect;
                    prevcheckitem = checkitem;

                } /* while(checkmen != MENUNULL) */

                checkitem->NextSelect = men;

            } /* else if (men != mhd->lastmenupick) */

            mhd->lastmenupick = men;
            item->NextSelect = MENUNULL;

        } /* if (item) */

    } /* if ((mhd->activemenunum != -1) && (mhd->activeitemnum != -1)) */
}

