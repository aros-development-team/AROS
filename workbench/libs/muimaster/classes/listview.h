#ifndef _CLASSES_LISTVIEW_H
#define _CLASSES_LISTVIEW_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Listview "Listview.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Listview                (MUIB_ZUNE | 0x00001900)

/*** Attributes *************************************************************/
#define MUIA_Listview_ClickColumn    (MUIB_MUI|0x0042d1b3) /* V7  ..g LONG    */
#define MUIA_Listview_DefClickColumn (MUIB_MUI|0x0042b296) /* V7  isg LONG    */
#define MUIA_Listview_DoubleClick    (MUIB_MUI|0x00424635) /* V4  i.g BOOL    */
#define MUIA_Listview_DragType       (MUIB_MUI|0x00425cd3) /* V11 isg LONG    */
#define MUIA_Listview_Input          (MUIB_MUI|0x0042682d) /* V4  i.. BOOL    */
#define MUIA_Listview_List           (MUIB_MUI|0x0042bcce) /* V4  i.g Object  */
#define MUIA_Listview_MultiSelect    (MUIB_MUI|0x00427e08) /* V7  i.. LONG    */
#define MUIA_Listview_ScrollerPos    (MUIB_MUI|0x0042b1b4) /* V10 i.. BOOL    */
#define MUIA_Listview_SelectChange   (MUIB_MUI|0x0042178f) /* V4  ..g BOOL    */

enum
{
    MUIV_Listview_DragType_None = 0,
    MUIV_Listview_DragType_Immediate,
};

enum
{
    MUIV_Listview_MultiSelect_None = 0,
    MUIV_Listview_MultiSelect_Default,
    MUIV_Listview_MultiSelect_Shifted,
    MUIV_Listview_MultiSelect_Always,
};

enum
{
    MUIV_Listview_ScrollerPos_Default = 0,
    MUIV_Listview_ScrollerPos_Left,
    MUIV_Listview_ScrollerPos_Right,
    MUIV_Listview_ScrollerPos_None,
};


extern const struct __MUIBuiltinClass _MUI_Listview_desc; /* PRIV */

#endif /* _CLASSES_LISTVIEW_H */
