/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_POPSTRING_H
#define _MUI_CLASSES_POPSTRING_H

#define MUIC_Popstring "Popstring.mui"

/* Popstring methods */
#define MUIM_Popstring_Close  (METHOD_USER|0x0042dc52) /* MUI: V7  */
#define MUIM_Popstring_Open   (METHOD_USER|0x004258ba) /* MUI: V7  */
struct  MUIP_Popstring_Close  {ULONG MethodID; LONG result;};
struct  MUIP_Popstring_Open   {ULONG MethodID; };

/* Popstring attributes */
#define MUIA_Popstring_Button    (TAG_USER|0x0042d0b9) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_CloseHook (TAG_USER|0x004256bf) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_OpenHook  (TAG_USER|0x00429d00) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_String    (TAG_USER|0x004239ea) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_Toggle    (TAG_USER|0x00422b7a) /* MUI: V7  isg BOOL          */

extern const struct __MUIBuiltinClass _MUI_Popstring_desc; /* PRIV */

#endif
