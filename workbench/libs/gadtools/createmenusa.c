/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>

	AROS_LH2(struct Menu *, CreateMenusA,

/*  SYNOPSIS */
	AROS_LHA(struct NewMenu *, newmenu, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 8, GadTools)

/*  FUNCTION
	CreateMenusA() creates a complete menu or parts of a menu.

    INPUTS
	newmenu - pointer to struct NewMenu
	taglist - additional tags

    RESULT
	A pointer to a menu structure.

    NOTES
	CreateMenusA() stores no position information in the menu structure.
	You need to call LayoutMenusA() to retrieve them.
	The strings supplied for the menu are not copied into a private
	buffer. Therefore they must be preserved, until FreeMenus() was
	called.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeMenus(), LayoutMenusA()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    return NULL;

    AROS_LIBFUNC_EXIT
} /* CreateMenusA */
