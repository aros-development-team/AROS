/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <exec/types.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>

        AROS_LH3(BOOL, LayoutMenuItemsA,

/*  SYNOPSIS */
	AROS_LHA(struct MenuItem *, menuitem, A0),
	AROS_LHA(APTR, vi, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, GadToolsBase, 10, GadTools)

/*  FUNCTION

    INPUTS
	menuitem - Menu item to be layouted.
	vi -       Visual info to layout the menu item for.
	tagList  - Additional tags.

    RESULT
	FALSE, if an error occured.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	LayoutMenusA(), GetVisualInfoA()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct VisualInfo	*vinfo = (struct VisualInfo *)vi;
    struct MenuItem 	*item = menuitem;

    /* First layout subitems */

    while (NULL != item)
    {
	if (NULL != item->SubItem)
	{
	    if (FALSE == layoutsubitems(item,
                                	vinfo,
                                	tagList,
                                	GTB(GadToolsBase)))
                return FALSE;
	}
	item = item->NextItem;
    }

    /*
    ** Process all menu items and subitems
    */
    if (FALSE == layoutmenuitems(menuitem,
                        	 vinfo,
                        	 tagList,
                        	 GTB(GadToolsBase)))
	return FALSE;


    return TRUE;  

    AROS_LIBFUNC_EXIT
    
} /* LayoutMenuItemsA */
