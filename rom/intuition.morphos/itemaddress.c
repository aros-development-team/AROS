/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

#define DEBUG_ITEMADDRESS(x)    ;
#define DEBUG_ITEMADDRESS2(x)   ;

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(struct MenuItem *, ItemAddress,

         /*  SYNOPSIS */
         AROS_LHA(struct Menu *, menustrip, A0),
         AROS_LHA(UWORD        , menunumber, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 24, Intuition)

/*  FUNCTION
    Returns the address of the menuitem 'menunumber' of 'menustrip'.
    The number is the one you get from intuition after the user has
    selected a menu.
    The menunumber must be well-defined.
    Valid numbers are MENUNULL, which makes the routine return NULL,
    or valid item number of your menustrip, which contains
    - a valid menu number
    - a valid item number
    - if the menu-item has a sub-item, a valid sub-item number
    Menu number and item number must be specified. Sub-item, if
    available, is optional, therefore this function returns either
    the item or sub-item.
 
    INPUTS
    menustrip - Pointer to the first menu of the menustrip.
    menunumber - Packed value describing the menu, item and if
        appropriate sub-item.
 
    RESULT
    Returns NULL for menunumber == MENUNULL or the address of the
    menuitem described by menunumber.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Menu     *thismenu;
    struct MenuItem *thisitem = NULL;
    int     	     i;

    DEBUG_ITEMADDRESS(dprintf("ItemAddress: Strip %p Number 0x%lx (%d/%d/%d)\n",
                              menustrip, menunumber, MENUNUM(menunumber),
                              ITEMNUM(menunumber), SUBNUM(menunumber)));

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    SANITY_CHECKR(menustrip,NULL)

    if ( menunumber != MENUNULL )
    {
        thismenu = menustrip;
        DEBUG_ITEMADDRESS2(dprintf("ItemAddress: Menu %p\n", thismenu));

        for ( i = 0 ; thismenu && i < MENUNUM ( menunumber ) ; i++ )
        {
            thismenu = thismenu->NextMenu;
            DEBUG_ITEMADDRESS2(dprintf("ItemAddress: Menu %p\n", thismenu));
        }

        if (thismenu)
        {
            thisitem = thismenu->FirstItem;
            DEBUG_ITEMADDRESS2(dprintf("ItemAddress: Item %p\n", thisitem));

            for ( i = 0 ; thisitem && i < ITEMNUM ( menunumber ) ; i++ )
            {
                thisitem = thisitem->NextItem;
                DEBUG_ITEMADDRESS2(dprintf("ItemAddress: Item %p\n", thisitem));
            }

            if (thisitem && ( SUBNUM ( menunumber ) != NOSUB ) && thisitem->SubItem )
            {
                thisitem = thisitem->SubItem;
                DEBUG_ITEMADDRESS2(dprintf("ItemAddress: SubItem %p\n", thisitem));

                for ( i = 0 ; thisitem && i < SUBNUM ( menunumber ) ; i++ )
                {
                    DEBUG_ITEMADDRESS2(dprintf("ItemAddress: SubItem %p\n", thisitem));
                    thisitem = thisitem->NextItem;
                }
            }
        }
    }

    DEBUG_ITEMADDRESS(dprintf("ItemAddress: return %p\n", thisitem));

    return thisitem;

    AROS_LIBFUNC_EXIT
} /* ItemAddress */
