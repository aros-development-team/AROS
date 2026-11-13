#ifndef _MUI_CLASSES_CYCLE_H
#define _MUI_CLASSES_CYCLE_H

/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Cycle "Cycle.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Cycle         (MUIB_ZUNE | 0x00000a00)

/*** Attributes *************************************************************/
#define MUIA_Cycle_Active  (MUIB_MUI | 0x00421788) /* MUI:V4  isg LONG      */
#define MUIA_Cycle_Entries (MUIB_MUI | 0x00420629) /* MUI:V4  is. STRPTR    */

enum
{
    MUIV_Cycle_Active_Next = -1,
    MUIV_Cycle_Active_Prev = -2,
};

extern const struct __MUIBuiltinClass _MUI_Cycle_desc; /* PRIV */

#endif /* _MUI_CLASSES_CYCLE_H */
