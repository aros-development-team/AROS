/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <string.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(struct Screen *, LockPubScreen,

         /*  SYNOPSIS */
         AROS_LHA(CONST_STRPTR, name, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 85, Intuition)

/*  FUNCTION
 
    Locks a public screen, thus preventing it from closing.
    This is useful if you want to put up a visitor window on a public screen
    and need to check some of the public screen's field first -- not locking
    the screen may lead to the public screen not existing when your visitor
    window is ready.
 
    If you try to lock the Workbench screen or the default public screen
    and there isn't any, the Workbench screen will be automatically opened
    and locked.
 
    INPUTS
 
    Name   --  Name of the public screen or NULL for the default public
               screen. The name "Workbench" refers to the Workbench screen.
 
    RESULT
 
    A pointer to the screen or NULL if something went wrong. Failure can
    happen for instance when the public screen is in private state or doesn't
    exist.
 
    NOTES
 
    You don't need to hold the lock when your visitor window is opened as
    the pubscreen cannot be closed as long as there are visitor windows
    on it.
 
    EXAMPLE
 
    To open a visitor window which needs information from the screen structure
    of the public screen to open on, do this:
 
    if((pubscreen = LockPubScreen("PubScreentoOpenon")) != NULL)
    {
        ...check pubscreen's internal data...
    OpenWindow(VisitorWindow, pubscreen);
    UnlockPubScreen(NULL, pubscreen);
    ...use your visitor window...
    CloseWindow(VisitorWindow);
    }
 
    BUGS
 
    SEE ALSO
 
    OpenWindow(), UnlockPubScreen(), GetScreenData()
 
    INTERNALS
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *screen = NULL;
    struct List   *list;

    DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: name <%s>\n",
                                name ? name : (CONST_STRPTR)"NULL"));
    DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: task %p <%s>\n",
                                SysBase->ThisTask,
                                SysBase->ThisTask->tc_Node.ln_Name ? SysBase->ThisTask->tc_Node.ln_Name : "NULL"));

    list = LockPubScreenList();

    if( !name )
    {

        screen = GetPrivIBase(IntuitionBase)->DefaultPubScreen;

        /* If IntuitionBase->DefaultPubScreen is NULL, then Workbench screen
           is default public screen. But note that, Workbench screen might
           here not be open either. */

        if (!screen) screen = GetPrivIBase(IntuitionBase)->WorkBench;

        if (screen)
        {
            ASSERT_VALID_PTR(screen);
            GetPrivScreen(screen)->pubScrNode->psn_VisitorCount++;
            DEBUG_VISITOR(dprintf("LockPubScreen: 1 screen %p count %ld Task <%s>\n",
                                  screen, GetPrivScreen(screen)->pubScrNode->psn_VisitorCount,
                                  FindTask(NULL)->tc_Node.ln_Name));
            DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: screen %p count %d\n",
                                        screen, GetPrivScreen(screen)->pubScrNode->psn_VisitorCount));
        }

    }
    else
    {
        struct PubScreenNode *psn;
	
        ASSERT_VALID_PTR_ROMOK(name);

        /* Browse the public screen list */
        if( (psn = (struct PubScreenNode *) FindName(list, (UBYTE *)name )) )
        {
            ASSERT_VALID_PTR(psn);

            /* Don't lock screens in private state */
            if( (psn != NULL) && !(psn->psn_Flags & PSNF_PRIVATE) )
            {
                /* Increment screen lock count */
                psn->psn_VisitorCount++;
                screen = psn->psn_Screen;
                DEBUG_VISITOR(dprintf("LockPubScreen: 2 node %p screen %p count %ld <%s>\n",
                                      psn, screen, psn->psn_VisitorCount,
                                      FindTask(NULL)->tc_Node.ln_Name));
                DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: node %p screen %p count %d\n",
                                            psn, screen, psn->psn_VisitorCount));
                ASSERT_VALID_PTR(screen);
            }
        }

    }

    UnlockPubScreenList();

    /* If no screen was found and the requested one was the Workbench screen or
     * the default public screen, open the Workbench screen and lock it. */
    if( (screen == NULL) && ((name == NULL) || (strcmp( name, "Workbench" ) == 0)) )
    {
        OpenWorkBench();
        DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: opened workbench\n"));

        LockPubScreenList();

        screen = GetPrivIBase(IntuitionBase)->WorkBench;
        if (!screen)
        {
            struct PubScreenNode *psn;

            /* Maybe something patched OpenWorkbench, and there is a 'Workbench'
             * screen in the list. Our private pointer is just not set. */
            if( (psn = (struct PubScreenNode *) FindName(list, "Workbench" )) )
            {
                /* Don't lock screens in private state */
                if( (psn != NULL) && !(psn->psn_Flags & PSNF_PRIVATE) )
                    screen = psn->psn_Screen;
            }
        }

        if( screen )
        {
            ASSERT_VALID_PTR(screen);
            GetPrivScreen(screen)->pubScrNode->psn_VisitorCount++;
            DEBUG_VISITOR(dprintf("LockPubScreen: 3 screen %p count %d <%s>\n",
                                  screen, GetPrivScreen(screen)->pubScrNode->psn_VisitorCount,
                                  FindTask(NULL)->tc_Node.ln_Name));

            DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: screen %p count %d\n",
                                        screen, GetPrivScreen(screen)->pubScrNode->psn_VisitorCount));
        }

        UnlockPubScreenList();
    }

    DEBUG_LOCKPUBSCREEN(dprintf("LockPubScreen: return %p\n", screen));

    return screen;

    AROS_LIBFUNC_EXIT
} /* LockPubScreen */
