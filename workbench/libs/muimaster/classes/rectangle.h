/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_RECTANGLE_H
#define _MUI_CLASSES_RECTANGLE_H

#define MUIC_Rectangle "Rectangle.mui"

/* Rectangle attributes */
#define MUIA_Rectangle_BarTitle (MUIB_MUI|0x00426689) /* V11 i.g STRPTR */
#define MUIA_Rectangle_HBar     (MUIB_MUI|0x0042c943) /* V7  i.g BOOL   */
#define MUIA_Rectangle_VBar     (MUIB_MUI|0x00422204) /* V7  i.g BOOL   */

extern const struct __MUIBuiltinClass _MUI_Rectangle_desc; /* PRIV */

#endif
