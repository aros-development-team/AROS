#ifndef _MUI_CLASSES_PROP_H
#define _MUI_CLASSES_PROP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Prop           "Prop.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Prop           (MUIB_ZUNE | 0x00002900)  

/*** Methods ****************************************************************/
#define MUIM_Prop_Decrease  (MUIB_MUI|0x00420dd1) /* MUI: V16 */
#define MUIM_Prop_Increase  (MUIB_MUI|0x0042cac0) /* MUI: V16 */
struct  MUIP_Prop_Decrease  {STACKED ULONG MethodID; STACKED LONG amount;};
struct  MUIP_Prop_Increase  {STACKED ULONG MethodID; STACKED LONG amount;};

/*** Attributes *************************************************************/
#define MUIA_Prop_Entries        (MUIB_MUI|0x0042fbdb) /* MUI: V4  isg LONG */
#define MUIA_Prop_First          (MUIB_MUI|0x0042d4b2) /* MUI: V4  isg LONG */
#define MUIA_Prop_Horiz          (MUIB_MUI|0x0042f4f3) /* MUI: V4  i.g BOOL */
#define MUIA_Prop_Slider         (MUIB_MUI|0x00429c3a) /* MUI: V4  isg BOOL */
#define MUIA_Prop_UseWinBorder   (MUIB_MUI|0x0042deee) /* MUI: V13 i.. LONG */
#define MUIA_Prop_Visible        (MUIB_MUI|0x0042fea6) /* MUI: V4  isg LONG */

#define MUIA_Prop_OnlyTrigger    (MUIB_Prop | 0x00000000) /* Zune: PRIV .s. BOOL */

enum
{
    MUIV_Prop_UseWinBorder_None = 0,
    MUIV_Prop_UseWinBorder_Left,
    MUIV_Prop_UseWinBorder_Right,
    MUIV_Prop_UseWinBorder_Bottom,
};

#define MUIA_Prop_Release        (MUIB_MUI|0x00429839) /* MUI:    ..g BOOL  PRIV */
#define MUIA_Prop_DeltaFactor    (MUIB_MUI|0x00427c5e) /* MUI:    is. LONG */
#define MUIA_Prop_DoSmooth       (MUIB_MUI|0x004236ce) /* MUI: V4 i.. LONG */


extern const struct __MUIBuiltinClass _MUI_Prop_desc; /* PRIV */

#endif /* _MUI_CLASSES_PROP_H */
