/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"
#include <exec/types.h>
#define DEBUG 0
#include <aros/debug.h>

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
 
  BOOL end = FALSE;
  struct Menu * firstmenu = NULL;
  struct Menu * curmenu;
  struct MenuItem * curitem;
  struct MenuItem * cursubitem;
  ULONG menu_ctr = 0;
  ULONG item_ctr = 0;
  ULONG subitem_ctr = 0;
  struct TagItem * ti;
  BOOL fullmenu = GetTagData(GTMN_FullMenu, FALSE, tagList);

#warning Variable 'fullmenu' should be used!!!

  ULONG err = 0;

  D(bug("Entering %s\n",__FUNCTION__));
 
  while (FALSE == end)
  {
    BOOL is_image = FALSE;
    
    switch (newmenu->nm_Type)
    {
      case NM_TITLE:
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
      break;

      case IM_ITEM:
        is_image = TRUE;
        /* Fall through */
      case NM_ITEM: /* also the BARLABEL */

        /*
        ** There has to be a menu structure available
        ** to create an item. 
        */
        
        if (NULL == curmenu)
        {
          err = GTMENU_INVALID;
          goto failexit;
        }

        if (item_ctr > 64)
        {
          err = GTMENU_TRIMMED;
          goto failexit;
        }
        
        curitem = makemenuitem(newmenu,
                               is_image,
                               tagList,
                               GTB(GadToolsBase));
        
        if (NULL == curitem)
        {
          err = GTMENU_NOMEM;
          goto failexit;
        }
        
        /*
        ** Append this item to the current menu title
        */
        appenditem(curmenu, curitem);
        
        item_ctr ++;
        subitem_ctr = 0;
      break;
      
      case IM_SUB:
        is_image = TRUE;
        /* Fall through */
      case NM_SUB:
        /*
        ** There has to be an item menu structure available
        ** to create a sub item. 
        */
        if (NULL ==  curitem)
        {
          err = GTMENU_INVALID;
          goto failexit;
        }

        if (subitem_ctr > 32)
        {
          err = GTMENU_TRIMMED;
          goto failexit;
        }

	if (!curitem->SubItem)
	{
	    /* First sub item */
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
	
        cursubitem = makemenuitem(newmenu,
                                  is_image,
                                  tagList,
                                  GTB(GadToolsBase));
        
        if (NULL == cursubitem)
        {
          err = GTMENU_NOMEM;
          goto failexit;
        }
        /*
        ** Append this item to the current menu item
        */
        appendsubitem(curitem, cursubitem);
        
        subitem_ctr ++;
      break;
      
      case NM_IGNORE:
        /*
        ** Nothing to do in this case
        */
      break;
      
      case NM_END:
        /*
        ** The end.
        */
        end = TRUE;
      break;
    }
  
    /*
    ** Advance to the next item in the array
    */
    newmenu = &newmenu[1];
  }

  return firstmenu;


failexit:
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
