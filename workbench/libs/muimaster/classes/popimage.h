/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_POPIMAGE_H
#define _MUI_CLASSES_POPIMAGE_H

#define MUIC_Popimage "Popimage.mui"

/* Popimage methods */
#define MUIM_Popimage_OpenWindow   (MUIB_MUI|0x0042a548)     /* PRIV */
#define MUIM_Popimage_CloseWindow  (MUIB_MUI|0x0042a549)     /* PRIV */
struct MUIP_Popimage_OpenWindow    {ULONG MethodID;};           /* PRIV */
struct MUIP_Popimage_CloseWindow   {ULONG MethodID; LONG ok;};  /* PRIV */

extern const struct __MUIBuiltinClass _MUI_Popimage_desc; /* PRIV */

#endif


