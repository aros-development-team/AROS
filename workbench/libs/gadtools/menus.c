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

#define SDEBUG 1
#define DEBUG 1
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

BOOL is_menubarlabelclass_image(struct Image *im, struct GadToolsBase_intern *GadToolsBase)
{
    BOOL is_barlabel = FALSE;

    DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: im %p\n", im));

    if (im)
    {
	DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: depth %ld\n", im->Depth));
	if (im->Depth == CUSTOMIMAGEDEPTH)
	{
	    Class *cl;

	    DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: boopsi\n"));

	    cl = OCLASS(im);

	    DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: cl %p\n",cl));

	    if (cl)
	    {
		DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: ID %p\n",cl->cl_ID));
		if (cl->cl_ID)
		{
		    if (strcmp(cl->cl_ID, MENUBARLABELCLASS) == 0)
		    {
			DEBUG_FREEMENUS(bug("is_menubarlabelclass_image: barlabel\n"));
			is_barlabel = TRUE;
		    }
		}
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

#if NEWMENUCODE
struct Menu * makemenutitle(struct NewMenu * newmenu,
                            UBYTE		**MyMenuMemPtr,
                            struct TagItem * taglist)
{
    struct Menu	* menu = NULL;
    ULONG	* p;

    p =(ULONG*) *MyMenuMemPtr;
    memset(p,0,sizeof(ULONG)+sizeof(struct Menu)+sizeof(APTR));
    *MyMenuMemPtr += sizeof(ULONG)+sizeof(struct Menu)+sizeof(APTR);

    *p = 0;
    menu = (struct Menu *)(p + 1);
    menu->Flags    	      = (newmenu->nm_Flags & NM_MENUDISABLED) ^ MENUENABLED;
    menu->MenuName 	      = newmenu->nm_Label;
    GTMENU_USERDATA(menu) = newmenu->nm_UserData;

    return menu;
}

ULONG	getmenutitlesize(struct NewMenu * newmenu,
                         struct TagItem * taglist)
{
    return(sizeof(ULONG)+sizeof(struct Menu)+sizeof(APTR));
}

#else
struct Menu * makemenutitle(struct NewMenu * newmenu,
                            struct TagItem * taglist)
{
    struct Menu	* menu = NULL;
    ULONG	* p;

    p = (ULONG *)AllocMem(sizeof(ULONG)+sizeof(struct Menu)+sizeof(APTR), MEMF_CLEAR);

    if (NULL != p)
    {
	*p = 0;
	menu = (struct Menu *)(p + 1);
	menu->Flags    	      = (newmenu->nm_Flags & NM_MENUDISABLED) ^ MENUENABLED;
	menu->MenuName 	      = newmenu->nm_Label;
	GTMENU_USERDATA(menu) = newmenu->nm_UserData;
    }

    return menu;
}
#endif

/**************************************************************************************************/
#if NEWMENUCODE
struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               UBYTE		**MyMenuMemPtr,
                               struct Image	***MyBarTablePtr,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem	* menuitem = NULL;
    UWORD		allocsize;
    ULONG		* p;

    /* Note: 2 IntuiTexts must be alloced, because CreateMenusA can depend on it
             if a subitem mark (">>") must be added */

    if (newmenu->nm_Label == NM_BARLABEL) is_image = TRUE;

    allocsize = sizeof(ULONG) + sizeof(struct MenuItem) + sizeof(APTR);
    if (!is_image) allocsize += sizeof(struct IntuiText) * 2; /* for text + (commandstring OR ">>") */

    p =(ULONG*) *MyMenuMemPtr;
    memset(p,0,allocsize);
    *MyMenuMemPtr += allocsize;

	*p = allocsize;
	menuitem = (struct MenuItem *)(p + 1);

	DEBUG_ALLOCMENUS(bug("makemenuitem: MenuItem %p\n",menuitem));
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

	    DEBUG_ALLOCMENUS(bug("makemenuitem: Text MenuItem\n"));
	    menuitem->ItemFill = (APTR)it;

	    it->FrontPen = GetTagData(GTMN_FrontPen, 0, taglist);
	    it->DrawMode = JAM1;

	    it->IText    = newmenu->nm_Label;
	    DEBUG_ALLOCMENUS(bug("makemenuitem: FillName <%s>\n",it->IText));

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
	    DEBUG_ALLOCMENUS(bug("makemenuitem: Image MenuItem\n"));
	    menuitem->Flags &= ~ITEMTEXT;

	    if (newmenu->nm_Label != NM_BARLABEL)
	    {
		/*
		** An image.
		*/
		menuitem->ItemFill = (APTR)newmenu->nm_Label;
		DEBUG_ALLOCMENUS(bug("makemenuitem: FillImage 0x%lx\n",menuitem->ItemFill));
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

		DEBUG_ALLOCMENUS(bug("makemenuitem: BarImage 0x%lx\n",menuitem->ItemFill));

		if (!menuitem->ItemFill)
		{
		    DEBUG_ALLOCMENUS(bug("makemenuitem: barimage failed\n"));
		    menuitem = NULL;
		}
		DEBUG_ALLOCMENUS(bug("makemenuitem: BarTablePtr 0x%lx\n",*MyBarTablePtr));
		/*
		 * Here we put the new Image to the array the BarTablePtr points to
		 * and then inc the BarTablePtr to the next empty Array slot
		 */
		**MyBarTablePtr	=	menuitem->ItemFill;
		/*
		 * Attention..the () is *required*
		 */
		(*MyBarTablePtr)++;
	    }

	} /* is_image */

    return menuitem;
}

ULONG getmenuitemsize(struct NewMenu * newmenu,
                      BOOL is_image,
                      struct TagItem * taglist,
                      struct GadToolsBase_intern * GadToolsBase)
{
ULONG	allocsize;

    if (newmenu->nm_Label == NM_BARLABEL) is_image = TRUE;

    allocsize = sizeof(ULONG) + sizeof(struct MenuItem) + sizeof(APTR);
    if (!is_image) allocsize += sizeof(struct IntuiText) * 2; /* for text + (commandstring OR ">>") */

    DEBUG_ALLOCMENUS(bug("getmenuitemsize: allocsize %ld\n",allocsize));
    return(allocsize);
}

#else
struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase)
{
    struct MenuItem	* menuitem = NULL;
    ULONG		allocsize;
    ULONG		* p;

    /* Note: 2 IntuiTexts must be alloced, because CreateMenusA can depend on it
             if a subitem mark (">>") must be added */

    if (newmenu->nm_Label == NM_BARLABEL) is_image = TRUE;

    allocsize = sizeof(ULONG) + sizeof(struct MenuItem) + sizeof(APTR);
    if (!is_image) allocsize += sizeof(struct IntuiText) * 2; /* for text + (commandstring OR ">>") */

    p = (ULONG *)AllocMem(allocsize, MEMF_CLEAR);

    if (NULL != p)
    {
	*p = allocsize;
	menuitem = (struct MenuItem *)(p + 1);

	DEBUG_ALLOCMENUS(bug("makemenuitem: MenuItem %p\n",menuitem));
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

	    DEBUG_ALLOCMENUS(bug("makemenuitem: Text MenuItem\n"));
	    menuitem->ItemFill = (APTR)it;

	    it->FrontPen = GetTagData(GTMN_FrontPen, 0, taglist);
	    it->DrawMode = JAM1;

	    it->IText    = newmenu->nm_Label;
	    DEBUG_ALLOCMENUS(bug("makemenuitem: FillName <%s>\n",it->IText));

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
	    DEBUG_ALLOCMENUS(bug("makemenuitem: Image MenuItem\n"));
	    menuitem->Flags &= ~ITEMTEXT;

	    if (newmenu->nm_Label != NM_BARLABEL)
	    {
		/*
		** An image.
		*/
		menuitem->ItemFill = (APTR)newmenu->nm_Label;
		DEBUG_ALLOCMENUS(bug("makemenuitem: FillImage 0x%lx\n",menuitem->ItemFill));
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

		DEBUG_ALLOCMENUS(bug("makemenuitem: BarImage 0x%lx\n",menuitem->ItemFill));

		if (!menuitem->ItemFill)
		{
		    DEBUG_ALLOCMENUS(bug("makemenuitem: barimage failed\n"));
		    FreeVec(menuitem);
		    menuitem = NULL;
		}
	    }

	} /* is_image */

    } /* if (NULL != menuitem) */

    return menuitem;
}
#endif

/**************************************************************************************************/

#if !NEWMENUCODE
static void freeitem(struct MenuItem *item, struct GadToolsBase_intern * GadToolsBase)
{
    ULONG * p = (ULONG *)item - 1;

    DEBUG_FREEMENUS(bug("FreeItem: item %p\n", item));

    DEBUG_FREEMENUS(bug("FreeItem: NextItem %p\n", item->NextItem));
    DEBUG_FREEMENUS(bug("FreeItem: Left %ld Top %ld Width %ld Height %ld\n", item->LeftEdge,item->TopEdge,item->Width,item->Height));
    DEBUG_FREEMENUS(bug("FreeItem: Flags 0x%lx ItemFill 0x%lx SelectFill 0x%lx\n", item->Flags,item->ItemFill,item->SelectFill));
    DEBUG_FREEMENUS(bug("FreeItem: Command %lc SubItem 0x%lx NextSelect 0x%lx\n", item->Command,item->SubItem,item->NextSelect));

    if (!(item->Flags & ITEMTEXT))
    {
	struct Image *im = (struct Image *)item->ItemFill;

	DEBUG_FREEMENUS(bug("FreeItem: free image\n"));
	if (is_menubarlabelclass_image(im, GadToolsBase))
	{
	    DEBUG_FREEMENUS(bug("FreeItem: menubarimage %p\n",im));
	    DisposeObject(im);
	}
    }
    else
    {
	struct IntuiText *MyText=(struct IntuiText*) item->ItemFill;
	struct IntuiText *MyText2=(struct IntuiText*) item->SelectFill;
	if (MyText)
	{
	    DEBUG_FREEMENUS(bug("FreeItem: FillName <%s>\n",MyText->IText));
	}
	if (MyText2)
	{
	    DEBUG_FREEMENUS(bug("FreeItem: SelectName <%s>\n",MyText2->IText));
	}
    }

    DEBUG_FREEMENUS(bug("FreeItem: freeitem %p size %ld\n", p,*p));

    FreeMem(p, *p);
    DEBUG_FREEMENUS(bug("FreeItem: done\n"));
}

/**************************************************************************************************/

void freeitems(struct MenuItem * mi, struct GadToolsBase_intern * GadToolsBase)
{
    DEBUG_FREEMENUS(bug("FreeItems: MenuItem %p\n",mi));
    while (mi)
    {
	struct MenuItem * _mi = mi->NextItem, *si = mi->SubItem;

	DEBUG_FREEMENUS(bug("FreeItems: CurrentItem %p NextItem %p\n",mi,_mi));

	DEBUG_FREEMENUS(bug("FreeItems: free SubItems\n"));
	while(si)
	{
	    struct MenuItem * _si = si->NextItem;

	    DEBUG_FREEMENUS(bug("FreeItems: Current SubItem %p Next SubItem %p\n",si,_si));

	    freeitem(si, GadToolsBase);
	    si = _si;
	}

	DEBUG_FREEMENUS(bug("FreeItems: free MenuItem %p\n",mi));

	freeitem(mi, GadToolsBase);

	mi = _mi;
    }
    DEBUG_FREEMENUS(bug("FreeItems: done\n"));
}
#endif
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
    struct MenuItem 	*item;
    struct TextExtent    te;
    WORD    	    	 minx = 0, maxwidth = 0, maxrightstuffwidth = 0;

    if (!firstitem) return 0;

    /* Calc. the max. width of item text/image (+ checkmark image)
       Calc. the min. item->LeftEdge */

    for(item = firstitem; item && (item != lastitem); item = item->NextItem)
    {
        if (item->LeftEdge < minx) minx = item->LeftEdge;
	if (item->Width > maxwidth) maxwidth = item->Width;
    }

    /* Calc. the max. width of AmigaKey/CommandString/">>" */

    FontExtent(vi->vi_dri->dri_Font, &te);

    for(item = firstitem; item && (item != lastitem); item = item->NextItem)
    {
        WORD width = 0;

	if (item->Flags & COMMSEQ)
	{
	    width = te.te_Width;

	    if (item->Flags & ITEMTEXT)
	    {
        	struct TextFont  *font;
		struct IntuiText *it = (struct IntuiText *)item->ItemFill;

		if (it->ITextFont)
		{
        	    if ((font = OpenFont(it->ITextFont)))
        	    {
		    	struct TextExtent e;
			
			FontExtent(font, &e);
			
        	        width = e.te_Width;
        	        CloseFont(font);
        	    }
		}
	    }

            width += amikeyimage->Width +
		     AMIGAKEY_KEY_SPACING;

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
    struct TextExtent	te;
    
    ULONG 		curX = 0;
    ULONG 		curY = 0;

    amikeyimage = (struct Image *)GetTagData(GTMN_AmigaKey, 0, taglist);
    if (!amikeyimage) amikeyimage = vi->vi_dri->dri_AmigaKey;

    chkimage = (struct Image *)GetTagData(GTMN_Checkmark, 0, taglist);
    if (!chkimage) chkimage = vi->vi_dri->dri_CheckMark;

    FontExtent(vi->vi_dri->dri_Font, &te);
    
    while (NULL != menuitem)
    {
	ULONG addwidth = 0;
	ULONG addleft = 0;

	menuitem->LeftEdge = curX;
	menuitem->TopEdge  = curY;

    #if 0 /* stegerg: disabled, because menuitem->Width right here, is supposed to
             only contain the width of the leftside stuff, ie. excluding possible
	     commseq or commtext! */
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

	    addwidth = te.te_Width;
	    if (menuitem->Flags & ITEMTEXT)
	    {
        	struct TextFont  *font;
		struct IntuiText *it = (struct IntuiText *)menuitem->ItemFill;

		if (it->ITextFont)
		{
        	    if ((font = OpenFont(it->ITextFont)))
        	    {
		    	struct TextExtent e;
			
			FontExtent(font, &e);
			
        	      	addwidth = e.te_Width;
			
        	      	CloseFont(font);
        	    }
		}
	    }

	    addwidth += amikeyimage->Width +
			AMIGAKEY_KEY_SPACING +
			TEXT_AMIGAKEY_SPACING;

	} /* if (0 != (menuitem->Flags & COMMSEQ)) */
    #endif

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
	    }
	    else
	    {
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
    	    	menuitem->Width = 10;
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
