/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */

#include <string.h>

#include <exec/types.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <graphics/text.h>

        AROS_LH3(BOOL, LayoutMenusA,

/*  SYNOPSIS */
	AROS_LHA(struct Menu *, menu, A0),
	AROS_LHA(APTR, vi, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, GadToolsBase, 11, GadTools)

/*  FUNCTION

    INPUTS
	menu -     Menu to be layouted.
	vi -       Visual info to layout the menu for.
	tagList  - Additional tags.

    RESULT
	FALSE, if an error occured.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      CreateMenusA() LayoutMenuItemsA() GetVisualInfoA()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    ULONG 		curX = 0;
    ULONG 		curY = 0;
    struct VisualInfo 	* vinfo = (struct VisualInfo *)vi;

    struct TextFont 	* textfont = vinfo->vi_dri->dri_Font;

    /* Set GTMN_Menu to menu, as GTMN_Menu is really just a
       LayoutMenuItemsA() tag, and should not appear in the
       taglist passed to LayoutMenusA(). stegerg. */
    
    struct TagItem 	stdlayouttags[] =
    {
	{GTMN_Menu		, (IPTR)menu},
	{GTMN_TextAttr		, NULL	    },
	{GTMN_NewLookMenus	, TRUE	    },
	{GTMN_Checkmark		, NULL	    },
	{GTMN_AmigaKey		, NULL	    },
	{GTMN_FrontPen		, 0L	    },
	{TAG_DONE	    	    	    }
    };

    if (NULL == textfont)
        return FALSE;

    stdlayouttags[TAG_TextAttr].ti_Data     = GetTagData(GTMN_TextAttr,
                                                	 NULL, 
                                                	 tagList);

    stdlayouttags[TAG_NewLookMenus].ti_Data = GetTagData(GTMN_NewLookMenus, 
                                                	 FALSE, 
                                                	 tagList);

    stdlayouttags[TAG_CheckMark].ti_Data    = GetTagData(GTMN_Checkmark,
                                                	 NULL, 
                                                	 tagList);

    stdlayouttags[TAG_AmigaKey].ti_Data     = GetTagData(GTMN_AmigaKey,
                                                	 NULL, 
                                                	 tagList);

    /*
    ** Only if the FrontPen is provided I will make it a valid
    ** entry in the tag list.
    */ 

    if (NULL != tagList && NULL != FindTagItem(GTMN_FrontPen, tagList))
    {
	stdlayouttags[TAG_FrontPen].ti_Data = GetTagData(GTMN_FrontPen,
                                                         0, 
                                                         tagList);
    }
    else
    {
	stdlayouttags[TAG_FrontPen].ti_Tag = TAG_DONE;
    }


    while (NULL != menu)
    {
	if (NULL != menu->FirstItem)
	{
	    stdlayouttags[TAG_Menu].ti_Data = (ULONG)menu;

	    if (FALSE == LayoutMenuItemsA(menu->FirstItem,
                                	  vi,
                                	  stdlayouttags))
                return FALSE;
	}
	
	/*
	** Set the coordinates of this menu title
	** !!! This might still look ugly...
	*/
	menu->LeftEdge = curX;
	menu->TopEdge  = curY;

	menu->Width    = TextLength(&vinfo->vi_screen->RastPort,
                                    menu->MenuName, 
                                    strlen(menu->MenuName)) +
			 vinfo->vi_screen->MenuHBorder * 2;

    #if 0     
	/* stegerg: the Amiga just clips them away, and BTW:
            dri->dri_Resolution.X is not screen width!! It's
	    aspect information */                           
	if (menu->Width + curX > vinfo->vi_dri->dri_Resolution.X)
	{
    	    //#warning Proper layout of menu titles???
	    curX  = 0;
	    curY += ((textfont->tf_YSize * 5) / 4);

	    menu->LeftEdge = curX;
	    menu->TopEdge  = curY;
	}
    #endif

	menu->Height = textfont->tf_YSize;
    
	/* Proper layout??? */
	curX += menu->Width + vinfo->vi_screen->BarHBorder * 2;


	menu = menu->NextMenu;
    } /* while (NULL != menu) */

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* LayoutMenusA */
