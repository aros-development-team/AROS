/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _CLASSES_LISTVIEW_H
#define _CLASSES_LISTVIEW_H

#define MUIC_Listview "Listview.mui"

/* Listview attributes */
#define MUIA_Listview_ClickColumn    (TAG_USER|0x0042d1b3) /* V7  ..g LONG    */
#define MUIA_Listview_DefClickColumn (TAG_USER|0x0042b296) /* V7  isg LONG    */
#define MUIA_Listview_DoubleClick    (TAG_USER|0x00424635) /* V4  i.g BOOL    */
#define MUIA_Listview_DragType       (TAG_USER|0x00425cd3) /* V11 isg LONG    */
#define MUIA_Listview_Input          (TAG_USER|0x0042682d) /* V4  i.. BOOL    */
#define MUIA_Listview_List           (TAG_USER|0x0042bcce) /* V4  i.g Object  */
#define MUIA_Listview_MultiSelect    (TAG_USER|0x00427e08) /* V7  i.. LONG    */
#define MUIA_Listview_ScrollerPos    (TAG_USER|0x0042b1b4) /* V10 i.. BOOL    */
#define MUIA_Listview_SelectChange   (TAG_USER|0x0042178f) /* V4  ..g BOOL    */

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

#endif
