#ifndef _MUI_CLASSES_POPFRAME_H
#define _MUI_CLASSES_POPFRAME_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popframe              "Popframe.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popframe              (MUIB_ZUNE | 0x00002200)

/*** Methods ****************************************************************/
#define MUIM_Popframe_OpenWindow   (MUIB_Popframe | 0x00000000)     /* PRIV */
#define MUIM_Popframe_CloseWindow  (MUIB_Popframe | 0x00000001)     /* PRIV */
struct MUIP_Popframe_OpenWindow    {STACKED ULONG MethodID;};           /* PRIV */
struct MUIP_Popframe_CloseWindow   {STACKED ULONG MethodID; STACKED LONG ok;};  /* PRIV */


extern const struct __MUIBuiltinClass _MUI_Popframe_desc; /* PRIV */

#endif /* _MUI_CLASSES_POPFRAME_H */
