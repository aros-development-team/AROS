/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
#include "inputhandler.h"
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "menus.h"
#include "menutask.h"
#include "smallmenu.h"
#include "smallmenu_menusupport.h"
#include "smallmenu_render.h"
#include "intuition_customizesupport.h"
#include "mosmisc.h"

#include <cybergraphx/cybergraphics.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

extern ULONG HookEntry();

#define DEBUG_CLICK(x)  ;
#define DEBUG_ADDTO(x)  ;

#define CLOCKTICKS 1

/**************************************************************************************************/

/* this #defines are taken from workbench/libs/gadtools/menus.c!! */

#define TEXT_AMIGAKEY_SPACING  6

#define ITEXT_EXTRA_LEFT   2
#define ITEXT_EXTRA_RIGHT  2
#define ITEXT_EXTRA_TOP    1
#define ITEXT_EXTRA_BOTTOM 1

//static const char *subitemindicator = "»";

/**************************************************************************************************/

void HandleMouseMove(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase);
void HandleMouseClick(struct InputEvent *ie, struct MenuHandlerData *mhd,
                             struct IntuitionBase *IntuitionBase);

/**************************************************************************************************/

/******************************
**  CreateMenuHandlerTask()  **
******************************/
struct Task *CreateMenuHandlerTask(APTR taskparams, struct IntuitionBase *IntuitionBase)
{
    struct Task *task;
    APTR    stack;

    task = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (task)
    {
        stack = AllocMem(MENUTASK_STACKSIZE, MEMF_PUBLIC);
        if (stack)
        {
            NEWLIST(&task->tc_MemEntry);
            task->tc_Node.ln_Type = NT_TASK;
            task->tc_Node.ln_Name = MENUTASK_NAME;
            task->tc_Node.ln_Pri = MENUTASK_PRIORITY;

            task->tc_SPLower=stack;
            task->tc_SPUpper=(BYTE *)stack + MENUTASK_STACKSIZE;

#ifdef __MORPHOS__
            task->tc_SPReg = task->tc_SPUpper;

            {
                struct TaskInitExtension taskext;
                struct TagItem tags[4];

                taskext.Trap = TRAP_PPCTASK;
                taskext.Extension = 0;
                taskext.Tags = tags;

                tags[0].ti_Tag = TASKTAG_CODETYPE;
                tags[0].ti_Data = CODETYPE_PPC;
                tags[1].ti_Tag = TASKTAG_PC;
                tags[1].ti_Data = (ULONG)DefaultMenuHandler;
                tags[2].ti_Tag = TASKTAG_PPC_ARG1;
                tags[2].ti_Data = (ULONG)taskparams;
                tags[3].ti_Tag = TAG_END;

                if(AddTask(task, (APTR)&taskext, NULL) != NULL)
                {
                    /* Everything went OK */
                    return (task);
                }
            }
#else

#if AROS_STACK_GROWS_DOWNWARDS
task->tc_SPReg = (BYTE *)task->tc_SPUpper-SP_OFFSET - sizeof(APTR);
            ((APTR *)task->tc_SPUpper)[-1] = taskparams;
#else
task->tc_SPReg=(BYTE *)task->tc_SPLower-SP_OFFSET + sizeof(APTR);
            *(APTR *)task->tc_SPLower = taskparams;
#endif

            if(AddTask(task, DefaultMenuHandler, NULL) != NULL)
            {
                /* Everything went OK */
                return (task);
            }
#endif
            FreeMem(stack, MENUTASK_STACKSIZE);

        } /* if(stack != NULL) */
        FreeMem(task,sizeof(struct Task));

    } /* if (task) */
    return (NULL);

}

#undef DefaultMenuHandler


/**************************************************************************************************/

/***************************
**  DefaultMenuHandler()  **
***************************/
void DefaultMenuHandler(struct MenuTaskParams *taskparams)
{
    struct IntuitionBase    *IntuitionBase = taskparams->intuitionBase;

    struct MenuHandlerData  *mhd = NULL;
    UBYTE           *mem;
    struct MsgPort      *port = NULL;
    struct MsgPort      TimerPort;
    struct timerequest  *timerio = NULL;
    ULONG  mpmask = 0, timermask = 0;

    BOOL            success = FALSE;
    BOOL            timeron = FALSE;

    if ((mem = AllocMem(sizeof(struct MsgPort) +
                sizeof(struct MenuHandlerData), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        port = (struct MsgPort *)mem;

        port->mp_SigBit     = AllocSignal(-1);
        if (port->mp_SigBit != (UBYTE) -1)
        {
            port->mp_Node.ln_Type   = NT_MSGPORT;
            port->mp_Flags      = PA_SIGNAL;
            port->mp_SigTask    = FindTask(NULL);
            NEWLIST(&port->mp_MsgList);

            mpmask = 1L << port->mp_SigBit;

            mhd = (struct MenuHandlerData *)(mem + sizeof(struct MsgPort));

            success = TRUE;
        }

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

        if (mem)
        {
            FreeMem(mem, sizeof(struct MsgPort) + sizeof(struct MenuHandlerData));
        }

        Wait(0);
    }

    D(bug("DefaultMenuHandler: initialization ok. Now waiting for messages from Intuition.\n"));

    for(;;)
    {
        ULONG sigs;
        struct MenuMessage *msg;

        sigs = Wait(mpmask | timermask);

        D(bug("DefaultMenuHandler: got sigmask 0x%lx (timermask %lx)\n",sigs,timermask));

        if (sigs & mpmask)
            while((msg = GetMenuMessage(port, IntuitionBase)))
            {
                switch(msg->code)
                {
                case MMCODE_START:

#ifdef USEWINDOWLOCK
                    // let's prevent other windows from opening while menus are on
                    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
                    mhd->windowlock = TRUE;
#endif

                    mhd->win            = msg->win;
                    mhd->scr            = mhd->win->WScreen;
                    mhd->dri            = (struct IntDrawInfo *)GetScreenDrawInfo(mhd->scr);

                    mhd->menu           = msg->win->MenuStrip;

                    /* lend menus */
                    if (((struct IntWindow *)msg->win)->menulendwindow)
                    {
                        mhd->menu = ((struct IntWindow *)msg->win)->menulendwindow->MenuStrip;
                    }

                    mhd->scrmousex      = mhd->scr->MouseX;
                    mhd->scrmousey      = mhd->scr->MouseY;
                    mhd->firstmenupick  = MENUNULL;
                    mhd->keepmenuup     = TRUE;

                    mhd->backfillhook.h_Entry = (HOOKFUNC)HookEntry;
                    mhd->backfillhook.h_SubEntry = (HOOKFUNC)CustomizeBackfillFunc;
                    mhd->backfillhook.h_Data = &mhd->hookdata;
                    mhd->hookdata.intuitionBase = IntuitionBase;
                    mhd->isundermouse = FALSE;

                    mhd->openseconds = msg->ie.ie_TimeStamp.tv_secs;
                    mhd->openmicros = msg->ie.ie_TimeStamp.tv_micro;

                    mhd->delayedopen = 0;

                    {
                        if ((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_MENUUNDERMOUSE) || ((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_MENUMOUSEPOS) && (mhd->scr->MouseY >= mhd->scr->BarHeight) ))
                            mhd->isundermouse = TRUE;
                    }

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
                        case IECLASS_RAWKEY:
                            HandleRawKey(&msg->ie,mhd,IntuitionBase);
                            break;

/*                        case IECLASS_TIMER:
                            if (mhd->delayedopen)
                            {
                                UQUAD currenttime,delaytime;

                                currenttime = ((UQUAD)msg->ie.ie_TimeStamp.tv_secs) * 50;
                                currenttime += msg->ie.ie_TimeStamp.tv_micro / 20000;

                                delaytime = ((UQUAD)mhd->delayedopenseconds) * 50;
                                delaytime += mhd->delayedopenmicros / 20000;

                                if (currenttime >= delaytime + 10)
                                {
                                    CreateMenuWindow(mhd->delayedopen,mhd->scr,(struct MsgPort *)-1,CalcSubWindowXPos(mhd->delayedopen->parent,IntuitionBase),CalcSubWindowYPos(mhd->delayedopen->parent,IntuitionBase),FALSE,IntuitionBase);
                                    mhd->delayedopen = 0;
                                }

                            }
                            break;  */

#ifdef __MORPHOS__
                        case IECLASS_NEWTIMER:
                            if (mhd->delayedopen)
                            {
                                if (WindowsReplied(mhd->scr,IntuitionBase))
                                {
                                    CreateMenuWindow(mhd->delayedopen,mhd->scr,(struct MsgPort *)-1,CalcSubWindowXPos(mhd->delayedopen->parent,IntuitionBase),CalcSubWindowYPos(mhd->delayedopen->parent,IntuitionBase),FALSE,IntuitionBase);
                                    mhd->delayedopen = 0;
                                }
                            }
                            break;
#endif /* __MORPHOS__ */
                        }

                    } /* if (mhd->active) */
                    break;

                case MMCODE_STARTCLOCK:
                    if (!timeron && (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENBARCLOCK))
                    {
                        TimerPort.mp_Node.ln_Type   = NT_MSGPORT;
                        TimerPort.mp_Flags      = PA_SIGNAL;
                        TimerPort.mp_SigBit     = AllocSignal(-1);
                        TimerPort.mp_SigTask    = FindTask(0);
                        NEWLIST(&TimerPort.mp_MsgList);

                        timermask = 1L << TimerPort.mp_SigBit;

                        if ((timerio = CreateIORequest(&TimerPort,sizeof(struct timerequest))))
                        {

                            if (!OpenDevice("timer.device",UNIT_VBLANK,(struct IORequest *)timerio,0))
                            {
                                timerio->tr_node.io_Command = TR_ADDREQUEST;
                                timerio->tr_time.tv_secs = 0; // let's start to tick ;)
                                timerio->tr_time.tv_micro = 1;
                                SendIO((struct IORequest *)timerio);
                                timeron = TRUE;
                            }
                            else
                            {
                                DeleteIORequest(timerio);
                            }
                        }

                        if (!timeron) FreeSignal(TimerPort.mp_SigBit);
                    } else if (timeron)
                    {
                        WaitIO((struct IORequest *)timerio);
                        timerio->tr_node.io_Command = TR_ADDREQUEST;
                        timerio->tr_time.tv_secs = CLOCKTICKS;
                        timerio->tr_time.tv_micro = 0;

                        D(bug("DefaultMenuHandler: starting new SendIO()\n"));

                        //jDc: tick only when user wants us to tick
                        if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENBARCLOCK)
                        SendIO((struct IORequest *)timerio);
                    }
                    break;

                } /* switch(msg->code) */

                ReplyMenuMessage(msg, IntuitionBase);

            } /* while((msg = (struct MenuMessage *)GetMsg(port))) */

        D(bug("DefaultMenuHandler: Checking timermask\n"));

        if (sigs & timermask && timeron)
        {
            D(bug("DefaultMenuHandler: WaitIO()\n"));

            WaitIO((struct IORequest *)timerio);

            timerio->tr_node.io_Command = TR_ADDREQUEST;
            timerio->tr_time.tv_secs = CLOCKTICKS;
            timerio->tr_time.tv_micro = 0;

            D(bug("DefaultMenuHandler: starting new SendIO()\n"));

            //jDc: tick only when user wants us to tick
            if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENBARCLOCK)
            SendIO((struct IORequest *)timerio);

            D(bug("DefaultMenuHandler: Rendering ScreenBar clock\n"));

            if (IntuitionBase->FirstScreen) //there are some opened screens
                    RenderScreenBarClock(NULL,IntuitionBase);
        }

        D(bug("DefaultMenuHandler: Restarting loop.\n"));

    } /* for(;;) */
}

/**************************************************************************************************/

/*******************************
**  InitDefaultMenuHandler()  **
*******************************/
BOOL InitDefaultMenuHandler(struct IntuitionBase *IntuitionBase)
{
    struct MenuTaskParams   params;
    struct Task         *task;
    BOOL            result = FALSE;

    params.intuitionBase = IntuitionBase;
    params.Caller    = FindTask(NULL);
    params.success   = FALSE;

    SetSignal(0, SIGF_INTUITION);

    if ((task = CreateMenuHandlerTask(&params, IntuitionBase)))
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

void HandleMouseMove(struct MenuHandlerData *mhd, struct IntuitionBase *IntuitionBase)
{
    struct Layer    *lay = 0;
    struct Window   *win = NULL;
    struct SmallMenuEntry *sel = 0;

    mhd->scrmousex = mhd->scr->MouseX;
    mhd->scrmousey = mhd->scr->MouseY;

    LockLayerInfo(&mhd->scr->LayerInfo);
    lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey);
    UnlockLayerInfo(&mhd->scr->LayerInfo);

    if (lay)
    {
        win = (struct Window *)lay->Window;

        if (win)
        {
            sel = FindEntryWindow(win,mhd->entries,IntuitionBase);
            if (sel)
            {
                struct SmallMenuEntry *work;

                work = sel->parent ? sel->parent->submenu : mhd->entries;

                for (; work; work = work->next)
                {
                    if ((work != sel) && (work->flags & SMF_SELECTED))
                    {
                        work->flags &= ~SMF_SELECTED;
                        RenderItem(work->smk->window->RPort,work,REALX(work),REALY(work),IntuitionBase);
                        if (work->submenu) CloseMenuWindow(work->submenu,IntuitionBase);
                        mhd->delayedopen = 0;
                    }
                }

                if (!(sel->flags & SMF_NOTSELECTABLE))
                {
                    sel->flags |= SMF_SELECTED;
                    RenderItem(sel->smk->window->RPort,sel,REALX(sel),REALY(sel),IntuitionBase);

                    if ((sel->smk->flags & SMK_NEEDSDELAY) && sel->submenu)
                    {
                        if (sel->submenu != mhd->delayedopen)
                        {
                            mhd->delayedopenseconds = IntuitionBase->Seconds;
                            mhd->delayedopenmicros = IntuitionBase->Micros;
                            mhd->delayedopen = sel->submenu;
                        }
                    } else {
                        if (sel->submenu) CreateMenuWindow(sel->submenu,mhd->scr,(struct MsgPort *)-1,CalcSubWindowXPos(sel->submenu->parent,IntuitionBase),CalcSubWindowYPos(sel->submenu->parent,IntuitionBase),FALSE,IntuitionBase);
                    }
                }
            }
        }
    }

    if (!win || !sel)
    {
        sel = FindLastEntry(mhd->entries,IntuitionBase);
        if (sel && !sel->submenu)
        {
            sel->flags &= ~SMF_SELECTED;
            RenderItem(sel->smk->window->RPort,sel,REALX(sel),REALY(sel),IntuitionBase);
        }
    }
}

/**************************************************************************************************/

#define STICKY (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_STICKYMENUS)
#define DELAYEDSTICKY (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_STICKYDELAY)
#define DELAYVALID (DoubleClick(mhd->openseconds,mhd->openmicros,ie->ie_TimeStamp.tv_secs,ie->ie_TimeStamp.tv_micro))

void HandleMouseClick(struct InputEvent *ie, struct MenuHandlerData *mhd,
                             struct IntuitionBase *IntuitionBase)
{
    BOOL die = TRUE;

    switch (ie->ie_Code)
    {
        case MENUUP:
        case SELECTDOWN:
            {
                struct Layer *lay;

                LockLayerInfo(&mhd->scr->LayerInfo);
                lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey);
                UnlockLayerInfo(&mhd->scr->LayerInfo);

                if (lay)
                {
                    struct Window *win = (struct Window *)lay->Window;

                    if (win)
                    {
                        struct SmallMenuEntry *sel = FindEntryWindow(win,mhd->entries,IntuitionBase);
                        if (sel)
                        {
                            if (sel->submenu || (sel->flags & SMF_NOTSELECTABLE) || !sel->parent)
                            {
                                if (STICKY) die = FALSE;
                                if (DELAYEDSTICKY && DELAYVALID) die = TRUE;
                                mhd->keepmenuup = FALSE;
                                break;
                            }

                            DEBUG_CLICK(dprintf("HandleMouseClick: AddToSelection\n"));

                            if (sel->flags & SMF_CHECKMARK)
                            {
                                HandleCheckItem(mhd,sel,IntuitionBase);
                            }

                            AddToSelection(mhd,sel,IntuitionBase);


#ifdef NOCBMPATENTS
                            if ((ie->ie_Code == SELECTDOWN) && (ie->ie_Qualifier & IEQUALIFIER_RBUTTON))
                            {
                                die = FALSE;
                            }
#endif

                            break;
                        }
                    }
                }
                if (mhd->keepmenuup && STICKY) die = FALSE;
                if (DELAYEDSTICKY && DELAYVALID) die = TRUE;
                mhd->keepmenuup = FALSE;
            }
            break;

        default:
            if (mhd->keepmenuup) die = FALSE;
            mhd->keepmenuup = FALSE;
            break;
    }

    if (die)
    {
        KillMenus(mhd,IntuitionBase);
    }
 
};

/**************************************************************************************************/

BOOL HandleCheckItem(struct MenuHandlerData *mhd, struct SmallMenuEntry *entry,
                            struct IntuitionBase *IntuitionBase)
{
    /* Note: If you change something here, you probably must also change
         menus.c/CheckMenuItemWasClicked() which is used when the
      user uses the menu key shortcuts! */

    struct MenuItem *item = (struct MenuItem *)entry->reference;
    struct SmallMenuEntry *work;
    BOOL re_render = FALSE;
    BOOL changed = FALSE;

    if (item->Flags & MENUTOGGLE)
    {
        item->Flags ^= CHECKED;
        entry->flags ^= SMF_CHECKED;
        re_render = TRUE;
        changed = TRUE;
    }
    else
    {
        if (!(item->Flags & CHECKED))
        {
            item->Flags |= CHECKED;
            entry->flags |= SMF_CHECKED;
            re_render = TRUE;
            changed = TRUE;
        } else {
            if (!(item->MutualExclude))
            {
                item->Flags &= ~CHECKED;
                entry->flags &= ~SMF_CHECKED;
                re_render = TRUE;
                changed = TRUE;
            }
        }
    }

    if (re_render)
    {
        RenderItem(entry->smk->window->RPort,entry,REALX(entry),REALY(entry),IntuitionBase);
    }

    if (item->MutualExclude)
    {
        struct MenuItem *checkitem = 0;
        WORD        i,itemnum = entry->referencenumber;

        work = entry->parent->submenu;

        for(i = 0; (i < 32) && work; i++, work = work->next)
        {
            checkitem = work->reference;
            if ((i != itemnum) && (item->MutualExclude & (1L << i)) &&
                ((checkitem->Flags & (CHECKED | CHECKIT)) == (CHECKIT | CHECKED)))
            {
                checkitem->Flags &= ~CHECKED;
                work->flags &= ~SMF_CHECKED;
                RenderItem(work->smk->window->RPort,work,REALX(work),REALY(work),IntuitionBase);
            }
        }

    } /* if (item->MutualExclude) */

    return changed;
}

/**************************************************************************************************/
 
/**************************************************************************************************/
void AddToSelection(struct MenuHandlerData *mhd, struct SmallMenuEntry *entry, struct IntuitionBase *IntuitionBase)
{
    ULONG menunum=-1,menuitemnum=-1,submenunum=-1;
    struct SmallMenuEntry *work;

    DEBUG_ADDTO(dprintf("AddToSelection: called\n"));

    if (entry)
    {
        //check if we're not disabled (checking whole family isn't really necessary, but oh well... ;)
        for (work = entry; work; work = work->parent)
        {
            if (work->flags & SMF_DISABLED) return;
        }
    }

    DEBUG_ADDTO(dprintf("AddToSelection: disable passed ok\n"));

    /*
    Now we need to get the FULLMENUNUM. god, this cbm-crap-menu-API sucks so much!!!
    too bad this doesn't allow unlimited menu depth :(
    */

    if (entry->parent->flags & SMF_MENUSTRIP)
    {
        menuitemnum = entry->referencenumber;
        menunum = entry->parent->referencenumber;
    } else {
        submenunum = entry->referencenumber;
        menuitemnum = entry->parent->referencenumber;
        if (entry->parent->parent) //null should never happen here anyway, but.. ;)
        {
            menunum = entry->parent->parent->referencenumber;
        }
    }

    DEBUG_ADDTO(dprintf("AddToSelection: menu %ld menuitem %ld submenuitem %ld\n",menunum,menuitemnum,submenunum));

    if ((menunum != -1) && (menuitemnum != -1))
    {
        struct MenuItem *item = NULL;
        UWORD men = FULLMENUNUM(menunum, menuitemnum, submenunum);

        item = (struct MenuItem *)entry->reference;

        if (item && (ItemAddress(mhd->menu, men) == item))
        {
            UWORD men = FULLMENUNUM(menunum, menuitemnum, submenunum);

            DEBUG_ADDTO(dprintf("AddToSelection: adding item %lx\n",item));

            if (mhd->firstmenupick == MENUNULL)
            {
                mhd->firstmenupick = men;
            }
            else if (men != mhd->lastmenupick)
            {
                struct MenuItem *checkitem, *prevcheckitem = NULL;
                UWORD checkmen = mhd->firstmenupick;

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

    } /* if ((menunum != -1) && (menuitemnum != -1)) */
}


/**************************************************************************************************/

