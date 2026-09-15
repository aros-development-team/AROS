#ifndef _MUI_CLASSES_FLEXGROUP_H
#define _MUI_CLASSES_FLEXGROUP_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Flexgroup             "Flexgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Flexgroup             (MUIB_ZUNE | 0x00004200)

/*** Attributes *************************************************************/
#define MUIA_Flexgroup_Direction \
    (MUIB_Flexgroup | 0x00000000)   /* Zune: V1 isg LONG */
#define MUIA_Flexgroup_Justify \
    (MUIB_Flexgroup | 0x00000001)   /* Zune: V1 isg LONG */
#define MUIA_Flexgroup_Align \
    (MUIB_Flexgroup | 0x00000002)   /* Zune: V1 isg LONG */
#define MUIA_Flexgroup_Gap \
    (MUIB_Flexgroup | 0x00000003)   /* Zune: V1 isg LONG */

enum
{
    MUIV_Flexgroup_Direction_Row = 0,
    MUIV_Flexgroup_Direction_Column,
};

enum
{
    MUIV_Flexgroup_Justify_Start = 0,
    MUIV_Flexgroup_Justify_End,
    MUIV_Flexgroup_Justify_Center,
    MUIV_Flexgroup_Justify_SpaceBetween,
    MUIV_Flexgroup_Justify_SpaceAround,
    MUIV_Flexgroup_Justify_SpaceEvenly,
};

enum
{
    MUIV_Flexgroup_Align_Stretch = 0,
    MUIV_Flexgroup_Align_Start,
    MUIV_Flexgroup_Align_End,
    MUIV_Flexgroup_Align_Center,
};

/* Per-child growing and shrinking uses the weight along the main axis:
   MUIA_HorizWeight for Row, MUIA_VertWeight for Column.  A weight of 0
   pins the child to its natural size, like CSS "flex: none". */

extern const struct __MUIBuiltinClass _MUI_Flexgroup_desc;      /* PRIV */

#endif /* _MUI_CLASSES_FLEXGROUP_H */
