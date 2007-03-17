#ifndef _MUI_CLASSES_IMAGE_H
#define _MUI_CLASSES_IMAGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Image                  "Image.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Image                  (MUIB_ZUNE | 0x00001300)

/*** Attributes *************************************************************/
#define MUIA_Image_FontMatch        (MUIB_MUI|0x0042815d) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FontMatchHeight  (MUIB_MUI|0x00429f26) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FontMatchWidth   (MUIB_MUI|0x004239bf) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FreeHoriz        (MUIB_MUI|0x0042da84) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FreeVert         (MUIB_MUI|0x0042ea28) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_OldImage         (MUIB_MUI|0x00424f3d) /* MUI: V4  i.. struct Image * */
#define MUIA_Image_Spec             (MUIB_MUI|0x004233d5) /* MUI: V4  i.. char *         */
#define MUIA_Image_State            (MUIB_MUI|0x0042a3ad) /* MUI: V4  is. LONG           */


extern const struct __MUIBuiltinClass _MUI_Image_desc; /* PRIV */

#endif /* _MUI_CLASSES_IMAGE_H */
