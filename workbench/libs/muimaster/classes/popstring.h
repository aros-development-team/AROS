/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_POPSTRING_H
#define _MUI_CLASSES_POPSTRING_H

#define MUIC_Popstring "Popstring.mui"

/* Popstring methods */
#define MUIM_Popstring_Close  (MUIB_MUI|0x0042dc52) /* MUI: V7  */
#define MUIM_Popstring_Open   (MUIB_MUI|0x004258ba) /* MUI: V7  */
struct  MUIP_Popstring_Close  {ULONG MethodID; LONG result;};
struct  MUIP_Popstring_Open   {ULONG MethodID; };

/* Popstring attributes */
#define MUIA_Popstring_Button    (MUIB_MUI|0x0042d0b9) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_CloseHook (MUIB_MUI|0x004256bf) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_OpenHook  (MUIB_MUI|0x00429d00) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_String    (MUIB_MUI|0x004239ea) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_Toggle    (MUIB_MUI|0x00422b7a) /* MUI: V7  isg BOOL          */

extern const struct __MUIBuiltinClass _MUI_Popstring_desc; /* PRIV */

#endif
