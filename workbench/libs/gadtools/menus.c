/*
   (C) 1997-98 AROS - The Amiga Research OS
   $Id$

   Desc: GadTools menu creation functions
   Lang: English
*/
#include <stdio.h>
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


void appendmenu(struct Menu * firstmenu,
                struct Menu * lastmenu)
{
  while (NULL != firstmenu->NextMenu)
    firstmenu = firstmenu->NextMenu;
    
  firstmenu->NextMenu = lastmenu;
}


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


void appendsubitem(struct MenuItem * curitem,
                   struct MenuItem * cursubitem)
{
  while (NULL != curitem->SubItem) 
    curitem = curitem->SubItem;
  
  curitem->SubItem = cursubitem;
}


struct Menu * makemenutitle(struct NewMenu * newmenu,
                            struct TagItem * taglist)
{
  struct Menu * menu;
  menu = (struct Menu *)AllocMem(sizeof(struct Menu)+sizeof(APTR), 
                                 MEMF_CLEAR);
  
  if (NULL != menu)
  {
    menu->Flags    = newmenu->nm_Flags;
    menu->MenuName = newmenu->nm_Label;
    GTMENU_USERDATA(menu) = newmenu->nm_UserData;
  }
  
  return menu;
}


struct MenuItem * makemenuitem(struct NewMenu * newmenu,
                               BOOL is_image,
                               struct TagItem * taglist,
                               struct GadToolsBase_intern * GadToolsBase)
{
  struct MenuItem * menuitem;
  menuitem = (struct MenuItem *)
               AllocMem(sizeof(struct MenuItem)+sizeof(APTR),
                        MEMF_CLEAR);
  
  if (NULL != menuitem)
  { 
    menuitem->Flags         = newmenu->nm_Flags;

//    menuitem->???           = newmenu->nm_CommKey;

    menuitem->MutualExclude = newmenu->nm_MutualExclude;
    GTMENUITEM_USERDATA(menuitem) = newmenu->nm_UserData;
    
    
    if (FALSE == is_image)
    {
      /*
      ** Text
      */
      struct IntuiText * it;
      it = (struct IntuiText *) AllocMem(sizeof(struct IntuiText),
                                         MEMF_CLEAR);
      if (NULL == it)
      {
        FreeMem(menuitem, sizeof(struct MenuItem)+sizeof(APTR));
        return NULL;
      }
      menuitem->ItemFill = (APTR)it;
      
      it->FrontPen = GetTagData(GTMN_FrontPen, 0, taglist);
      it->DrawMode = JAM2;
      it->IText    = newmenu->nm_Label;

      menuitem->Flags |= ITEMTEXT;
    }
    else
    {
      /*
      ** An image.
      */
      menuitem->ItemFill = (APTR)newmenu->nm_Label;
      menuitem->Flags &= ~ITEMTEXT;      
    }
  }
  
  return menuitem;
}


void freeitems(struct Menu * m)
{
  struct MenuItem * mi = m->FirstItem;
  while (mi)
  {
    struct MenuItem * _mi = mi->NextItem;
    
    if (NULL != mi->SubItem)
      freesubitems(mi);
    
    if (mi->Flags & ITEMTEXT)
      FreeMem(mi->ItemFill, sizeof(struct IntuiText));
    
    FreeMem(mi, sizeof(struct MenuItem) + sizeof(APTR));
    mi = _mi;
  }
}


void freesubitems(struct MenuItem * mi)
{
  /*
  ** Free a subitems of this MenuItem.
  */
  struct MenuItem * si = mi->SubItem;
  while (NULL != si)
  {
    struct MenuItem * _si = si->NextItem;
    
    /*
    ** If this sub item has sub items then also free those.
    */
    if (NULL != si->SubItem)
      freesubitems(si);

    if (si->Flags & ITEMTEXT)
      FreeMem(mi->ItemFill, sizeof(struct IntuiText));
    
    FreeMem(si, sizeof(struct MenuItem) + sizeof(APTR));
    si = _si;
  }
}


BOOL layoutmenuitems(struct MenuItem * firstitem,
                     struct VisualInfo * vi,
                     struct TagItem * taglist,
                     struct GadToolsBase_intern * GadToolsBase)
{
  struct MenuItem * menuitem = firstitem;
  ULONG curX = 0;
  ULONG curY = 0;
  ULONG maxX = 0;

  while (NULL != menuitem)
  {
    ULONG addwidth = 0;
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
      struct Image * amikeyimage = (struct Image *) 
                     GetTagData(GTMN_AmigaKey,
                                (ULONG)vi->vi_dri->dri_AmigaKey,
                                taglist);

      if (NULL != amikeyimage)
      {
        addwidth += amikeyimage->Width;
      }

    }
    
    if (0 != (menuitem->Flags & (CHECKIT | MENUTOGGLE)))
    {
      /*
      ** A checkmark will appear on the left.
      */
      struct Image * chkimage = (struct Image *) 
                     GetTagData(GTMN_Checkmark,
                                (ULONG)vi->vi_dri->dri_CheckMark,
                                taglist);
      
      if (NULL != chkimage)
      {
        menuitem->LeftEdge = chkimage->Width;
        addwidth += chkimage->Width;
      }
      
    }

    if (0 != (menuitem->Flags & ITEMTEXT))
    {
      /*
      ** Text
      */
      struct IntuiText * it = (struct IntuiText *)menuitem->ItemFill;
      struct TagItem * ti;
      
      if (NULL == it)
        return FALSE;
      
      if (NULL != (ti = FindTagItem(GTMN_FrontPen, taglist)))
        it->FrontPen = ti->ti_Data;
        
      it->ITextFont = (struct TextAttr *)GetTagData(GTMN_TextAttr,
                                                    0,
                                                    taglist);
                                                    
      menuitem->Width = IntuiTextLength(it) + addwidth;
      
      if (maxX < (menuitem->Width + menuitem->LeftEdge))
        maxX = menuitem->Width + menuitem->LeftEdge;
        
      /*
      ** Calculate the y coordinate for the next menu item.
      */
      if (NULL == it->ITextFont)
      {
        /*
        ** No font is provided, so I will work with the standard system font.
        */ 
        curY += GfxBase->DefaultFont->tf_YSize;
      }
      else
      {
        struct TextFont * font;
        if (NULL != (font = OpenFont(it->ITextFont)))
        {
          curY += font->tf_YSize;
          CloseFont(font);
        }
        else
          return FALSE; 
      }
    }
    else
    {
      /*
      ** Image
      */
      struct Image * im = (struct Image *)menuitem->ItemFill;

      if (NULL == im)
        return FALSE;

      menuitem->Width  = im->Width;
      menuitem->Height = im->Height;
      
      curY += im->Height;
    }
    
    if (maxX < (menuitem->Width + menuitem->LeftEdge + addwidth))
      maxX = menuitem->Width + menuitem->LeftEdge + addwidth;

    
    /*
    ** In case this menu becomes too high it will have more columns.
    */
    if (curY > vi->vi_dri->dri_Resolution.Y)
    {
      curY = 0;
      curX = maxX;
      menuitem->LeftEdge = curX;
      menuitem->TopEdge  = curY;
      
      maxX = menuitem->LeftEdge + menuitem->Width;
    }

    /*
    ** Process the next menu item
    */    
    menuitem = menuitem->NextItem;
  }
  
  return TRUE;
}

BOOL layoutsubitems(struct MenuItem * motheritem,
                    struct VisualInfo * vi,
                    struct TagItem * taglist,
                    struct GadToolsBase_intern * GadToolsBase)
{
  struct MenuItem * menuitem = motheritem->SubItem;
  while (NULL != menuitem)
  {
  
    menuitem = menuitem->NextItem;
  }
  
  return TRUE;
}