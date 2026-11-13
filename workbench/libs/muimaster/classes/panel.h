#ifndef _MUI_CLASSES_PANEL_H
#define _MUI_CLASSES_PANEL_H

/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Panel class public interface
*/

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

/*
 * Panel Class
 */

/* Panel class identifier */
#define MUIC_Panel "Panel.mui"

/* Panel attributes */
#define MUIA_Panel_Padding              (TAG_USER | 0x40000004)
#define MUIA_Panel_Title                (TAG_USER | 0x40000007)
#define MUIA_Panel_TitlePosition        (TAG_USER | 0x40000008)
#define MUIA_Panel_TitleTextPosition    (TAG_USER | 0x40000009)
#define MUIA_Panel_TitleVertical        (TAG_USER | 0x4000000A)
#define MUIA_Panel_Collapsible          (TAG_USER | 0x4000000B)
#define MUIA_Panel_Collapsed            (TAG_USER | 0x4000000C)
#define MUIA_Panel_DrawSeparator        (TAG_USER | 0x4000000D)
#define MUIA_Panel_TitleClickedHook     (TAG_USER | 0x4000000E)
#define MUIA_Panel_Draggable            (TAG_USER | 0x4000000F)
#define MUIA_Panel_DrawStateIndicator   (TAG_USER | 0x40000010)

/* Title position values */
#define MUIV_Panel_Title_None       0
#define MUIV_Panel_Title_Top        1
#define MUIV_Panel_Title_Left       2

/* Title text position values */
#define MUIV_Panel_Title_Text_Centered 0
#define MUIV_Panel_Title_Text_Left     1
#define MUIV_Panel_Title_Text_Right    2

extern const struct __MUIBuiltinClass _MUI_Panel_desc; /* PRIV */

/*
 * DragHandle Class
 */

/*** Name *******************************************************************/
#define MUIC_DragHandle "DragHandle.mui"

/*** Identifier base ********************************************************/
#define MUIB_DragHandle (MUIB_ZUNE | 0x00002100)

/*** Attributes *************************************************************/
#define MUIA_DragHandle_Vertical (MUIB_DragHandle | 0x0000) /* [ISG] Object * - How to render draghandle */

/*** Methods ****************************************************************/

/*** Special method IDs *****************************************************/

/*** Structures *************************************************************/

/*** Private ****************************************************************/

extern const struct __MUIBuiltinClass _MUI_DragHandle_desc; /* PRIV */

/*
 * PanelTitle class
 */

/* PanelTitle class identifier */
#define MUIC_PanelTitle "PanelTitle.mui"

/* PanelTitle attributes */
#define MUIA_PanelTitle_Text                (TAG_USER | 0x40001001)
#define MUIA_PanelTitle_Position            (TAG_USER | 0x40001002)
#define MUIA_PanelTitle_TextPosition        (TAG_USER | 0x40001003)
#define MUIA_PanelTitle_Vertical            (TAG_USER | 0x40001004)
#define MUIA_PanelTitle_Collapsible         (TAG_USER | 0x40001005)
#define MUIA_PanelTitle_Collapsed           (TAG_USER | 0x40001006)
#define MUIA_PanelTitle_ShowSeparator       (TAG_USER | 0x40001007)
#define MUIA_PanelTitle_ClickHook           (TAG_USER | 0x40001008)
#define MUIA_PanelTitle_DrawStateIndicator  (TAG_USER | 0x40001009)

/* Position values */
#define MUIV_PanelTitle_Position_Top        1
#define MUIV_PanelTitle_Position_Left       2

/* Text position values */
#define MUIV_PanelTitle_TextPosition_Centered   0
#define MUIV_PanelTitle_TextPosition_Left       1
#define MUIV_PanelTitle_TextPosition_Right      2

/* Methods */
#define MUIM_PanelTitle_Toggle              (TAG_USER | 0x40001101)

/* Method structures */
struct MUIP_PanelTitle_Toggle
{
    STACKED ULONG MethodID;
};

extern const struct __MUIBuiltinClass _MUI_PanelTitle_desc; /* PRIV */

#endif /* _MUI_CLASSES_PANEL_H */
