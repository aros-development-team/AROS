#ifndef _MUI_CLASSES_POPLIST_H
#define _MUI_CLASSES_POPLIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Poplist                "Poplist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Poplist                (MUIB_ZUNE | 0x00002500)

/*** Attributes *************************************************************/
#define MUIA_Poplist_Array          (MUIB_MUI|0x0042084c) /* MUI: V8  i.. char ** */

extern const struct __MUIBuiltinClass _MUI_Poplist_desc; /* PRIV */

#endif /* _MUI_CLASSES_POPLIST_H */
