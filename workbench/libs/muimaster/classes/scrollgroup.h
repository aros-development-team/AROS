/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLGROUP_H
#define _MUI_CLASSES_SCROLLGROUP_H

#define MUIC_Scrollgroup "Scrollgroup.mui"

/* Scrollgroup attributes */
#define MUIA_Scrollgroup_Contents     (MUIB_MUI|0x00421261) /* V4  i.g Object * */
#define MUIA_Scrollgroup_FreeHoriz    (MUIB_MUI|0x004292f3) /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_FreeVert     (MUIB_MUI|0x004224f2) /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_HorizBar     (MUIB_MUI|0x0042b63d) /* V16 ..g Object * */
#define MUIA_Scrollgroup_UseWinBorder (MUIB_MUI|0x004284c1) /* V13 i.. BOOL     */
#define MUIA_Scrollgroup_VertBar      (MUIB_MUI|0x0042cdc0) /* V16 ..g Object * */

extern const struct __MUIBuiltinClass _MUI_Scrollgroup_desc; /* PRIV */

#endif
