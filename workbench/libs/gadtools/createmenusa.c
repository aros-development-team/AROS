/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/
#include "gadtools_intern.h"
#include <exec/memory.h>
#include <exec/types.h>
#define DEBUG 0

#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>

#include <aros/debug.h>

#include <proto/gadtools.h>

/*********************************************************************

    NAME */

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
#if NEWMENUCODE
{
/*
 * We need to allocate the whole menu in one block as we can't just
 * go through the menustructure in freemenus as programs may do dynamic
 * menu links which would be illegally freeed that way.
 * As barlabels are dynamic Boopsi image objects we need to store them
 * in a menu pregap so we can free them through DisposeObject() before
 * we free the whole menublock.
 *
 * Menu Memory structure:
 *  void *BarLabelTable[BarLabelCount];
 *  ULONG BarLabelCount;
 *  Menu(returned Ptr):
 *   MenuStructure
 *   .
 *   .
 */


    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    struct Menu		* firstmenu = NULL, * curmenu = NULL;
    struct MenuItem	* newitem, * firstitem = NULL, * curitem = NULL;
    struct MenuItem	* newsubitem, * firstsubitem = NULL, * cursubitem = NULL;
    struct TagItem	* ti;
    ULONG		menu_ctr = 0;
    ULONG		item_ctr = 0;
    ULONG		subitem_ctr = 0;
    BOOL		end;
    BOOL		fullmenu = GetTagData(GTMN_FullMenu, FALSE, tagList);
    UBYTE		prevtype = (UBYTE)-1;
    ULONG		BarLabels;
    ULONG		MenuMemSize;
    struct NewMenu	*menuentry;
    UBYTE		*MyMenuMemory;
    UBYTE		*MyMenuMemPtr;
    struct Image	**MyBarTablePtr;
    ULONG		err = 0;

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NewMenu %p TagList %p full %d\n",
			    newmenu, tagList, fullmenu));

    D(bug("Entering %s\n",__FUNCTION__));

    BarLabels	=	0;
    MenuMemSize	=	0;
    menuentry	=	newmenu;

    end = FALSE;
    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: count elements\n"));
    while (!end)
    {
	BOOL is_image = FALSE;
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: Type %d\n", menuentry->nm_Type));
	switch (menuentry->nm_Type)
	{
	    case NM_TITLE:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NW_TITLE\n"));
		MenuMemSize += getmenutitlesize(newmenu,tagList);
		break;

	    case IM_ITEM:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_ITEM\n"));
                is_image = TRUE;
                /* Fall through */

	    case NM_ITEM: /* also the BARLABEL */
		/*
		** There has to be a menu structure available
		** to create an item, unless GTMN_FullMenu is FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_ITEM\n"));
		if (menuentry->nm_Label == NM_BARLABEL)
		{
			BarLabels++;
		}
		MenuMemSize += getmenuitemsize(newmenu,
					       is_image,
					       tagList,
					       GTB(GadToolsBase));

		break;

	    case IM_SUB:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_SUB\n"));
		is_image = TRUE;
		/* Fall through */

	    case NM_SUB:
		/*
		** There has to be an item menu structure available
		** to create a sub item, unless GTMN_FullMenu == FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_SUB\n"));
		if (menuentry->nm_Label == NM_BARLABEL)
		{
			BarLabels++;
		}
		MenuMemSize += getmenuitemsize(newmenu,
					       is_image,
					       tagList,
					       GTB(GadToolsBase));
		break;

	    case NM_IGNORE:
		/*
		** Nothing to do in this case
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_IGNORE\n"));
		break;

	    case NM_END:
		/*
		** The end.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_END\n"));
		end = TRUE;
		break;

	} /* switch (menuentry->nm_Type) */
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: Current MenuMemSize %ld\n",MenuMemSize));
	menuentry++;
    } /* while (!end) */

    /*
     * Add space for barlabel count+area which must come at the beginning.
     */
    MenuMemSize += sizeof(ULONG) + (BarLabels * sizeof(void*));

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: MenuMemSize %ld BarLabels %ld\n",MenuMemSize,BarLabels));

    if (!(MyMenuMemory=AllocVec(MenuMemSize,MEMF_ANY | MEMF_CLEAR)))
    {
	ULONG	*Ptr;
	/*
	** Set the secondary error value if requested.
	*/
	if ((Ptr =(ULONG*) GetTagData(GTMN_SecondaryError, (IPTR) NULL, tagList)))
	{
	    *Ptr = GTMENU_NOMEM;
	}
	return(NULL);
    }
    MyBarTablePtr =(struct Image**) MyMenuMemory;
    MyMenuMemPtr =(UBYTE*) &MyBarTablePtr[BarLabels];
    *((ULONG*) MyMenuMemPtr) = BarLabels;
    MyMenuMemPtr += sizeof(ULONG);

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: MyMenuMemory 0x%lx MyBarTablePtr 0x%lx MyMenuMemPtr 0x%lx\n",MyMenuMemory,MyBarTablePtr,MyMenuMemPtr));

    end = FALSE;
    while (!end)
    {
	BOOL is_image = FALSE;

	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: Type %d\n", newmenu->nm_Type));
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: Current MyMenuMemory 0x%lx MyBarTablePtr 0x%lx MyMenuMemPtr 0x%lx\n",MyMenuMemory,MyBarTablePtr,MyMenuMemPtr));

	switch (newmenu->nm_Type)
	{
	    case NM_TITLE:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NW_TITLE\n"));
		curmenu = makemenutitle(newmenu,
                                        &MyMenuMemPtr,
					tagList);

		if (NULL == curmenu)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		/*
		** Append it to the list of menus or make it the
		** first entry.
		*/
		if (NULL == firstmenu)
		    firstmenu = curmenu;
		else
		    appendmenu(firstmenu, curmenu);

		menu_ctr ++;
		item_ctr = 0;
		curitem = firstitem = cursubitem = firstsubitem = NULL;
		break;

	    case IM_ITEM:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_ITEM\n"));
                is_image = TRUE;
                /* Fall through */

	    case NM_ITEM: /* also the BARLABEL */
		/*
		** There has to be a menu structure available
		** to create an item, unless GTMN_FullMenu is FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_ITEM\n"));

		if (fullmenu && (NULL == curmenu))
		{
		    err = GTMENU_INVALID;
		    goto failexit;
		}

		if (item_ctr >= 64)
		{
		    /* CreateMenusA must still return success here, only that the
		       menus will be trimmed */
		    err = GTMENU_TRIMMED;
		    break;
		}

		newitem = makemenuitem(newmenu,
                                       &MyMenuMemPtr,
                                       &MyBarTablePtr,
				       is_image,
				       tagList,
				       GTB(GadToolsBase));

		if (NULL == newitem)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		/*
		** Append this item to the current menu title
		*/

		if (curitem)
		{
		    curitem->NextItem = newitem;
		}
		else
		{
		    firstitem = newitem;
		    if (curmenu)
		    {
			curmenu->FirstItem = newitem;
			DEBUG_CREATEMENUSA(dprintf("CreateMenusA: set FirstItem 0x%lx\n", newitem));
		    }
		}

		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NextItem 0x%lx\n", newitem));

		item_ctr ++;
		subitem_ctr = 0;
		cursubitem = firstsubitem = NULL;

		curitem = newitem;
		break;

	    case IM_SUB:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_SUB\n"));
		is_image = TRUE;
		/* Fall through */

	    case NM_SUB:
		/*
		** There has to be an item menu structure available
		** to create a sub item, unless GTMN_FullMenu == FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_SUB\n"));
		if ( (fullmenu && (NULL == curitem)) ||
		     (prevtype == NM_TITLE) )
		{
		    err = GTMENU_INVALID;
		    goto failexit;
		}

		if (subitem_ctr >= 32)
		{
		    err = GTMENU_TRIMMED;
		    /* CreateMenusA must still return success here, only that the
		       menus will be trimmed */
		    break;
		}

		newsubitem = makemenuitem(newmenu,
                                          &MyMenuMemPtr,
                                          &MyBarTablePtr,
					  is_image,
					  tagList,
					  GTB(GadToolsBase));

		if (NULL == newsubitem)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		if (curitem)
		{
		    if (!curitem->SubItem)
		    {
		    }
		}

		/*
		** Append this item to the current sub menu item
		*/

		if (cursubitem)
		{
		    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: CurSubItem 0x%lx NextItem 0x%lx\n", cursubitem,newsubitem));
		    cursubitem->NextItem = newsubitem;
		}
		else
		{
		    firstsubitem = newsubitem;
		    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: FirstSubItem 0x%lx\n", firstsubitem));
		    if (curitem)
		    {
			curitem->SubItem = newsubitem;

			DEBUG_CREATEMENUSA(dprintf("CreateMenusA: CurItem 0x%lx SubItem 0x%lx\n", curitem,newsubitem));

			/* Add the ">>" mark. Hmm ... maybe if would be better if this
			   was done in LayoutMenus() ??? */

			curitem->Flags &= ~COMMSEQ;
			if (curitem->Flags & ITEMTEXT)
			{
			    struct IntuiText *it = (struct IntuiText *)curitem->ItemFill;

			    if (!it->NextText)
			    {
				struct IntuiText *it2 = it + 1;

				it->NextText = it2;
				it2->IText = "»";
			    }
			}

		    }
		}

		cursubitem = newsubitem;
		subitem_ctr ++;
		break;

	    case NM_IGNORE:
		/*
		** Nothing to do in this case
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_IGNORE\n"));
		break;

	    case NM_END:
		/*
		** The end.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_END\n"));
		end = TRUE;
		break;

	} /* switch (newmenu->nm_Type) */

	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: firstmenu %p curmenu %p firstitem %p curitem %p firstsub %p cursub %p\n",
				firstmenu, curmenu, firstitem, curitem, firstsubitem, cursubitem));

	prevtype = newmenu->nm_Type;

	/*
	** Advance to the next item in the array
	*/
	newmenu = &newmenu[1];

    } /* while (FALSE == end) */

    /*
    ** Set the secondary error value if requested.
    */
    ti = FindTagItem(GTMN_SecondaryError, tagList);

    if (NULL != ti)
        ti->ti_Data = err;

    if (!firstmenu)
    {
	firstmenu = (struct Menu *)firstitem;
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: No FirstMenu..use firstitem 0x%lx\n", firstitem));
    }
    if (!firstmenu)
    {
	firstmenu = (struct Menu *)firstsubitem;
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: No FirstMenu..use firstsubitem 0x%lx\n", firstsubitem));
    }

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: return %p\n", firstmenu));

//    DumpMenu(firstmenu);

    return firstmenu;


failexit:

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: failed\n"));

    /*
    ** Free all memory
    */
    FreeMenus(firstmenu);

    /*
    ** Set the secondary error value if requested.
    */
    ti = FindTagItem(GTMN_SecondaryError, tagList);

    if (NULL != ti)
        ti->ti_Data = err;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* CreateMenusA */

#else
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    struct Menu		* firstmenu = NULL, * curmenu = NULL;
    struct MenuItem	* newitem, * firstitem = NULL, * curitem = NULL;
    struct MenuItem	* newsubitem, * firstsubitem = NULL, * cursubitem = NULL;
    struct TagItem	* ti;
    ULONG		menu_ctr = 0;
    ULONG		item_ctr = 0;
    ULONG		subitem_ctr = 0;
    BOOL		end = FALSE;
    BOOL		fullmenu = GetTagData(GTMN_FullMenu, FALSE, tagList);
    UBYTE		prevtype = (UBYTE)-1;

    ULONG		err = 0;

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NewMenu %p TagList %p full %d\n",
			    newmenu, tagList, fullmenu));

    D(bug("Entering %s\n",__FUNCTION__));

    while (FALSE == end)
    {
	BOOL is_image = FALSE;

	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: Type %d\n", newmenu->nm_Type));

	switch (newmenu->nm_Type)
	{
	    case NM_TITLE:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NW_TITLE\n"));
		curmenu = makemenutitle(newmenu,
					tagList);

		if (NULL == curmenu)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		/*
		** Append it to the list of menus or make it the
		** first entry.
		*/
		if (NULL == firstmenu)
		    firstmenu = curmenu;
		else
		    appendmenu(firstmenu, curmenu);

		menu_ctr ++;
		item_ctr = 0;
		curitem = firstitem = cursubitem = firstsubitem = NULL;
		break;

	    case IM_ITEM:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_ITEM\n"));
                is_image = TRUE;
                /* Fall through */

	    case NM_ITEM: /* also the BARLABEL */
		/*
		** There has to be a menu structure available
		** to create an item, unless GTMN_FullMenu is FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_ITEM\n"));

		if (fullmenu && (NULL == curmenu))
		{
		    err = GTMENU_INVALID;
		    goto failexit;
		}

		if (item_ctr >= 64)
		{
		    /* CreateMenusA must still return success here, only that the
		       menus will be trimmed */
		    err = GTMENU_TRIMMED;
		    break;
		}

		newitem = makemenuitem(newmenu,
				       is_image,
				       tagList,
				       GTB(GadToolsBase));

		if (NULL == newitem)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		/*
		** Append this item to the current menu title
		*/

		if (curitem)
		{
		    curitem->NextItem = newitem;
		}
		else
		{
		    firstitem = newitem;
		    if (curmenu)
		    {
			curmenu->FirstItem = newitem;
			DEBUG_CREATEMENUSA(dprintf("CreateMenusA: set FirstItem 0x%lx\n", newitem));
		    }
		}

		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NextItem 0x%lx\n", newitem));

		item_ctr ++;
		subitem_ctr = 0;
		cursubitem = firstsubitem = NULL;

		curitem = newitem;
		break;

	    case IM_SUB:
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: IM_SUB\n"));
		is_image = TRUE;
		/* Fall through */

	    case NM_SUB:
		/*
		** There has to be an item menu structure available
		** to create a sub item, unless GTMN_FullMenu == FALSE.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_SUB\n"));
		if ( (fullmenu && (NULL == curitem)) ||
		     (prevtype == NM_TITLE) )
		{
		    err = GTMENU_INVALID;
		    goto failexit;
		}

		if (subitem_ctr >= 32)
		{
		    err = GTMENU_TRIMMED;
		    /* CreateMenusA must still return success here, only that the
		       menus will be trimmed */
		    break;
		}

		newsubitem = makemenuitem(newmenu,
					  is_image,
					  tagList,
					  GTB(GadToolsBase));

		if (NULL == newsubitem)
		{
		    err = GTMENU_NOMEM;
		    goto failexit;
		}

		if (curitem)
		{
		    if (!curitem->SubItem)
		    {
		    }
		}

		/*
		** Append this item to the current sub menu item
		*/

		if (cursubitem)
		{
		    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: CurSubItem 0x%lx NextItem 0x%lx\n", cursubitem,newsubitem));
		    cursubitem->NextItem = newsubitem;
		}
		else
		{
		    firstsubitem = newsubitem;
		    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: FirstSubItem 0x%lx\n", firstsubitem));
		    if (curitem)
		    {
			curitem->SubItem = newsubitem;

			DEBUG_CREATEMENUSA(dprintf("CreateMenusA: CurItem 0x%lx SubItem 0x%lx\n", curitem,newsubitem));

			/* Add the ">>" mark. Hmm ... maybe if would be better if this
			   was done in LayoutMenus() ??? */

			curitem->Flags &= ~COMMSEQ;
			if (curitem->Flags & ITEMTEXT)
			{
			    struct IntuiText *it = (struct IntuiText *)curitem->ItemFill;

			    if (!it->NextText)
			    {
				struct IntuiText *it2 = it + 1;

				it->NextText = it2;
				it2->IText = "»";
			    }
			}

		    }
		}

		cursubitem = newsubitem;
		subitem_ctr ++;
		break;

	    case NM_IGNORE:
		/*
		** Nothing to do in this case
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_IGNORE\n"));
		break;

	    case NM_END:
		/*
		** The end.
		*/
		DEBUG_CREATEMENUSA(dprintf("CreateMenusA: NM_END\n"));
		end = TRUE;
		break;

	} /* switch (newmenu->nm_Type) */

	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: firstmenu %p curmenu %p firstitem %p curitem %p firstsub %p cursub %p\n",
				firstmenu, curmenu, firstitem, curitem, firstsubitem, cursubitem));

	prevtype = newmenu->nm_Type;

	/*
	** Advance to the next item in the array
	*/
	newmenu = &newmenu[1];

    } /* while (FALSE == end) */

    /*
    ** Set the secondary error value if requested.
    */
    ti = FindTagItem(GTMN_SecondaryError, tagList);

    if (NULL != ti)
        ti->ti_Data = err;

    if (!firstmenu)
    {
	firstmenu = (struct Menu *)firstitem;
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: No FirstMenu..use firstitem 0x%lx\n", firstitem));
    }
    if (!firstmenu)
    {
	firstmenu = (struct Menu *)firstsubitem;
	DEBUG_CREATEMENUSA(dprintf("CreateMenusA: No FirstMenu..use firstsubitem 0x%lx\n", firstsubitem));
    }

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: return %p\n", firstmenu));

    DumpMenu(firstmenu);

    return firstmenu;


failexit:

    DEBUG_CREATEMENUSA(dprintf("CreateMenusA: failed\n"));

    /*
    ** Free all memory
    */
    FreeMenus(firstmenu);

    /*
    ** Set the secondary error value if requested.
    */
    ti = FindTagItem(GTMN_SecondaryError, tagList);

    if (NULL != ti)
        ti->ti_Data = err;

    return NULL;

    AROS_LIBFUNC_EXIT

} /* CreateMenusA */
#endif

#if 0
void	DumpMenuItem(struct MenuItem *item)
{
	if (!item) return;
	while (item)
	{
		DEBUG_DUMPMENUS(bug("DumpMenus: Item 0x%lx NextItem 0x%lx SubItem 0x%lx\n", item,item->NextItem,item->SubItem));
		if (item->Flags & ITEMTEXT)
	    	{
			struct IntuiText *MyText=(struct IntuiText*) item->ItemFill;
			DEBUG_DUMPMENUS(bug("DumpMenus: <%s>\n",MyText->IText));
		}
		else
	    	{
			DEBUG_DUMPMENUS(bug("DumpMenus: Image 0x%lx\n", item->ItemFill));
		}
		DumpMenuItem(item->SubItem);
		item = item->NextItem;
	}
}

void	DumpMenu(struct Menu *menu)
{
ULONG *p = (ULONG *)menu - 1;

	if (!menu) return;
	if (*p)
	{
		DumpMenuItem((struct MenuItem*) menu);
	}
	else
	{
		while (menu)
		{
			DEBUG_DUMPMENUS(bug("DumpMenus: Menu 0x%lx <%s> NextMenu 0x%lx FirstItem 0x%lx\n", menu, menu->MenuName, menu->NextMenu, menu->FirstItem));
			DumpMenuItem(menu->FirstItem);
			menu = menu->NextMenu;
		}
	}
}
#endif
