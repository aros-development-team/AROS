/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ItemAddress()
    Lang: english
*/
#include "intuition_intern.h"

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
int i;
struct Menu * thismenu;
struct MenuItem * thisitem;

    if ( menunumber == MENUNULL || menustrip == NULL )
    {
	return NULL;
    }
    else
    {
	thismenu = menustrip;
	for ( i = 0 ; i < MENUNUM ( menunumber ) ; i++ )
	{
	    thismenu = thismenu->NextMenu;
	}
	thisitem = thismenu->FirstItem;
	for ( i = 0 ; i < ITEMNUM ( menunumber ) ; i++ )
	{
	    thisitem = thisitem->NextItem;
	}
	if ( ( SUBNUM ( menunumber ) != NOSUB ) && thisitem->SubItem )
	{
	    thisitem = thisitem->SubItem;
	    for ( i = 0 ; i < SUBNUM ( menunumber ) ; i++ )
	    {
		thisitem = thisitem->NextItem;
	    }
	}
    }

    return thisitem;

    AROS_LIBFUNC_EXIT
} /* ItemAddress */
