/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_CYCLE_H
#define _MUI_CLASSES_CYCLE_H

#define MUIC_Cycle "Cycle.mui"

/* Cycle attributes */
#define MUIA_Cycle_Active  (MUIB_MUI|0x00421788) /* MUI:V4  isg LONG      */
#define MUIA_Cycle_Entries (MUIB_MUI|0x00420629) /* MUI:V4  i.. STRPTR    */

enum
{
    MUIV_Cycle_Active_Next = -1,
    MUIV_Cycle_Active_Prev = -2,
};

extern const struct __MUIBuiltinClass _MUI_Cycle_desc; /* PRIV */

#endif
