/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>

#include <graphics/gfxmacros.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 The following stuff is taken from AROS's intuition. Maybe if this go to
 the public somewhere we don't need this in the AROS version
**************************************************************************/

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

void GetMenuBox(struct Window *win, struct MenuItem *item,
	        WORD *xmin, WORD *ymin, WORD *xmax, WORD *ymax)
{

    WORD left, right, top, bottom;

    left  = top    =  0x7fff;
    right = bottom = -0x7fff;

    while(item != NULL)
    {
        left   = min(left,   item->LeftEdge);
	top    = min(top,    item->TopEdge);
	right  = max(right,  item->LeftEdge + item->Width  - 1);
	bottom = max(bottom, item->TopEdge  + item->Height - 1);
		
	item = item->NextItem;
    }

    if (xmin) *xmin = left   - win->WScreen->MenuHBorder;
    if (ymin) *ymin = top    - win->WScreen->MenuVBorder;
    if (xmax) *xmax = right  + win->WScreen->MenuHBorder;
    if (ymax) *ymax = bottom + win->WScreen->MenuVBorder;
    
}

void CalculateDims(struct Window *win, struct Menu *menu)
{
    struct MenuItem *item;

    while(menu != NULL)
    {
	item = menu->FirstItem;

	GetMenuBox(win, item, &menu->JazzX, &menu->JazzY, &menu->BeatX, &menu->BeatY);

	menu = menu->NextMenu;
    }
}

/* Mark items that has subitems. This is necessary for the input handler
   code. It's not possible to check item->SubItem within it as we save
   the layer coordinates there. */
void Characterize(struct Menu *menu)
{
    while(menu != NULL)
    {
	struct MenuItem *item;

	item = menu->FirstItem;

	while(item != NULL)
	{
//	    if(item->SubItem != NULL)
//		item->Flags |= HASSUBITEM;

	    item = item->NextItem;
	}

	menu = menu->NextMenu;
    }
}

#define MENUS_UNDERMOUSE 1
#define MENUS_AMIGALOOK 1

/** BEGIN AROS **/

#define ITEM_ITEM    1
#define ITEM_SUBITEM 2

#define AMIGAKEY_KEY_SPACING     4 /* GadTools assumes this, too */
#define AMIGAKEY_BORDER_SPACING  2

struct MenuHandlerData
{
    struct Window 	*win;
    struct Screen 	*scr;
    struct DrawInfo	*dri;
    struct Window	*menubarwin;
    struct Window 	*menuwin;
    struct Window	*submenuwin;
    struct Menu	*menu;
    struct Menu	*activemenu;
    struct MenuItem	*activeitem;
    struct MenuItem	*activesubitem;
    struct Rectangle	submenubox;
    struct Image	*checkmark;
    struct Image	*amigakey;
    WORD		menubarwidth;
    WORD		menubarheight;
    WORD		menubaritemwidth;
    WORD		menubaritemheight;
    WORD		nummenubaritems;
    WORD		activemenunum;
    WORD		activeitemnum;
    WORD		activesubitemnum;
    WORD		maxcommkeywidth_menu;
    WORD		maxcommkeywidth_submenu;
    WORD		scrmousex;
    WORD		scrmousey;
    UWORD		firstmenupick;
    UWORD		lastmenupick;
    BOOL 		active;
};

/* this #defines are taken from workbench/libs/gadtools/menus.c!! */

#define TEXT_AMIGAKEY_SPACING  6

#define ITEXT_EXTRA_LEFT   2
#define ITEXT_EXTRA_RIGHT  2
#define ITEXT_EXTRA_TOP    1
#define ITEXT_EXTRA_BOTTOM 1

static const char subitemindicator[] = "»";

/**************************************************************************************************/

static void HandleMouseMove(struct MenuHandlerData *mhd);
static void HandleMouseClick(struct MenuHandlerData *mhd, int menuup);
static void HandleCheckItem(struct Window *win, struct MenuItem *item, WORD itemnum,
			    struct MenuHandlerData *mhd);

static void HighlightMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd);

static struct Menu *FindMenu(WORD *var, struct MenuHandlerData *mhd);
static struct MenuItem *FindItem(WORD *var, struct MenuHandlerData *mhd);
static struct MenuItem *FindSubItem(WORD *var, struct MenuHandlerData *mhd);

static void MakeMenuBarWin(struct MenuHandlerData *mhd);
static void KillMenuBarWin(struct MenuHandlerData *mhd);
static void RenderMenuBar(struct MenuHandlerData *mhd);

static void MakeMenuWin(struct MenuHandlerData *mhd);
static void KillMenuWin(struct MenuHandlerData *mhd);
static void RenderMenu(struct MenuHandlerData *mhd);
static void RenderMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd);

static void MakeSubMenuWin(struct MenuHandlerData *mhd);
static void KillSubMenuWin(struct MenuHandlerData *mhd);
static void RenderSubMenu(struct MenuHandlerData *mhd);

static void RenderItem(struct MenuItem *item, WORD itemtype,  struct Rectangle *box,
		       struct MenuHandlerData *mhd);

static void RenderMenuBG(struct Window *win, struct MenuHandlerData *mhd);
static void RenderCheckMark(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd);
static void RenderAmigaKey(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd);
static void RenderDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
				  struct MenuHandlerData *mhd);
#if 0
static void RenderFrame(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD state,
			struct MenuHandlerData *mhd);
#endif
static void HighlightItem(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd);
static WORD CalcMaxCommKeyWidth(struct Window *win, struct MenuHandlerData *mhd);
static void AddToSelection(struct MenuHandlerData *mhd);
		       
    
/**************************************************************************************************/

static void HandleMouseMove(struct MenuHandlerData *mhd)
{
    struct Layer    *lay;
    struct Window   *win = NULL;
    struct Menu     *menu;
    struct MenuItem *item;
    
    WORD    	    new_activemenunum = mhd->activemenunum;
    WORD    	    new_activeitemnum = mhd->activeitemnum;
    WORD    	    new_activesubitemnum = mhd->activesubitemnum;

    mhd->scrmousex = mhd->scr->MouseX;
    mhd->scrmousey = mhd->scr->MouseY;

    if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey)))
    {	
        win = (struct Window *)lay->Window;

	if (win && (win == mhd->submenuwin))
	{
	    /* Mouse over submenu box */
	    item = FindSubItem(&new_activesubitemnum, mhd);
	    
	    if (new_activesubitemnum != mhd->activesubitemnum)
	    {
	        if (mhd->activesubitemnum != -1)
		{
		    HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd);
		}
		
		mhd->activesubitemnum = new_activesubitemnum;
		mhd->activesubitem = item;
		
		if (item)
		{
		    HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd);
		}
	    }
	    
	}
	else if (win && (win == mhd->menuwin))
	{
	    item = FindItem(&new_activeitemnum, mhd);
	    
	    if (new_activeitemnum != mhd->activeitemnum)
	    {
	        if (mhd->activeitemnum != -1)
		{
		    HighlightItem(mhd->activeitem, ITEM_ITEM, mhd);
		    KillSubMenuWin(mhd);
		}
		
		mhd->activeitemnum = new_activeitemnum;
		mhd->activeitem = item;
		
		if (item)
		{
		    HighlightItem(mhd->activeitem, ITEM_ITEM, mhd);
		    
		    if (item->SubItem)
		    {
		        MakeSubMenuWin(mhd);
		    }		
		}
	    }
	} /* if (win && (win == mhd->menuwin)) */
	else if (win && (win == mhd->menubarwin))
	{
	    /* Mouse over menu box */
	    
	    menu = FindMenu(&new_activemenunum, mhd);
	    
	    if (new_activemenunum != mhd->activemenunum)
	    {
		if (mhd->activemenunum != -1)
		{
		    HighlightMenuTitle(mhd->activemenu, mhd);
		    KillMenuWin(mhd);
		    KillSubMenuWin(mhd);
		}

		mhd->activemenunum = new_activemenunum;
		mhd->activemenu = menu;
		
		if (menu)
		{
		    HighlightMenuTitle(mhd->activemenu, mhd);
		    MakeMenuWin(mhd);
		}
	    }

	    if ((mhd->activeitemnum != -1) && (!mhd->submenuwin))
	    {
	        HighlightItem(mhd->activeitem, ITEM_ITEM, mhd);
		mhd->activeitemnum = -1;
		mhd->activeitem = NULL;
	    }
	    
	} /* if (win && (win == mhd->menubarwin)) */
	else
	{	
	    win = NULL;
	}
    } /* if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey))) */
    
    if (!win)
    {
	/* mouse outside any menu window */

	if ((mhd->activeitemnum != -1) && (!mhd->submenuwin))
	{
	    HighlightItem(mhd->activeitem, ITEM_ITEM, mhd);
	    mhd->activeitemnum = -1;
	    mhd->activeitem = NULL;
	}
	else if (mhd->activesubitemnum != -1)
	{
	    HighlightItem(mhd->activesubitem, ITEM_SUBITEM, mhd);
	    mhd->activesubitemnum = -1;
	    mhd->activesubitem = NULL;
	}
    }
 
}

/**************************************************************************************************/

static void HandleMouseClick(struct MenuHandlerData *mhd, int menuup)
{
    struct Layer *lay;

    if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey)))
    {	
	struct Window *win = (struct Window *)lay->Window;
	struct MenuItem *item = NULL;
	WORD itemnum;

     	win = (struct Window *)lay->Window;

	if (win && (win == mhd->submenuwin) && (mhd->activesubitemnum != -1))
	{
	    item = mhd->activesubitem;
	}
	else if (win && (win == mhd->menuwin) && (mhd->activeitemnum != -1))
	{
	    item = mhd->activeitem;
	}
		
	if (item) if (item->Flags & CHECKIT)
	{
	    HandleCheckItem(win, item, itemnum, mhd);
	}
		
	AddToSelection(mhd);
    } /* if ((lay = WhichLayer(&mhd->scr->LayerInfo, mhd->scrmousex, mhd->scrmousey))) */
}

/**************************************************************************************************/

static void HandleCheckItem(struct Window *win, struct MenuItem *item, WORD itemnum,
			    struct MenuHandlerData *mhd)
{
    /* Note: If you change something here, you probably must also change
             menus.c/CheckMenuItemWasClicked() which is used when the
	     user uses the menu key shortcuts! */
	     
    WORD itemtype = ((win == mhd->menuwin) ? ITEM_ITEM : ITEM_SUBITEM);
    		    
    BOOL re_render = FALSE;

    if (item->Flags & MENUTOGGLE)
    {
	item->Flags ^= CHECKED;
	re_render = TRUE;
    }
    else
    {
	if (!(item->Flags & CHECKED))
	{
	    item->Flags |= CHECKED;
	    re_render = TRUE;
	}
    }

    if (re_render)
    {
	BOOL toggle_hi = FALSE;

	if ((item->Flags & HIGHITEM) &&
	    ((item->Flags & HIGHFLAGS) == HIGHCOMP)) toggle_hi = TRUE;

	if (toggle_hi) HighlightItem(item, itemtype, mhd);				
	RenderCheckMark(item, itemtype, mhd);
	if (toggle_hi) HighlightItem(item, itemtype, mhd);

    }

    if (item->MutualExclude)
    {
	struct MenuItem *checkitem = (itemtype == ITEM_ITEM) ? mhd->activemenu->FirstItem :
							       mhd->activeitem->SubItem;
	BOOL 	    	toggle_hi = FALSE;
	WORD 	    	i;

	if ((item->Flags & HIGHITEM) &&
	    ((item->Flags & HIGHFLAGS) == HIGHBOX)) toggle_hi = TRUE;

	if (toggle_hi) HighlightItem(item, itemtype, mhd);				

	for(i = 0; (i < 32) && checkitem; i++, checkitem = checkitem->NextItem)
	{
	    if ((i != itemnum) && (item->MutualExclude & (1L << i)) &&
		((checkitem->Flags & (CHECKED | CHECKIT)) == (CHECKIT | CHECKED)))
	    {
		checkitem->Flags &= ~CHECKED;
		RenderCheckMark(checkitem, itemtype, mhd);
	    }
	}

	if (toggle_hi) HighlightItem(item, itemtype, mhd);
	
    } /* if (item->MutualExclude) */
}

/**************************************************************************************************/

static void HighlightMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd)
{
    if (menu->Flags & MENUENABLED)
    {
        struct RastPort *rp = mhd->menubarwin->RPort;

#if MENUS_UNDERMOUSE
	struct Menu 	*m = mhd->menu;
	WORD 	    	x1 = mhd->scr->MenuHBorder;
	WORD	    	x2 = x1 + mhd->menubaritemwidth - 1;
	WORD 	    	y1, y2, i;
	
	for(i = 0; m != menu; m = m->NextMenu) i++;
	
	y1 = mhd->scr->MenuVBorder + i * mhd->menubaritemheight;
	y2 = y1 + mhd->menubaritemheight - 1;
	
#else
        WORD x1 = menu->LeftEdge + mhd->scr->BarHBorder - mhd->scr->MenuHBorder;
	WORD y1 = 0;
	WORD x2 = x1 + menu->Width - 1;
	WORD y2 = mhd->scr->BarHeight - 1;
#endif
	
#if MENUS_AMIGALOOK	
	SetDrMd(rp, COMPLEMENT);
	RectFill(rp, x1, y1, x2, y2);
#else
	menu->Flags ^= HIGHITEM;

#if !MENUS_UNDERMOUSE
	y1++;
#endif
	
	SetDrMd(rp, JAM1);
	SetAPen(rp, mhd->dri->dri_Pens[(menu->Flags & HIGHITEM) ? FILLPEN : BACKGROUNDPEN]);
	RectFill(rp, x1, y1, x2, y2);

	RenderMenuTitle(menu, mhd);
	
	if (menu->Flags & HIGHITEM)
	{
#if MENUS_UNDERMOUSE
	    RenderFrame(rp, x1, y1, x2, y2, IDS_SELECTED, mhd);
#else
	    SetAPen(rp, mhd->dri->dri_Pens[SHINEPEN]);
	    RectFill(rp, x1, y1, x1, y2);
	    SetAPen(rp, mhd->dri->dri_Pens[SHADOWPEN]);
	    RectFill(rp, x2, y1, x2, y2);
#endif
	}
	
	
#endif/* MENUS_AMIGALOOK */
    }
}

/**************************************************************************************************/

static struct Menu *FindMenu(WORD *var, struct MenuHandlerData *mhd)
{
    struct Menu *menu;
    WORD    	mouse_x, mouse_y, i;
    
    mouse_x = mhd->scrmousex - mhd->menubarwin->LeftEdge;
    mouse_y = mhd->scrmousey - mhd->menubarwin->TopEdge;

#if MENUS_UNDERMOUSE
    menu = NULL;
    
    mouse_x -= mhd->scr->MenuHBorder;
    mouse_y -= mhd->scr->MenuVBorder;
    
    if ((mouse_x >= 0) && (mouse_x < mhd->menubaritemwidth) && (mouse_y >= 0))
    {
        i = mouse_y / mhd->menubaritemheight;

	if ((i >= 0) && (i < mhd->nummenubaritems))
	{
            WORD i2 = i;
	
	    menu = mhd->menu;
	    while(i && menu)
	    {
		i--; menu = menu->NextMenu;
	    }

	    if (menu && (i == 0))
	    {
		*var = i2;
	    }
	}
    }
#else
    for(menu = mhd->menu, i = 0; menu; menu = menu->NextMenu, i++)
    {
	if ((mouse_x >= menu->LeftEdge) &&
	    (mouse_x < menu->LeftEdge + menu->Width) &&
	    (mouse_y >= 0) &&
	    (mouse_y <= mhd->scr->BarHeight))
	{
	    *var = i;
	    break;
	}		
    }
#endif

    return menu;
}

/**************************************************************************************************/

static struct MenuItem *FindItem(WORD *var, struct MenuHandlerData *mhd)
{
    struct MenuItem *item = NULL;
    WORD    	    mouse_x, mouse_y, i;
    
    if (mhd->menuwin)
    {
	mouse_x = mhd->scrmousex - mhd->menuwin->LeftEdge + mhd->activemenu->JazzX;
	mouse_y = mhd->scrmousey - mhd->menuwin->TopEdge  + mhd->activemenu->JazzY;

	for(item = mhd->activemenu->FirstItem, i = 0; item; item = item->NextItem, i++)
	{
	    if ((mouse_x >= item->LeftEdge) &&
		(mouse_x < item->LeftEdge + item->Width) &&
		(mouse_y >= item->TopEdge) &&
		(mouse_y < item->TopEdge + item->Height))
	    {
		*var = i;
		break;
	    }		
	}
    } /* if (mhd->menuwin) */
    
    if ((item == NULL) && !mhd->submenuwin) *var = -1;
    
    return item;
}

/**************************************************************************************************/

static struct MenuItem *FindSubItem(WORD *var, struct MenuHandlerData *mhd)
{
    struct MenuItem *item = NULL;
    WORD    	    mouse_x, mouse_y, i;
    
    if (mhd->submenuwin)
    {
	mouse_x = mhd->scrmousex - mhd->submenuwin->LeftEdge + mhd->submenubox.MinX;
	mouse_y = mhd->scrmousey - mhd->submenuwin->TopEdge  + mhd->submenubox.MinY;

	*var = -1;
	
	for(item = mhd->activeitem->SubItem, i = 0; item; item = item->NextItem, i++)
	{
	    if ((mouse_x >= item->LeftEdge) &&
		(mouse_x < item->LeftEdge + item->Width) &&
		(mouse_y >= item->TopEdge) &&
		(mouse_y < item->TopEdge + item->Height))
	    {
		*var = i;
		break;
	    }		
	}
	
    } /* if (mhd->menuwin) */
       
    return item;
}

/**************************************************************************************************/

static void MakeMenuBarWin(struct MenuHandlerData *mhd)
{
    struct TagItem win_tags[9];
    struct Menu     *menu;

#if MENUS_UNDERMOUSE
    struct RastPort *temprp;
    WORD    	    w, maxw = 0;
#endif

    win_tags[0].ti_Tag = WA_Left;
    win_tags[0].ti_Data = 0;
    win_tags[1].ti_Tag = WA_Top;
    win_tags[1].ti_Data = 0;
    win_tags[2].ti_Tag = WA_Width;
    win_tags[2].ti_Data = mhd->scr->Width;
    win_tags[3].ti_Tag = WA_Height;
    win_tags[3].ti_Data = mhd->scr->BarHeight + 1;
    win_tags[4].ti_Tag = WA_AutoAdjust;
    win_tags[4].ti_Data = TRUE;
    win_tags[5].ti_Tag = WA_Borderless;
    win_tags[5].ti_Data = TRUE;
    win_tags[6].ti_Tag = WA_CustomScreen;
    win_tags[6].ti_Data = (ULONG)mhd->scr;
    win_tags[7].ti_Tag = WA_BackFill;
    win_tags[7].ti_Tag = (IPTR)LAYERS_NOBACKFILL;
    win_tags[8].ti_Tag = TAG_DONE;

#if MENUS_UNDERMOUSE

    if (!(temprp = CloneRastPort(&mhd->scr->RastPort))) return;
    
    mhd->nummenubaritems = 0;
    for(menu = mhd->menu; menu; menu = menu->NextMenu)
    {
        w = TextLength(temprp, menu->MenuName, strlen(menu->MenuName));
	if (w > maxw) maxw = w;
        mhd->nummenubaritems++;
    }
    
    mhd->menubaritemwidth  = maxw + TextLength(temprp, subitemindicator, 1) + 
    			     TEXT_AMIGAKEY_SPACING +
    			     ITEXT_EXTRA_LEFT +
			     ITEXT_EXTRA_RIGHT;
			     
    mhd->menubaritemheight = temprp->TxHeight + ITEXT_EXTRA_TOP + ITEXT_EXTRA_BOTTOM;
        
    win_tags[2].ti_Data = mhd->menubaritemwidth + mhd->scr->MenuHBorder * 2;
    win_tags[3].ti_Data = mhd->menubaritemheight * mhd->nummenubaritems + mhd->scr->MenuVBorder * 2;
    win_tags[0].ti_Data = mhd->scr->MouseX - win_tags[2].ti_Data / 2;
    win_tags[1].ti_Data = mhd->scr->MouseY;

    FreeRastPort(temprp);

#endif   
 
    mhd->menubarwin = OpenWindowTagList(0, win_tags);

    for(menu = mhd->menu; menu; menu = menu->NextMenu)
    {
        menu->Flags &= ~HIGHITEM;
    }
    
    RenderMenuBar(mhd);
}

/**************************************************************************************************/

static void KillMenuBarWin(struct MenuHandlerData *mhd)
{
    if (mhd->menubarwin)
    {
        CloseWindow(mhd->menubarwin);
	mhd->menubarwin = NULL;
    }
}

/**************************************************************************************************/

static void RenderMenuBar(struct MenuHandlerData *mhd)
{
    if (mhd->menubarwin)
    {
        struct Menu 	*menu = mhd->menu;
	struct RastPort *rp = mhd->menubarwin->RPort;
	
	SetFont(rp, mhd->dri->dri_Font);

#if MENUS_UNDERMOUSE

	RenderMenuBG(mhd->menubarwin, mhd);
	
#else

    #if MENUS_AMIGALOOK	
        SetABPenDrMd(rp, mhd->dri->dri_Pens[BARBLOCKPEN], 0, JAM1);
    #else
        SetABPenDrMd(rp, mhd->dri->dri_Pens[BACKGROUNDPEN], 0, JAM1);
    #endif
	RectFill(rp, 0, 0, mhd->menubarwin->Width - 1, mhd->menubarwin->Height - 2);
	SetAPen(rp, mhd->dri->dri_Pens[BARTRIMPEN]);
	RectFill(rp, 0, mhd->menubarwin->Height - 1, mhd->menubarwin->Width - 1, mhd->menubarwin->Height - 1);	

    #if !MENUS_AMIGALOOK
	SetAPen(rp, mhd->dri->dri_Pens[SHINEPEN]);
	RectFill(rp, 0, 0, 0, mhd->menubarwin->Height - 2);
	RectFill(rp, 1, 0, mhd->menubarwin->Width - 1, 0);
	SetAPen(rp, mhd->dri->dri_Pens[SHADOWPEN]);
	RectFill(rp, mhd->menubarwin->Width - 1, 1, mhd->menubarwin->Width - 1, mhd->menubarwin->Height - 2);
	
    #endif

#endif

	for(; menu; menu = menu->NextMenu)
	{
	    RenderMenuTitle(menu, mhd);
	}
    }
}

/**************************************************************************************************/

static void RenderMenuTitle(struct Menu *menu, struct MenuHandlerData *mhd)
{
    struct RastPort *rp = mhd->menubarwin->RPort;
    WORD    	    len = strlen(menu->MenuName);

#if MENUS_UNDERMOUSE
    struct Menu     *m;
    WORD    	    x, y, yoff;
    
    yoff = 0;
    for(m = mhd->menu; m && (m != menu);m = m ->NextMenu)
    {
        yoff++;
    }
    
    x = mhd->scr->MenuHBorder + ITEXT_EXTRA_LEFT;
    y = mhd->scr->MenuVBorder + ITEXT_EXTRA_TOP + yoff * mhd->menubaritemheight;
#else
    WORD x   = mhd->scr->BarHBorder + menu->LeftEdge;
    WORD y   = mhd->scr->BarVBorder;
#endif

#if MENUS_AMIGALOOK
    SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
#else
    SetAPen(rp, mhd->dri->dri_Pens[(menu->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN]);
#endif

    Move(rp, x, y + rp->TxBaseline);
    Text(rp, menu->MenuName, len);

#if MENUS_UNDERMOUSE
    if (menu->FirstItem)
    {
        WORD silen = TextLength(rp, subitemindicator, 1);
	WORD x2 = mhd->scr->MenuHBorder + mhd->menubaritemwidth - ITEXT_EXTRA_RIGHT - silen;
	
	Move(rp, x2, y + rp->TxBaseline);
	Text(rp, subitemindicator, 1);
    }
#endif

    if (!(menu->Flags & MENUENABLED))
    {
#if MENUS_UNDERMOUSE
	WORD x2 = mhd->scr->MenuHBorder + mhd->menubaritemwidth - 1;
#else
        WORD x2 = x + TextLength(rp, menu->MenuName, len) - 1;
#endif
	WORD y2 = y + rp->TxHeight - 1;
	
	RenderDisabledPattern(rp, x, y, x2, y2, mhd);
    }
}

/**************************************************************************************************/

static void MakeMenuWin(struct MenuHandlerData *mhd)
{
    struct MenuItem *item;
    
    WORD width  = mhd->activemenu->BeatX - mhd->activemenu->JazzX + 1;
    WORD height = mhd->activemenu->BeatY - mhd->activemenu->JazzY + 1;
#if MENUS_UNDERMOUSE
    WORD xpos   = mhd->menubarwin->LeftEdge + mhd->menubarwin->Width - 16;
    WORD ypos   = mhd->menubarwin->TopEdge;
#else
    WORD xpos   = mhd->activemenu->LeftEdge + mhd->scr->BarHBorder + mhd->activemenu->JazzX;

  #if MENUS_AMIGALOOK
    WORD ypos   = mhd->scr->BarHeight + 1 + mhd->activemenu->JazzY;
  #else
    WORD ypos   = mhd->scr->BarHeight + 1;
  #endif
  
#endif
      
    struct TagItem win_tags[9];

    win_tags[0].ti_Tag = WA_Left;
    win_tags[0].ti_Data = xpos;
    win_tags[1].ti_Tag = WA_Top;
    win_tags[1].ti_Data = ypos;
    win_tags[2].ti_Tag = WA_Width;
    win_tags[2].ti_Data = width;
    win_tags[3].ti_Tag = WA_Height;
    win_tags[3].ti_Data = height;
    win_tags[4].ti_Tag = WA_AutoAdjust;
    win_tags[4].ti_Data = TRUE;
    win_tags[5].ti_Tag = WA_Borderless;
    win_tags[5].ti_Data = TRUE;
    win_tags[6].ti_Tag = WA_CustomScreen;
    win_tags[6].ti_Data = (ULONG)mhd->scr;
    win_tags[7].ti_Tag = WA_BackFill;
    win_tags[7].ti_Tag = (IPTR)LAYERS_NOBACKFILL;
    win_tags[8].ti_Tag = TAG_DONE;

    
#if MENUS_UNDERMOUSE    
    win_tags[1].ti_Data += (mhd->menubaritemheight * mhd->activemenunum + mhd->scr->MenuVBorder) -
    			   height / 2;
    if (xpos + width > mhd->scr->Width)
    {
        win_tags[0].ti_Data = mhd->menubarwin->LeftEdge - width + 16;
    }
#endif

    if ((item = mhd->activemenu->FirstItem))
    {
        while(item)
	{
	    item->Flags &= ~HIGHITEM;
	    item = item->NextItem;
	}
        mhd->menuwin = OpenWindowTagList(0, win_tags);
	
	mhd->maxcommkeywidth_menu = CalcMaxCommKeyWidth(mhd->menuwin, mhd);
	
	RenderMenu(mhd);
	
	mhd->activemenu->Flags |= MIDRAWN;
    }
}

/**************************************************************************************************/

static void KillMenuWin(struct MenuHandlerData *mhd)
{
    if (mhd->menuwin)
    {
        struct MenuItem *item;
	
        CloseWindow(mhd->menuwin);
	mhd->menuwin = NULL;
		
	for(item = mhd->activemenu->FirstItem; item; item = item->NextItem)
	{
	    item->Flags &= ~ISDRAWN;
	}

	mhd->activemenu->Flags &= ~MIDRAWN;

	mhd->activeitemnum = -1;
	mhd->activeitem = NULL;
    }
}

/**************************************************************************************************/

static void RenderMenu(struct MenuHandlerData *mhd)
{

    if (mhd->menuwin)
    {
	struct MenuItem *item;

	RenderMenuBG(mhd->menuwin, mhd);
	
	SetFont(mhd->menuwin->RPort, mhd->dri->dri_Font);

	for(item = mhd->activemenu->FirstItem; item; item = item->NextItem)
	{
	    RenderItem(item, ITEM_ITEM, (struct Rectangle *)(&mhd->activemenu->JazzX), mhd);
	}
	
    } /* if (mhd->menuwin) */
}

/**************************************************************************************************/

static void MakeSubMenuWin(struct MenuHandlerData *mhd)
{
    struct MenuItem *item = mhd->activeitem->SubItem;
    struct TagItem  win_tags[9];
        
    win_tags[0].ti_Tag = WA_Left;
    win_tags[0].ti_Data = 0;
    win_tags[1].ti_Tag = WA_Top;
    win_tags[1].ti_Data = 0;
    win_tags[2].ti_Tag = WA_Width;
    win_tags[2].ti_Data = 0;
    win_tags[3].ti_Tag = WA_Height;
    win_tags[3].ti_Data = 0;
    win_tags[4].ti_Tag = WA_AutoAdjust;
    win_tags[4].ti_Data = TRUE;
    win_tags[5].ti_Tag = WA_Borderless;
    win_tags[5].ti_Data = TRUE;
    win_tags[6].ti_Tag = WA_CustomScreen;
    win_tags[6].ti_Data = (ULONG)mhd->scr;
    win_tags[7].ti_Tag = WA_BackFill;
    win_tags[7].ti_Tag = (IPTR)LAYERS_NOBACKFILL;
    win_tags[8].ti_Tag = TAG_DONE;
    
    GetMenuBox(mhd->menubarwin, item, &mhd->submenubox.MinX, 
    				      &mhd->submenubox.MinY,
				      &mhd->submenubox.MaxX,
				      &mhd->submenubox.MaxY);

    win_tags[0].ti_Data = mhd->menuwin->LeftEdge +
    			  mhd->activeitem->LeftEdge - mhd->activemenu->JazzX +
			  mhd->submenubox.MinX;

    win_tags[1].ti_Data = mhd->menuwin->TopEdge +
    			  mhd->activeitem->TopEdge - mhd->activemenu->JazzY +
			  mhd->submenubox.MinY;
			  
    win_tags[2].ti_Data = mhd->submenubox.MaxX - mhd->submenubox.MinX + 1;
    win_tags[3].ti_Data = mhd->submenubox.MaxY - mhd->submenubox.MinY + 1;
    
    while(item)
    {
	item->Flags &= ~HIGHITEM;
	item = item->NextItem;
    }

    mhd->submenuwin = OpenWindowTagList(0, win_tags);

    mhd->maxcommkeywidth_submenu = CalcMaxCommKeyWidth(mhd->submenuwin, mhd);
 
    RenderSubMenu(mhd);
}

/**************************************************************************************************/

static void KillSubMenuWin(struct MenuHandlerData *mhd)
{
    if (mhd->submenuwin)
    {	
        CloseWindow(mhd->submenuwin);
	mhd->submenuwin = NULL;
		
	mhd->activesubitemnum = -1;
	mhd->activesubitem = NULL;
    }
}

/**************************************************************************************************/

static void RenderSubMenu(struct MenuHandlerData *mhd)
{

    if (mhd->submenuwin)
    {
	struct MenuItem *item;

	RenderMenuBG(mhd->submenuwin, mhd);
	
	SetFont(mhd->submenuwin->RPort, mhd->dri->dri_Font);
	
	for(item = mhd->activeitem->SubItem; item; item = item->NextItem)
	{
	    RenderItem(item, ITEM_SUBITEM, (struct Rectangle *)(&mhd->submenubox), mhd);
	}
	
    } /* if (mhd->submenuwin) */
}

/**************************************************************************************************/

static void RenderItem(struct MenuItem *item, WORD itemtype,  struct Rectangle *box,
		       struct MenuHandlerData *mhd)
{
    struct Window   *win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort *rp = win->RPort;
    WORD    	    offx = -box->MinX;
    WORD    	    offy = -box->MinY;
    BOOL    	    enabled = ((item->Flags & ITEMENABLED) &&
    		    	      (mhd->activemenu->Flags & MENUENABLED) &&
    		    	      ((itemtype == ITEM_ITEM) || (mhd->activeitem->Flags & ITEMENABLED))); 
    BOOL    	    item_supports_disable = FALSE;

    SetDrMd(rp, JAM1);

    if (item->ItemFill)
    {
	if (item->Flags & ITEMTEXT)
	{
#if MENUS_AMIGALOOK
            struct IntuiText *it = (struct IntuiText *)item->ItemFill;

	    PrintIText(rp, it, offx + item->LeftEdge, offy + item->TopEdge);
#else
	    struct IntuiText *it = (struct IntuiText *)item->ItemFill;

	    it->FrontPen = mhd->dri->dri_Pens[(item->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN];
	    it->DrawMode = JAM1;
	    
	    PrintIText(rp, it, offx + item->LeftEdge, offy + item->TopEdge);
#endif
	}
	else
	{
    	    struct Image *im = (struct Image *)item->ItemFill;
	    LONG    	 state = IDS_NORMAL;

	    if (!enabled && (im->Depth == CUSTOMIMAGEDEPTH))
	    {
		IPTR val = 0;

		GetAttr(IA_SupportsDisable, (Object *)im, &val);
		if (val)
		{
	            item_supports_disable = TRUE;
		    state = IDS_DISABLED;
		}
	    }

	    DrawImageState(rp, im, offx + item->LeftEdge, offy + item->TopEdge, state, mhd->dri);
	}
	
    } /* if (item->ItemFill) */
        
    RenderCheckMark(item, itemtype, mhd);
    RenderAmigaKey(item, itemtype, mhd);
    
    if (!enabled && !item_supports_disable)
    {
        RenderDisabledPattern(rp, offx + item->LeftEdge,
				  offy + item->TopEdge,
				  offx + item->LeftEdge + item->Width - 1,
				  offy + item->TopEdge + item->Height - 1,
			      mhd);
    }

}		      

/**************************************************************************************************/

static void RenderMenuBG(struct Window *win, struct MenuHandlerData *mhd)
{
    struct RastPort *rp = win->RPort;
    
#if MENUS_AMIGALOOK
    WORD    	    borderx = mhd->scr->MenuHBorder / 2;
    WORD    	    bordery = mhd->scr->MenuVBorder / 2;
#else
    WORD    	    borderx = 1;
    WORD    	    bordery = 1;
#endif

    /* White background */

#if MENUS_AMIGALOOK
    SetABPenDrMd(rp, mhd->dri->dri_Pens[BARBLOCKPEN], 0, JAM1);
#else
    SetABPenDrMd(rp, mhd->dri->dri_Pens[BACKGROUNDPEN], 0, JAM1);
#endif
    RectFill(rp, borderx,
		 bordery,
		 win->Width - 1 - borderx,
		 win->Height - 1 - bordery);

    /* Black border frame */
    
#if MENUS_AMIGALOOK
    SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
    RectFill(rp, 0, 0, win->Width - 1, bordery - 1);
    RectFill(rp, 0, bordery, borderx - 1, win->Height - 1 - bordery);
    RectFill(rp, win->Width - borderx, bordery, win->Width - 1, win->Height - 1);
    RectFill(rp, 0, win->Height - bordery, win->Width - 1 - borderx, win->Height - 1);
#else
    RenderFrame(rp, 0, 0, win->Width - 1, win->Height - 1, IDS_NORMAL, mhd);
#endif
}

/**************************************************************************************************/

static void RenderCheckMark(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd)
{
    struct Window   	*win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort 	*rp = win->RPort;
    struct Rectangle 	*box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    WORD    	    	offx = -box->MinX;
    WORD    	    	offy = -box->MinY;
    WORD    	    	state = ((item->Flags & HIGHITEM) &&
    		    	    	((item->Flags & HIGHFLAGS) == HIGHCOMP)) ? IDS_SELECTED : IDS_NORMAL;
    
    if (item->Flags & CHECKIT)
    {
	WORD x1, y1, x2, y2;

	x1 = item->LeftEdge + offx;
	y1 = item->TopEdge  + offy + (item->Height - mhd->checkmark->Height) / 2;
	x2 = x1 + mhd->checkmark->Width  - 1;
	y2 = y1 + mhd->checkmark->Height - 1;
    
        SetDrMd(rp, JAM1);
	
        if (item->Flags & CHECKED)
	{
	    DrawImageState(rp, mhd->checkmark, x1, y1, state, mhd->dri);
	} else {
#if MENUS_AMIGALOOK
	    SetAPen(rp, mhd->dri->dri_Pens[BARBLOCKPEN]);
#else
	    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
#endif
	    RectFill(rp, x1, y1, x2, y2);
	}
    }
    
}

/**************************************************************************************************/

static void RenderAmigaKey(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd)
{
    struct Window   	*win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort 	*rp = win->RPort;
    struct Rectangle 	*box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    WORD    	    	commkeywidth = ((itemtype == ITEM_ITEM) ? mhd->maxcommkeywidth_menu : mhd->maxcommkeywidth_submenu);    
    WORD    	    	offx = -box->MinX;
    WORD    	    	offy = -box->MinY;
    WORD    	    	state = ((item->Flags & HIGHITEM) &&
    		    	    	((item->Flags & HIGHFLAGS) == HIGHCOMP)) ? IDS_SELECTED : IDS_NORMAL;

    if (item->Flags & COMMSEQ)
    {
        struct TextFont *oldfont = rp->Font;
	struct TextFont *newfont = NULL;
	
	WORD x1, y1, x2, y2;

	if (item->Flags & ITEMTEXT)
	{
	    struct IntuiText *it = (struct IntuiText *)item->ItemFill;
	
	    if (it->ITextFont)
	    {
	        if ((newfont = OpenFont(it->ITextFont)))
		{
		    SetFont(rp, newfont);
		}
	    }
	}
	
	x1 = item->LeftEdge + offx + item->Width - AMIGAKEY_BORDER_SPACING -
	     mhd->amigakey->Width - AMIGAKEY_KEY_SPACING - commkeywidth;
	y1 = item->TopEdge  + offy + (item->Height - mhd->amigakey->Height + 1) / 2;
	x2 = x1 + mhd->amigakey->Width  - 1;
	y2 = y1 + mhd->amigakey->Height - 1;
    
        SetDrMd(rp, JAM1);
	
	DrawImageState(rp, mhd->amigakey, x1, y1, state, mhd->dri);
	
	x1 += mhd->amigakey->Width + AMIGAKEY_KEY_SPACING;
	
#if MENUS_AMIGALOOK
	SetAPen(rp, mhd->dri->dri_Pens[BARDETAILPEN]);
#else
	SetAPen(rp, mhd->dri->dri_Pens[(item->Flags & HIGHITEM) ? FILLTEXTPEN : TEXTPEN]);
#endif
	Move(rp, x1, item->TopEdge + offy + (item->Height - rp->TxHeight) / 2 +
		     rp->TxBaseline);
	Text(rp, &item->Command, 1);
	
	if (newfont)
	{
	    CloseFont(newfont);
	    SetFont(rp, oldfont); 
	}
	
    } /* if (item->Flags & COMMSEQ) */
}

/**************************************************************************************************/

static void RenderDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
				  struct MenuHandlerData *mhd)
{
    static UWORD pattern [] = {0x8888, 0x2222};
    
    SetDrMd(rp, JAM1);
#if MENUS_AMIGALOOK
    SetAPen(rp, mhd->dri->dri_Pens[BARBLOCKPEN]);
#else
    SetAPen(rp, mhd->dri->dri_Pens[BACKGROUNDPEN]);
#endif

    SetAfPt(rp, pattern, 1);
    
    RectFill(rp, x1, y1, x2, y2);
    
    SetAfPt(rp, NULL, 0);
}

/**************************************************************************************************/
#if 0
static void RenderFrame(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, WORD state,
			struct MenuHandlerData *mhd)
{
    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? SHADOWPEN : SHINEPEN]);
    
    RectFill(rp, x1, y1, x2, y1);
    RectFill(rp, x1, y1 + 1, x1, y2);
    
    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? SHINEPEN : SHADOWPEN]);
    RectFill(rp, x2, y1 + 1, x2, y2);
    RectFill(rp, x1 + 1, y2, x2 - 1, y2);
}
#endif
/**************************************************************************************************/

static void HighlightItem(struct MenuItem *item, WORD itemtype, struct MenuHandlerData *mhd)
{
    struct Window   	*win = ((itemtype == ITEM_ITEM) ? mhd->menuwin : mhd->submenuwin);
    struct RastPort  	*rp = win->RPort;
    struct Rectangle 	*box = ((itemtype == ITEM_ITEM) ? ((struct Rectangle *)&mhd->activemenu->JazzX) : &mhd->submenubox);
    APTR    	    	fill; 
    WORD    	    	offx = -box->MinX;
    WORD    	    	offy = -box->MinY;
    WORD    	    	x1, y1, x2, y2;
    BOOL    	    	enabled;
    
    enabled = (item->Flags & ITEMENABLED) ? TRUE : FALSE;
    if (!(mhd->activemenu->Flags & MENUENABLED)) enabled = FALSE;
    if ((itemtype == ITEM_SUBITEM) && !(mhd->activeitem->Flags & ITEMENABLED)) enabled = FALSE;
    
    if (enabled)
    {
	item->Flags ^= HIGHITEM;

	fill = item->ItemFill;
	if ((item->Flags & HIGHITEM) && (item->SelectFill)) fill = item->SelectFill;

	x1 = offx + item->LeftEdge;
	y1 = offy + item->TopEdge;
	x2 = x1 + item->Width - 1;
	y2 = y1 + item->Height - 1;

	switch(item->Flags & HIGHFLAGS)
	{
    	    case HIGHIMAGE:
		SetDrMd(rp, JAM1);

		if(item->Flags & ITEMTEXT)
		{
#if MENUS_AMIGALOOK
		    PrintIText(rp, (struct IntuiText *)fill, x1, y1);
#else
		    struct IntuiText *it = (struct IntuiText *)fill;

		    it->FrontPen = mhd->dri->dri_Pens[TEXTPEN];
		    it->DrawMode = JAM1;
	    
		    PrintIText(rp, it, x1, y1);
#endif
		} else {
		    EraseImage(rp, (struct Image *)fill, x1, y1);
		    DrawImageState(rp, (struct Image *)fill, x1, y1, IDS_SELECTED, mhd->dri);
		}
		break;

	    case HIGHCOMP:
#if MENUS_AMIGALOOK
		SetDrMd(rp, COMPLEMENT);
		RectFill(rp, x1, y1, x2, y2);
#else
		{
		    WORD state = (item->Flags & HIGHITEM) ? IDS_SELECTED : IDS_NORMAL;
		    
		    SetDrMd(rp, JAM1);
		    SetAPen(rp, mhd->dri->dri_Pens[(state == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
		    RectFill(rp, x1, y1, x2, y2);
		    
		    RenderItem(item, itemtype, box, mhd);

		    if (state == IDS_SELECTED)
		    {
		        RenderFrame(rp, x1, y1, x2, y2, state, mhd);
		    }
		}
#endif
		break;

	    case HIGHBOX:
		SetDrMd(rp, COMPLEMENT);
		offx = mhd->scr->MenuHBorder;
		offy = mhd->scr->MenuVBorder;

		x1 -= offx; x2 += offx;
		y1 -= offy; y2 += offy;

		RectFill(rp, x1, y1, x2, y1 + offy - 1);
		RectFill(rp, x2 - offx + 1, y1 + offy, x2, y2);
		RectFill(rp, x1, y2 - offy + 1, x2 - offx, y2);
		RectFill(rp, x1, y1 + offy, x1 + offx - 1,y2 - offy);					
		break;

            case HIGHNONE:
		/* Do nothing */
		break;

	} /* switch(item->Flags & HIGHFLAGS) */
    
    } /* if (enabled) */
    
}



/**************************************************************************************************/

static WORD CalcMaxCommKeyWidth(struct Window *win, struct MenuHandlerData *mhd)
{
    WORD maxwidth = mhd->dri->dri_Font->tf_XSize;
    
    if (win)
    {
	struct MenuItem *item;

	if ((win == mhd->menuwin))
	{
	    item = mhd->activemenu->FirstItem;
	}
	else
	{
	    item = mhd->activeitem->SubItem;
	}
	
	for(; item; item = item->NextItem)
	{
	    if (item->Flags & ITEMTEXT)
	    {
	        struct IntuiText *it = (struct IntuiText *)item->ItemFill;
		
		if (it->ITextFont)
		{
		    struct TextFont *font;
		    
		    if ((font = OpenFont(it->ITextFont)))
		    {
		        if (font->tf_XSize > maxwidth) maxwidth = font->tf_XSize;
			
		        CloseFont(font);
		    }
		}
		
	    } /* if (item->Flags & ITEMTEXT) */
	    
	} /* for(; item; item = item->NextItem); */
		
    } /* if (win) */
    
    return maxwidth;
}

/**************************************************************************************************/

static void AddToSelection(struct MenuHandlerData *mhd)
{
    if ((mhd->activemenunum != -1) && (mhd->activemenu->Flags & MENUENABLED) &&
    	(mhd->activeitemnum != -1) && (mhd->activeitem->Flags & ITEMENABLED))
    {
        struct MenuItem *item = NULL;
	UWORD men = FULLMENUNUM(mhd->activemenunum, mhd->activeitemnum, mhd->activesubitemnum);
	
	if (mhd->activesubitemnum != -1)
	{
	    if (mhd->activesubitem->Flags & ITEMENABLED) item = mhd->activesubitem;
	}
	else if (!mhd->activeitem->SubItem)
	{
	    item = mhd->activeitem;
	}

	if (item && (ItemAddress(mhd->menu, men) == item))
	{
	    UWORD men = FULLMENUNUM(mhd->activemenunum, mhd->activeitemnum, mhd->activesubitemnum);
	    
	    if (mhd->firstmenupick == MENUNULL)
	    {
	        mhd->firstmenupick = men;
	    }
	    else if (men != mhd->lastmenupick)
	    {
	        struct MenuItem *checkitem, *prevcheckitem = NULL;
		UWORD checkmen = mhd->firstmenupick;
		
		/* Remove men from pick queue, if it was already in there
		   and then add it at the end of the pick queue */
		
		while(checkmen != MENUNULL)
		{
		    checkitem = ItemAddress(mhd->menu, checkmen);
		    
		    if (checkmen == men)
		    {
		        if (prevcheckitem == NULL)
			{
			    mhd->firstmenupick = checkitem->NextSelect;
			}
			else
			{
			    prevcheckitem->NextSelect = checkitem->NextSelect;
			}
		    }
		    
		    checkmen = checkitem->NextSelect;
		    prevcheckitem = checkitem;
		    
		} /* while(checkmen != MENUNULL) */
		
		checkitem->NextSelect = men;
							
	    } /* else if (men != mhd->lastmenupick) */
	    
	    mhd->lastmenupick = men;
	    item->NextSelect = MENUNULL;
	    
	} /* if (item) */
	
    } /* if ((mhd->activemenunum != -1) && (mhd->activeitemnum != -1)) */
}

/**************************************************************************************************/

/** END AROS **/

struct ZMenu
{
    struct MenuHandlerData  mhd;
};

struct ZMenu *zune_open_menu(struct Window *wnd, struct NewMenu *newmenu)
{
    struct TagItem tags[] =
    {
	{GTMN_NewLookMenus ,TRUE},
	{TAG_DONE   	    	}
    };

    struct ZMenu *zmenu;
    struct Menu *menu;
    APTR visinfo;
    zmenu = (struct ZMenu*)AllocVec(sizeof(struct ZMenu),MEMF_CLEAR);
    if (!zmenu) return NULL;

    visinfo = GetVisualInfoA(wnd->WScreen,NULL);
    if (!visinfo)
    {
	FreeVec(zmenu);
	return NULL;
    }

    if (!(menu = CreateMenusA(newmenu,NULL)))
    {
	FreeVec(zmenu);
	return NULL;
    }

    LayoutMenusA(menu, visinfo, tags);
    FreeVisualInfo(visinfo);

    Characterize(menu);
    CalculateDims(wnd, menu);

    zmenu->mhd.win  		= wnd;
    zmenu->mhd.scr  		= zmenu->mhd.win->WScreen;
    zmenu->mhd.dri 		= GetScreenDrawInfo(zmenu->mhd.scr);
    zmenu->mhd.menu 		= menu;
    zmenu->mhd.activemenunum    = -1;
    zmenu->mhd.activeitemnum    = -1;
    zmenu->mhd.activesubitemnum = -1;
    zmenu->mhd.checkmark	= zmenu->mhd.dri->dri_CheckMark;
    zmenu->mhd.amigakey	= zmenu->mhd.dri->dri_AmigaKey;
    zmenu->mhd.scrmousex	= zmenu->mhd.scr->MouseX;
    zmenu->mhd.scrmousey	= zmenu->mhd.scr->MouseY;
    zmenu->mhd.firstmenupick	= MENUNULL;

    MakeMenuBarWin(&zmenu->mhd);
    HandleMouseMove(&zmenu->mhd);
    return zmenu;
}

void zune_mouse_update(struct ZMenu *zmenu, int left_down)
{
    if (!zmenu) return;

    if (!left_down) HandleMouseMove(&zmenu->mhd);
    else  HandleMouseClick(&zmenu->mhd, 0);
}

struct MenuItem *zune_leave_menu(struct ZMenu *zmenu)
{
    HandleMouseClick(&zmenu->mhd, 1);

    return ItemAddress(zmenu->mhd.activemenu, zmenu->mhd.firstmenupick);
}

/* returns the address of the selected menuitem entry */
void zune_close_menu(struct ZMenu *zmenu)
{
    if (!zmenu) return;
 
    KillMenuBarWin(&zmenu->mhd);
    KillMenuWin(&zmenu->mhd);
    KillSubMenuWin(&zmenu->mhd);

    FreeMenus(zmenu->mhd.menu);	
    zmenu->mhd.menu = 0;

    if (zmenu->mhd.dri)
    {
	FreeScreenDrawInfo(zmenu->mhd.scr, zmenu->mhd.dri);
	zmenu->mhd.dri = 0;
    }

//	MH2Int_MakeMenusInactive(mhd->win, mhd->firstmenupick);
    zmenu->mhd.active = FALSE;
}

struct Menu *zune_get_menu_pointer(struct ZMenu *menu)
{
    return menu->mhd.menu;
}
