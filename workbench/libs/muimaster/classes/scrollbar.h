#ifndef _MUI_CLASSES_SCROLLBAR_H
#define _MUI_CLASSES_SCROLLBAR_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scrollbar       "Scrollbar.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrollbar       (MUIB_ZUNE | 0x00002e00)

/*** Attributes *************************************************************/
#define MUIA_Scrollbar_Type  (MUIB_MUI|0x0042fb6b) /* V11 i.. LONG */

enum
{
    MUIV_Scrollbar_Type_Default = 0,
    MUIV_Scrollbar_Type_Bottom,
    MUIV_Scrollbar_Type_Top,
    MUIV_Scrollbar_Type_Sym,
};


extern const struct __MUIBuiltinClass _MUI_Scrollbar_desc; /* PRIV */

#endif /* _MUI_CLASSES_SCROLLBAR_H */
