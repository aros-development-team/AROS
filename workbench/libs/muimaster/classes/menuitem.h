/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_MENUSTRIP_H
#define _MUI_CLASSES_MENUSTRIP_H

/**************************************************************************
 Menustrip
**************************************************************************/
#define MUIC_Menustrip "Menustrip.mui"
/* Menustrip attributes */
#define MUIA_Menustrip_Enabled (MUIB_MUI|0x0042815b) /* MUI: V8  isg BOOL */

/**************************************************************************
 Menu
**************************************************************************/
#define MUIC_Menu "Menu.mui"
/* Menu attributes */
#define MUIA_Menu_Enabled (MUIB_MUI|0x0042ed48) /* MUI: V8  isg BOOL   */
#define MUIA_Menu_Title   (MUIB_MUI|0x0042a0e3) /* MUI: V8  isg STRPTR */

/**************************************************************************
 Menuitem
**************************************************************************/
#define MUIC_Menuitem "Menuitem.mui"
/* Menuitem attributes */
#define MUIA_Menuitem_Checked       (MUIB_MUI|0x0042562a) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Checkit       (MUIB_MUI|0x00425ace) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_CommandString (MUIB_MUI|0x0042b9cc) /* MUI: V16 isg BOOL              */
#define MUIA_Menuitem_Enabled       (MUIB_MUI|0x0042ae0f) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Exclude       (MUIB_MUI|0x00420bc6) /* MUI: V8  isg LONG              */
#define MUIA_Menuitem_Shortcut      (MUIB_MUI|0x00422030) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Title         (MUIB_MUI|0x004218be) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Toggle        (MUIB_MUI|0x00424d5c) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Trigger       (MUIB_MUI|0x00426f32) /* MUI: V8  ..g struct MenuItem * */

#define MUIV_Menuitem_Shortcut_Check (-1)

/* The following is Zune Private */
#define MUIA_Menuitem_NewMenu       (MUIB_MUI|0x10426f33) /* Zune: V1 ..g struct NewMenu *  */


extern const struct __MUIBuiltinClass _MUI_Menuitem_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_Menu_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_Menustrip_desc; /* PRIV */

#endif

