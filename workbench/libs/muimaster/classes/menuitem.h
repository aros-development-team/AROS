#ifndef _MUI_CLASSES_MENUSTRIP_H
#define _MUI_CLASSES_MENUSTRIP_H

/****************************************************************************/
/** Menustrip                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menustrip[];
#else
#define MUIC_Menustrip "Menustrip.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menustrip_Enabled              0x8042815b /* V8  isg BOOL              */

/****************************************************************************/
/** Menu                                                                   **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menu[];
#else
#define MUIC_Menu "Menu.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menu_Enabled                   0x8042ed48 /* V8  isg BOOL              */
#define MUIA_Menu_Title                     0x8042a0e3 /* V8  isg STRPTR            */



/****************************************************************************/
/** Menuitem                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Menuitem[];
#else
#define MUIC_Menuitem "Menuitem.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Menuitem_Checked               0x8042562a /* V8  isg BOOL              */
#define MUIA_Menuitem_Checkit               0x80425ace /* V8  isg BOOL              */
#define MUIA_Menuitem_CommandString         0x8042b9cc /* V16 isg BOOL              */
#define MUIA_Menuitem_Enabled               0x8042ae0f /* V8  isg BOOL              */
#define MUIA_Menuitem_Exclude               0x80420bc6 /* V8  isg LONG              */
#define MUIA_Menuitem_Shortcut              0x80422030 /* V8  isg STRPTR            */
#define MUIA_Menuitem_Title                 0x804218be /* V8  isg STRPTR            */
#define MUIA_Menuitem_Toggle                0x80424d5c /* V8  isg BOOL              */
#define MUIA_Menuitem_Trigger               0x80426f32 /* V8  ..g struct MenuItem * */

#define MUIA_Menuitem_NewMenu               0x80426f33 /* ZV1 ..g struct NewMenu *  */

#define MUIV_Menuitem_Shortcut_Check -1

extern const struct __MUIBuiltinClass _MUI_Menuitem_desc;
extern const struct __MUIBuiltinClass _MUI_Menu_desc;
extern const struct __MUIBuiltinClass _MUI_Menustrip_desc;


#endif

