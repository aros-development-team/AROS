#ifndef _MUI_CLASSES_PALETTE_H
#define _MUI_CLASSES_PALETTE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id: cycle.h 17866 2003-06-04 12:23:25Z chodorowski $
*/

/*** Name *******************************************************************/
#define MUIC_Palette "Palette.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Palette         (MUIB_ZUNE | 0x00008a00)

/*** Attributes *************************************************************/
#define MUIA_Palette_Entries                0x8042a3d8 /* V6  i.g struct MUI_Palette_Entry * */
#define MUIA_Palette_Groupable              0x80423e67 /* V6  isg BOOL              */
#define MUIA_Palette_Names                  0x8042c3a2 /* V6  isg char **           */

#define MUIV_Palette_Entry_End -1

struct MUI_Palette_Entry
{
    LONG  mpe_ID;
    ULONG mpe_Red;
    ULONG mpe_Green;
    ULONG mpe_Blue;
    LONG  mpe_Group;
};

extern const struct __MUIBuiltinClass _MUI_Palette_desc; /* PRIV */

#endif /* _MUI_PALETTE_H */
