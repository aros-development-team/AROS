/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_PROP_H
#define _MUI_CLASSES_PROP_H

#define MUIC_Prop "Prop.mui"

/* Prop methods */
#define MUIM_Prop_Decrease  (METHOD_USER|0x00420dd1) /* MUI: V16 */
#define MUIM_Prop_Increase  (METHOD_USER|0x0042cac0) /* MUI: V16 */
struct  MUIP_Prop_Decrease  {ULONG MethodID; LONG amount;};
struct  MUIP_Prop_Increase  {ULONG MethodID; LONG amount;};

/* Prop attributes */
#define MUIA_Prop_Entries        (TAG_USER|0x0042fbdb) /* MUI: V4  isg LONG */
#define MUIA_Prop_First          (TAG_USER|0x0042d4b2) /* MUI: V4  isg LONG */
#define MUIA_Prop_Horiz          (TAG_USER|0x0042f4f3) /* MUI: V4  i.g BOOL */
#define MUIA_Prop_Slider         (TAG_USER|0x00429c3a) /* MUI: V4  isg BOOL */
#define MUIA_Prop_UseWinBorder   (TAG_USER|0x0042deee) /* MUI: V13 i.. LONG */
#define MUIA_Prop_Visible        (TAG_USER|0x0042fea6) /* MUI: V4  isg LONG */

#define MUIA_Prop_OnlyTrigger    (TAG_USER|0x1042fea7) /* Zune: PRIV */

enum
{
    MUIV_Prop_UseWinBorder_None = 0,
    MUIV_Prop_UseWinBorder_Left,
    MUIV_Prop_UseWinBorder_Right,
    MUIV_Prop_UseWinBorder_Bottom,
};

#define MUIA_Prop_FirstReal      (TAG_USER|0x004205dc) /* MUI: */
#define MUIA_Prop_PropRelease    (TAG_USER|0x00429839) /* MUI: */
#define MUIA_Prop_DeltaFactor    (TAG_USER|0x00427c5e) /* MUI:    is. LONG */
#define MUIA_Prop_DoSmooth       (TAG_USER|0x004236ce) /* MUI: V4 i.. LONG */

extern const struct __MUIBuiltinClass _MUI_Prop_desc; /* PRIV */

#endif
