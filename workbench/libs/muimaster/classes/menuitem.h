/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_MENUSTRIP_H
#define _MUI_CLASSES_MENUSTRIP_H

/**************************************************************************
 Menustrip
**************************************************************************/
#define MUIC_Menustrip "Menustrip.mui"
/* Menustrip attributes */
#define MUIA_Menustrip_Enabled (TAG_USER|0x0042815b) /* MUI: V8  isg BOOL */

/**************************************************************************
 Menu
**************************************************************************/
#define MUIC_Menu "Menu.mui"
/* Menu attributes */
#define MUIA_Menu_Enabled (TAG_USER|0x0042ed48) /* MUI: V8  isg BOOL   */
#define MUIA_Menu_Title   (TAG_USER|0x0042a0e3) /* MUI: V8  isg STRPTR */

/**************************************************************************
 Menuitem
**************************************************************************/
#define MUIC_Menuitem "Menuitem.mui"
/* Menuitem attributes */
#define MUIA_Menuitem_Checked       (TAG_USER|0x0042562a) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Checkit       (TAG_USER|0x00425ace) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_CommandString (TAG_USER|0x0042b9cc) /* MUI: V16 isg BOOL              */
#define MUIA_Menuitem_Enabled       (TAG_USER|0x0042ae0f) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Exclude       (TAG_USER|0x00420bc6) /* MUI: V8  isg LONG              */
#define MUIA_Menuitem_Shortcut      (TAG_USER|0x00422030) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Title         (TAG_USER|0x004218be) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Toggle        (TAG_USER|0x00424d5c) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Trigger       (TAG_USER|0x00426f32) /* MUI: V8  ..g struct MenuItem * */

#define MUIV_Menuitem_Shortcut_Check (-1)

/* The following is Zune Private */
#define MUIA_Menuitem_NewMenu       (TAG_USER|0x10426f33) /* Zune: V1 ..g struct NewMenu *  */


extern const struct __MUIBuiltinClass _MUI_Menuitem_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_Menu_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_Menustrip_desc; /* PRIV */

#endif

