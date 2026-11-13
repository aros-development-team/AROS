#ifndef _MUI_CLASSES_SCROLLGROUP_H
#define _MUI_CLASSES_SCROLLGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scrollgroup              "Scrollgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrollgroup              (MUIB_ZUNE | 0x00002f00)

/*** Attributes *************************************************************/
#define MUIA_Scrollgroup_AutoBars \
    (MUIB_MUI | 0x0042f50e)     /* V20 isg BOOL     */
#define MUIA_Scrollgroup_Contents \
    (MUIB_MUI | 0x00421261)     /* V4  i.g Object * */
#define MUIA_Scrollgroup_FreeHoriz \
    (MUIB_MUI | 0x004292f3)     /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_FreeVert \
    (MUIB_MUI | 0x004224f2)     /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_HorizBar \
    (MUIB_MUI | 0x0042b63d)     /* V16 ..g Object * */
#define MUIA_Scrollgroup_NoHorizBar \
    (MUIB_MUI | 0x0042cab1)     /* V20 isg BOOL     */
#define MUIA_Scrollgroup_NoVertBar \
    (MUIB_MUI | 0x004264c3)     /* V20 isg BOOL     */
#define MUIA_Scrollgroup_UseWinBorder \
    (MUIB_MUI | 0x004284c1)     /* V13 i.. BOOL     */
#define MUIA_Scrollgroup_VertBar \
    (MUIB_MUI | 0x0042cdc0)     /* V16 ..g Object * */


extern const struct __MUIBuiltinClass _MUI_Scrollgroup_desc;    /* PRIV */

#endif /* _MUI_CLASSES_SCROLLGROUP_H */
