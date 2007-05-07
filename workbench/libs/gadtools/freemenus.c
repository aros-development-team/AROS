/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free memory allocated by CreateMenusA()
    Lang: english
*/
#include <proto/exec.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>

	AROS_LH1(VOID, FreeMenus,

/*  SYNOPSIS */
	AROS_LHA(struct Menu *, menu, A0),

/*  LOCATION */
	struct Library *, GadToolsBase, 9, GadTools)

/*  FUNCTION
	Frees the menus allocated by CreateMenusA().

    INPUTS
	menu - pointer to the menu (or first MenuItem) to be freed, may be NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreateMenusA()

    INTERNALS

    HISTORY

***************************************************************************/
#if NEWMENUCODE
{
    AROS_LIBFUNC_INIT

    DEBUG_FREEMENUS(bug("FreeMenus: menu %p\n", menu));
    /*
     * ULONG BarLabelTable[BarLabelCount]
     * ULONG BarLabelCount
     * ULONG Menu==0|Item!=0
     * Menu:
     */
    if (menu)
    {
	ULONG *p = (ULONG *)menu - 1;
	ULONG BarLabels;
	int i;

	DEBUG_FREEMENUS(bug("FreeMenus: p 0x%lx\n", *p));
	DEBUG_FREEMENUS(bug("FreeMenus: NextMenu 0x%lx\n", menu->NextMenu));
	DEBUG_FREEMENUS(bug("FreeMenus: Left %ld Top %ld Width %ld Height %ld\n", menu->LeftEdge,menu->TopEdge,menu->Width,menu->Height));
	DEBUG_FREEMENUS(bug("FreeMenus: Flags 0x%lx\n", menu->Flags));
	DEBUG_FREEMENUS(bug("FreeMenus: MenuName <%s> FirstItem 0x%lx\n", menu->MenuName,menu->FirstItem));

	p = p - 1;
	/*
	 * p points now on BarLabelCount
	 */
	BarLabels	=	*p;
	DEBUG_FREEMENUS(bug("FreeMenus: BarLabels %ld\n", BarLabels));
	for (i=0;i<BarLabels;i++)
	{
		if (*--p)
		{
			DEBUG_FREEMENUS(bug("FreeMenus: Free Image 0x%lx\n", *p));
			DisposeObject((APTR) *p);
		}
	}
	DEBUG_FREEMENUS(bug("FreeMenus: Free MenuMem 0x%lx\n", p));
	FreeVec(p);
    }
    DEBUG_FREEMENUS(bug("FreeMenus: done\n"));

    AROS_LIBFUNC_EXIT
} /* FreeMenus */
#else
{
    AROS_LIBFUNC_INIT

    DEBUG_FREEMENUS(bug("FreeMenus: menu %p\n", menu));

    DumpMenu(menu);
    if (menu)
    {
	ULONG *p = (ULONG *)menu - 1;

	DEBUG_FREEMENUS(bug("FreeMenus: NextMenu 0x%lx\n", menu->NextMenu));
	DEBUG_FREEMENUS(bug("FreeMenus: Left %ld Top %ld Width %ld Height %ld\n", menu->LeftEdge,menu->TopEdge,menu->Width,menu->Height));
	DEBUG_FREEMENUS(bug("FreeMenus: Flags 0x%lx\n", menu->Flags));
	DEBUG_FREEMENUS(bug("FreeMenus: MenuName <%s> FirstItem 0x%lx\n", menu->MenuName,menu->FirstItem));
	/*
	 * Check if we are freeing a Menu or a MenuItem list.
	 */
	if (*p)
	{
	    DEBUG_FREEMENUS(bug("FreeMenus: menuitem list\n"));
	    freeitems((struct MenuItem *)menu, (struct GadToolsBase_intern *)GadToolsBase);
	}
	else
	{
	    DEBUG_FREEMENUS(bug("FreeMenus: menu list\n"));
	    while (NULL != menu)
	    {
		struct Menu * _menu;

		_menu = menu->NextMenu;
		DEBUG_FREEMENUS(bug("FreeMenus: menu 0x%lx nextmenu 0x%lx\n",menu,_menu));

		/*
		 ** Free all items and subitems of this menu title
		 */
		DEBUG_FREEMENUS(bug("FreeMenus: freeitems\n"));
		freeitems(menu->FirstItem, (struct GadToolsBase_intern *)GadToolsBase);

		DEBUG_FREEMENUS(bug("FreeMenus: menu 0x%lx size %ld\n",p,sizeof(ULONG) + sizeof(struct Menu) + sizeof(APTR)));
		FreeMem(p, sizeof(ULONG) + sizeof(struct Menu) + sizeof(APTR));

		menu = _menu;
		p = (ULONG *)menu - 1;
	    }
	}
    }
    DEBUG_FREEMENUS(bug("FreeMenus: done\n"));

    AROS_LIBFUNC_EXIT

} /* FreeMenus */
#endif
