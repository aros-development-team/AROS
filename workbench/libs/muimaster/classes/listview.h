/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _CLASSES_LISTVIEW_H
#define _CLASSES_LISTVIEW_H

/****************************************************************************/
/** Listview                                                               **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Listview[];
#else
#define MUIC_Listview "Listview.mui"
#endif

/* Attributes */

#define MUIA_Listview_ClickColumn           0x8042d1b3 /* V7  ..g LONG              */
#define MUIA_Listview_DefClickColumn        0x8042b296 /* V7  isg LONG              */
#define MUIA_Listview_DoubleClick           0x80424635 /* V4  i.g BOOL              */
#define MUIA_Listview_DragType              0x80425cd3 /* V11 isg LONG              */
#define MUIA_Listview_Input                 0x8042682d /* V4  i.. BOOL              */
#define MUIA_Listview_List                  0x8042bcce /* V4  i.g Object *          */
#define MUIA_Listview_MultiSelect           0x80427e08 /* V7  i.. LONG              */
#define MUIA_Listview_ScrollerPos           0x8042b1b4 /* V10 i.. BOOL              */
#define MUIA_Listview_SelectChange          0x8042178f /* V4  ..g BOOL              */

#define MUIV_Listview_DragType_None 0
#define MUIV_Listview_DragType_Immediate 1
#define MUIV_Listview_MultiSelect_None 0
#define MUIV_Listview_MultiSelect_Default 1
#define MUIV_Listview_MultiSelect_Shifted 2
#define MUIV_Listview_MultiSelect_Always 3
#define MUIV_Listview_ScrollerPos_Default 0
#define MUIV_Listview_ScrollerPos_Left 1
#define MUIV_Listview_ScrollerPos_Right 2
#define MUIV_Listview_ScrollerPos_None 3

#endif
