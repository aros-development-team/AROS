/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#define _MUI_CLASSES_IMAGEDISPLAY_H

#define MUIC_Imagedisplay "Imagedisplay.mui"

#define MUIA_Imagedisplay_Spec      (MUIB_MUI|0x0042a547) /* MUI: V11 isg struct MUI_ImageSpec * */
#define MUIA_Imagedisplay_FreeHoriz (MUIB_MUI|0x0042c356) /* Zune 20030323 i.. BOOL [TRUE] */
#define MUIA_Imagedisplay_FreeVert  (MUIB_MUI|0x0042c422) /* Zune 20030323 i.. BOOL [TRUE] */

extern const struct __MUIBuiltinClass _MUI_Imagedisplay_desc; /* PRIV */

#endif
