/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GadTools menu creation functions
    Lang: English
*/

/**************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "gadtools_intern.h"

/**************************************************************************************************/

#define CHECKMARK_TEXT_SPACING 2
#define MENU_COLUMN_SPACING    8 /* when items must be arranged in 2 or more columns to fit on screen */
#define AMIGAKEY_KEY_SPACING   4 /* we guess that Intuition uses the same value */
#define TEXT_AMIGAKEY_SPACING  6

#define ITEXT_EXTRA_LEFT   2
#define ITEXT_EXTRA_RIGHT  2
#define ITEXT_EXTRA_TOP    1
#define ITEXT_EXTRA_BOTTOM 1

#define IMAGE_EXTRA_LEFT   1
#define IMAGE_EXTRA_RIGHT  1
#define IMAGE_EXTRA_TOP    1
#define IMAGE_EXTRA_BOTTOM 1

/**************************************************************************************************/

static BOOL is_menubarlabelclass_image(struct Image *im, struct GadToolsBase_intern *GadToolsBase)
{
    BOOL is_barlabel = FALSE;

    if (im)
    {
	if (im->Depth == CUSTOMIMAGEDEPTH)
	{
	    Class *cl = OCLASS(im);

	    if (cl->cl_ID)
	    {
		if (strcmp(cl->cl_ID, MENUBARLABELCLASS) == 0) is_barlabel = TRUE;
	    }
	}
    }
    
    return is_barlabel;
}

/**************************************************************************************************/

void appendmenu(struct Menu * firstmenu,
                struct Menu * lastmenu)
{
    while (NULL != firstmenu->NextMenu)
        firstmenu = firstmenu->NextMenu;

    firstmenu->NextMenu = lastmenu;
}

/**************************************************************************************************/

void appenditem(struct Menu * curmenu,
                struct MenuItem * item)
{
    struct MenuItem * mi = curmenu->FirstItem;

    if (NULL == mi)
	curmenu->FirstItem = item;
    else
    {
	while (NULL != mi->NextItem)
	    mi = mi->NextItem;

	mi->NextItem = item;
    }
}

/**************************************************************************************************/

void appendsubitem(struct MenuItem * curitem,
                   struct MenuItem * cursubitem)
{
    struct MenuItem *mi = curitem->SubItem;

    if (NULL == mi)
	curitem->SubItem = cursubitem;
    else
    {
	while (NULL != mi->NextItem) 
	    mi = mi->NextItem;

	mi->NextItem = cursubitem;
    }
}

/**************************************************************************************************/

struct Menu * makemenutitle(struct NewMenu * newmenu,
                            struct TagItem * taglist)
{
    struct Menu * menu;
    
    menu = (struct Menu *)AllocMem(sizeof(struct Menu)+sizeof(APTR), 
                                   MEMF_CLEAR);

    if (NULL != menu)
    {
	menu->Flags    	      = (newmenu->nm_Flags & NM_MENUDISABLED) ^ MENUENABLED;
	menu->MenuName 	      = newmenu->nm_Label;
	GTMENU_USERDATA(menu) = newmenu->nm_UserData;
    }

    return menu;
}

/**************************************************************************************************/

struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem 	* menuitem;
    UWORD 		allocsize;

    /* Note: 2 IntuiTexts must be alloced, because CreateMenusA can depend on it 
             if a subitem mark (">>") must be added */

    if (newmenu->nm_Label == NM_BARLABEL) is_image = TRUE;

    allocsize = sizeof(struct MenuItem) + sizeof(APTR);
    if (!is_image) allocsize += sizeof(struct IntuiText) * 2; /* for text + (commandstring OR ">>") */

    menuitem = (struct MenuItem *)AllocVec(allocsize, MEMF_CLEAR);

    if (NULL != menuitem)
    { 
	menuitem->Flags = (newmenu->nm_Flags & (NM_ITEMDISABLED | CHECKIT | MENUTOGGLE | CHECKED)) ^
    			  ITEMENABLED;

	menuitem->Flags |= HIGHCOMP;

	if (newmenu->nm_CommKey)
	{
            if (!(newmenu->nm_Flags & NM_COMMANDSTRING))
	    {
		menuitem->Flags |= COMMSEQ;
		menuitem->Command = newmenu->nm_CommKey[0];
	    }    
	}

	menuitem->MutualExclude = newmenu->nm_MutualExclude;
	GTMENUITEM_USERDATA(menuitem) = newmenu->nm_UserData;

	if (FALSE == is_image)
	{
	    /*
	    ** Text
	    */
	    struct IntuiText * it = (struct IntuiText *)(((UBYTE *)menuitem) +
      							 sizeof(struct MenuItem) +
							 sizeof(APTR));

	    menuitem->ItemFill = (APTR)it;

	    it->FrontPen = GetTagData(GTMN_FrontPen, 0, taglist);
	    it->DrawMode = JAM1;

	    it->IText    = newmenu->nm_Label;

	    menuitem->Flags |= ITEMTEXT;

	    if (newmenu->nm_CommKey && (newmenu->nm_Flags & NM_COMMANDSTRING))
	    {
        	struct IntuiText *it2 = it + 1;

		*it2 = *it;
		it2->IText = newmenu->nm_CommKey;
		it->NextText = it2;
	    }
	  
	} /* if (FALSE == is_image) */
	else
	{
	    menuitem->Flags &= ~ITEMTEXT;     

	    if (newmenu->nm_Label != NM_BARLABEL)
	    {
		/*
		** An image.
		*/
		menuitem->ItemFill = (APTR)newmenu->nm_Label;
	    }
	    else
	    {	
        	/*
		** A barlabel image.
		*/

		struct TagItem barlabel_tags[] =
		{
		    {IA_Left	, ITEXT_EXTRA_LEFT	},
		    {IA_Top	, 3			},
		    {IA_Width	, 20			},
		    {IA_Height	, 2			},
		    {TAG_DONE				}
		};

		menuitem->Flags &= ~ITEMENABLED;

		menuitem->ItemFill = NewObjectA(NULL, MENUBARLABELCLASS, barlabel_tags);

		if (!menuitem->ItemFill)
		{
		    FreeVec(menuitem);
		    menuitem = NULL;
		}
	    } 

	} /* is_image */
      
    } /* if (NULL != menuitem) */

    return menuitem;
}

/**************************************************************************************************/

static void freeitem(struct MenuItem *item, struct GadToolsBase_intern * GadToolsBase)
{
    if (!(item->Flags & ITEMTEXT))
    {
	struct Image *im = (struct Image *)item->ItemFill;

	if (is_menubarlabelclass_image(im, GadToolsBase))
	{
	    DisposeObject(im);
	}
    }

    FreeVec(item);
}

/**************************************************************************************************/

void freeitems(struct Menu * m, struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem * mi = m->FirstItem;
    
    while (mi)
    {
	struct MenuItem * _mi = mi->NextItem, *si = mi->SubItem;

	while(si)
	{
	    struct MenuItem * _si = si->NextItem;

	    freeitem(si, GadToolsBase); 
	    si = _si;
	}

	freeitem(mi, GadToolsBase);

	mi = _mi;
    }
}

/**************************************************************************************************/

static WORD MyIntuiTextLength(struct VisualInfo *vi, struct IntuiText *it, 
			      struct GadToolsBase_intern * GadToolsBase)
{
    WORD width;
        
    if (it->ITextFont)
    {
        struct IntuiText *it_next = it->NextText;

	it->NextText = NULL;
        width = IntuiTextLength(it);
	it->NextText = it_next;

    }
    else
    {
        width = TextLength(&vi->vi_screen->RastPort, it->IText, strlen(it->IText)); 
    }
    
    return width;
}

/**************************************************************************************************/

static ULONG EqualizeItems(struct MenuItem *firstitem, struct MenuItem *lastitem,
    	    	    	   struct Image *amikeyimage, struct VisualInfo *vi,
			   struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem *item;
    WORD    	    minx = 0, maxwidth = 0, maxrightstuffwidth = 0;
    
    if (!firstitem) return 0;
    
    /* Calc. the max. width of item text/image (+ checkmark image)
       Calc. the min. item->LeftEdge */

    for(item = firstitem; item && (item != lastitem); item = item->NextItem)
    {
        if (item->LeftEdge < minx) minx = item->LeftEdge;
	if (item->Width > maxwidth) maxwidth = item->Width;
    }
    
    /* Calc. the max. width of AmigaKey/CommandString/">>" */
    
    for(item = firstitem; item && (item != lastitem); item = item->NextItem)
    {
        WORD width = 0;
	
	if (item->Flags & COMMSEQ)
	{	    
	    width =  vi->vi_dri->dri_Font->tf_XSize;

	    if (item->Flags & ITEMTEXT)
	    {
        	struct TextFont  *font;
		struct IntuiText *it = (struct IntuiText *)item->ItemFill;

		if (it->ITextFont)
		{
        	    if (NULL != (font = OpenFont(it->ITextFont)))
        	    {
        	        width = font->tf_XSize;
        	        CloseFont(font);
        	    }
		}
	    }

            width += amikeyimage->Width +
		     AMIGAKEY_KEY_SPACING +
		     TEXT_AMIGAKEY_SPACING;
		     
        }
	else if (item->Flags & ITEMTEXT)
	{
	    struct IntuiText *it = (struct IntuiText *)item->ItemFill;
	    struct IntuiText *it2;
	    
	    if ((it2 = it->NextText))
	    {
	        it2->FrontPen = it->FrontPen;
		it2->BackPen  = it->BackPen;
		it2->DrawMode = it->DrawMode;
		it2->TopEdge  = it->TopEdge;
		
		/* save width also in it->LeftEdge. Will be used below to
		   calc. x position */
		   
	        width = it2->LeftEdge = MyIntuiTextLength(vi, it2, GadToolsBase);
	    }
	}

	if (width > maxrightstuffwidth) maxrightstuffwidth = width;
	
    } /* for(item = firstitem; item; item = item->NextItem) */

    /* Calc. x coordinate of command strings and ">>"'s and submenu pos. */
    
    if (maxrightstuffwidth)
    {
        maxwidth += maxrightstuffwidth + TEXT_AMIGAKEY_SPACING;
	
	for(item = firstitem; item && (item != lastitem); item = item->NextItem)
	{
	    struct MenuItem *subitem;
	    
	    if (item->Flags & ITEMTEXT)
	    {
	        struct IntuiText *it = (struct IntuiText *)item->ItemFill;
		
		if ((it = it->NextText))
		{
		    /* it->LeftEdge contains the pixel width. see above */
		    
		    it->LeftEdge = minx + maxwidth - it->LeftEdge - ITEXT_EXTRA_RIGHT;
		}
	    }
	    
	    for(subitem = item->SubItem; subitem; subitem = subitem->NextItem)
	    {
	        subitem->LeftEdge += (maxwidth - maxrightstuffwidth);
	    }
	}
	
    } /* if (maxrightstuffwidth) */
    
    /* Make all items have the same width and set also the width and the
       drawinfo of barlabel images */
    
    for(item = firstitem; item && (item != lastitem); item = item->NextItem)
    {
        item->Width = maxwidth;
	
	if(!(item->Flags & ITEMTEXT))
	{
	    struct Image *im = (struct Image *)item->ItemFill;
	    
	    if (is_menubarlabelclass_image(im, GadToolsBase))
	    {
		struct TagItem image_tags [] =
		{
		    {IA_Width	    , maxwidth - ITEXT_EXTRA_LEFT - ITEXT_EXTRA_RIGHT	},
		    {SYSIA_DrawInfo , (IPTR)vi->vi_dri					},
		    {TAG_DONE			    	    	    	    	    	}
		};

		SetAttrsA(im, image_tags);

	    }
	}
    }
    
    return (ULONG)maxwidth;
}

/**************************************************************************************************/

BOOL layoutmenuitems(struct MenuItem * firstitem,
                     struct VisualInfo * vi,
                     struct TagItem * taglist,
                     struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem 	* menuitem = firstitem;
    struct MenuItem 	* equalizeitem = firstitem;
    struct Image 	* amikeyimage, * chkimage;

    ULONG 		curX = 0;
    ULONG 		curY = 0;

    amikeyimage = (struct Image *)GetTagData(GTMN_AmigaKey, 0, taglist);
    if (!amikeyimage) amikeyimage = vi->vi_dri->dri_AmigaKey;

    chkimage = (struct Image *)GetTagData(GTMN_Checkmark, 0, taglist);
    if (!chkimage) chkimage = vi->vi_dri->dri_CheckMark;

    while (NULL != menuitem)
    {
	ULONG addwidth = 0;
	ULONG addleft = 0;

	menuitem->LeftEdge = curX;
	menuitem->TopEdge  = curY;

	/*
	** Check the flags whether there exists a shortcut or a checkmark is
	** necessary..
	*/
	if (0 != (menuitem->Flags & COMMSEQ))
	{
	    /*
	    ** An Amiga key image and a character will appear
	    */
	    struct Image * amikeyimage = (struct Image *)GetTagData(GTMN_AmigaKey, 0, taglist);

	    if (!amikeyimage) amikeyimage = vi->vi_dri->dri_AmigaKey;

	    addwidth = vi->vi_dri->dri_Font->tf_XSize;
	    if (menuitem->Flags & ITEMTEXT)
	    {
        	struct TextFont  *font;
		struct IntuiText *it = (struct IntuiText *)menuitem->ItemFill;

		if (it->ITextFont)
		{
        	    if (NULL != (font = OpenFont(it->ITextFont)))
        	    {
        	      addwidth = font->tf_XSize;
        	      CloseFont(font);
        	    }
		}
	    }

	    addwidth += amikeyimage->Width +
			AMIGAKEY_KEY_SPACING +
			TEXT_AMIGAKEY_SPACING;
		      
	} /* if (0 != (menuitem->Flags & COMMSEQ)) */

	if (0 != (menuitem->Flags & CHECKIT))
	{
	    /*
	    ** A checkmark will appear on the left.
	    */

	    addleft  += chkimage->Width + CHECKMARK_TEXT_SPACING;
	    addwidth += chkimage->Width + CHECKMARK_TEXT_SPACING;      
	}

	if (0 != (menuitem->Flags & ITEMTEXT))
	{
	    /*
	    ** Text
	    */
	    struct IntuiText * it = (struct IntuiText *)menuitem->ItemFill;
	    struct TagItem   * ti;

	    if (NULL == it)
        	return FALSE;

	    if (NULL != (ti = FindTagItem(GTMN_FrontPen, taglist)))
	    {
        	it->FrontPen = ti->ti_Data;
	    } else {
        	it->FrontPen = vi->vi_dri->dri_Pens[BARDETAILPEN];
	    }

	    it->ITextFont = (struct TextAttr *)GetTagData(GTMN_TextAttr,
                                                	  0,
                                                	  taglist);

	    it->LeftEdge = ITEXT_EXTRA_LEFT + addleft;       
	    it->TopEdge = ITEXT_EXTRA_TOP;

	    menuitem->Width = MyIntuiTextLength(vi, it, GadToolsBase) +
      			      addwidth + ITEXT_EXTRA_LEFT + ITEXT_EXTRA_RIGHT;

	   /*
	    ** Calculate the height menu item.
	    */
	    if (NULL == it->ITextFont)
	    {
        	/*
        	** No font is provided, so I will work with the screen font.
        	*/ 
        	menuitem->Height = vi->vi_dri->dri_Font->tf_YSize + ITEXT_EXTRA_TOP + ITEXT_EXTRA_BOTTOM;
	    }
	    else
	    {
        	struct TextFont * font;
		
        	if (NULL != (font = OpenFont(it->ITextFont)))
        	{
        	    menuitem->Height = font->tf_YSize + ITEXT_EXTRA_TOP + ITEXT_EXTRA_BOTTOM;
        	    CloseFont(font);
        	}
        	else
        	    return FALSE; 
	    }
	  
	} /* if (0 != (menuitem->Flags & ITEMTEXT)) */
	else
	{
	    /*
	    ** Image
	    */
	    struct Image * im = (struct Image *)menuitem->ItemFill;

	    if (NULL == im) return FALSE;

	    if (is_menubarlabelclass_image(im, GadToolsBase))
	    {

        	menuitem->Height = 7;

	    }
	    else
	    {

		menuitem->Width  = im->Width + addwidth + IMAGE_EXTRA_LEFT + IMAGE_EXTRA_RIGHT;
		menuitem->Height = im->Height + IMAGE_EXTRA_TOP + IMAGE_EXTRA_BOTTOM;
		im->LeftEdge = IMAGE_EXTRA_LEFT + addleft;
		im->TopEdge = IMAGE_EXTRA_TOP;
	    }
	  
	} /* if (0 != (menuitem->Flags & ITEMTEXT)) else ... */

        curY += menuitem->Height;

	/*
	** In case this menu becomes too high it will have more columns.
	*/

	if (curY > vi->vi_screen->Height)
	{
	    ULONG width;

	    width = EqualizeItems(equalizeitem, menuitem, amikeyimage, vi, GadToolsBase);
	    equalizeitem = menuitem;

	    curY = 0;
	    curX += width + MENU_COLUMN_SPACING;
	    menuitem->LeftEdge = curX;
	    menuitem->TopEdge  = curY;
	    
	    curY += menuitem->Height;
	}

	/*
	** Process the next menu item
	*/    
	menuitem = menuitem->NextItem;
      
    } /* while (NULL != menuitem) */

    EqualizeItems(equalizeitem, NULL, amikeyimage, vi, GadToolsBase);

    return TRUE;
}

/**************************************************************************************************/

BOOL layoutsubitems(struct MenuItem * motheritem,
                    struct VisualInfo * vi,
                    struct TagItem * taglist,
                    struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem * menuitem = motheritem->SubItem;

    layoutmenuitems(menuitem, vi, taglist, GadToolsBase);

    return TRUE;
}

/**************************************************************************************************/
