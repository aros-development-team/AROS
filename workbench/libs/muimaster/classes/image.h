/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_IMAGE_H
#define _MUI_CLASSES_IMAGE_H

#define MUIC_Image "Image.mui"

/* Image attributes */
#define MUIA_Image_FontMatch        (TAG_USER|0x0042815d) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FontMatchHeight  (TAG_USER|0x00429f26) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FontMatchWidth   (TAG_USER|0x004239bf) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FreeHoriz        (TAG_USER|0x0042da84) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_FreeVert         (TAG_USER|0x0042ea28) /* MUI: V4  i.. BOOL           */
#define MUIA_Image_OldImage         (TAG_USER|0x00424f3d) /* MUI: V4  i.. struct Image * */
#define MUIA_Image_Spec             (TAG_USER|0x004233d5) /* MUI: V4  i.. char *         */
#define MUIA_Image_State            (TAG_USER|0x0042a3ad) /* MUI: V4  is. LONG           */

extern const struct __MUIBuiltinClass _MUI_Image_desc; /* PRIV */

#endif
