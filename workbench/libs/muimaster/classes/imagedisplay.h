#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#define _MUI_CLASSES_IMAGEDISPLAY_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Imagedisplay           "Imagedisplay.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Imagedisplay           (MUIB_ZUNE | 0x00001200)

/*** Attributes *************************************************************/
#define MUIA_Imagedisplay_Spec      (MUIB_MUI|0x0042a547) /* MUI: V11 isg struct MUI_ImageSpec * */
#define MUIA_Imagedisplay_FreeHoriz (MUIB_MUI|0x0042c356) /* Zune 20030323 i.. BOOL [TRUE] */
#define MUIA_Imagedisplay_FreeVert  (MUIB_MUI|0x0042c422) /* Zune 20030323 i.. BOOL [TRUE] */


extern const struct __MUIBuiltinClass _MUI_Imagedisplay_desc; /* PRIV */

#endif /* _MUI_CLASSES_IMAGEDISPLAY_H */
