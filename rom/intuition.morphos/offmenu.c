/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function OffMenu()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>

AROS_LH2(void, OffMenu,

         /*  SYNOPSIS */
         AROS_LHA(struct Window    *, window, A0),
         AROS_LHA(UWORD             , menunumber, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 30, Intuition)

/*  FUNCTION
    Disable a whole menu, an item or a sub-item depending on
    the menunumber.
 
    INPUTS
    window - The window, the menu belongs to
    menunumber - The packed information on what piece of menu to disable
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    OnMenu(), ResetMenuStrip()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    int i;
    struct Menu * thismenu;
    struct MenuItem * thisitem;

    DEBUG_OFFMENU(dprintf("OffMenu: Window 0x%lx MenuNumber 0x%lx\n", window, menunumber));

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    thismenu = window->MenuStrip;
    if (MENUNUM(menunumber) != NOMENU)
    {
        for (i = 0; i < MENUNUM(menunumber) && thismenu; i++)
        {
            thismenu = thismenu->NextMenu;
        }

        if (thismenu)
        {
            if (ITEMNUM(menunumber) == NOITEM)
            {
                thismenu->Flags &= ~MENUENABLED;
            }
            else
            {
                thisitem = thismenu->FirstItem;
                for (i = 0; i < ITEMNUM(menunumber) && thisitem; i++)
                {
                    thisitem = thisitem->NextItem;
                }
                if (thisitem)
                {
                    if (SUBNUM(menunumber) != NOSUB)
                    {
                        thisitem = thisitem->SubItem;
                        for (i = 0; i < SUBNUM(menunumber) && thisitem; i++)
                        {
                            thisitem = thisitem->NextItem;
                        }
                    }
                }
                if (thisitem)
                {
                    thisitem->Flags &= ~ITEMENABLED;
                }
            }
        }
    }

    AROS_LIBFUNC_EXIT
} /* OffMenu */
