/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>

AROS_LH2(void, OnMenu,

         /*  SYNOPSIS */
         AROS_LHA(struct Window    *, window, A0),
         AROS_LHA(UWORD             , menunumber, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 32, Intuition)

/*  FUNCTION
    Enable a whole menu, an item or a sub-item depending on
    the menunumber.
 
    INPUTS
    window - The window, the menu belongs to
    menunumber - The packed information on what piece of menu to enable
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    OffMenu(), ResetMenuStrip()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    int i;
    struct Menu * thismenu;
    struct MenuItem * thisitem;

    DEBUG_ONMENU(dprintf("OnMenu: Window 0x%lx MenuNumber 0x%lx\n", window, menunumber));

    SANITY_CHECK(window)

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
                thismenu->Flags |= MENUENABLED;
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
                    thisitem->Flags |= ITEMENABLED;
                }
            }
	    
        } /* if (thismenu) */
	
    } /* if (MENUNUM(menunumber) != NOMENU) */

    AROS_LIBFUNC_EXIT
} /* OnMenu */
