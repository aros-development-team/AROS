/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#define _MUI_CLASSES_IMAGEDISPLAY_H

#define MUIC_Imagedisplay "Imagedisplay.mui"

#define MUIA_Imagedisplay_Spec      (TAG_USER|0x0042a547) /* MUI: V11 isg struct MUI_ImageSpec * */
#define MUIA_Imagedisplay_FreeHoriz (TAG_USER|0x0042c356) /* Zune 20030323 i.. BOOL [TRUE] */
#define MUIA_Imagedisplay_FreeVert  (TAG_USER|0x0042c422) /* Zune 20030323 i.. BOOL [TRUE] */

extern const struct __MUIBuiltinClass _MUI_Imagedisplay_desc; /* PRIV */

#endif
