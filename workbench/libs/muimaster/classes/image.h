/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_IMAGE_H
#define _MUI_CLASSES_IMAGE_H

/****************************************************************************/
/** Image                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Image[];
#else
#define MUIC_Image "Image.mui"
#endif

/* Attributes */

#define MUIA_Image_FontMatch                0x8042815d /* V4  i.. BOOL              */
#define MUIA_Image_FontMatchHeight          0x80429f26 /* V4  i.. BOOL              */
#define MUIA_Image_FontMatchWidth           0x804239bf /* V4  i.. BOOL              */
#define MUIA_Image_FreeHoriz                0x8042da84 /* V4  i.. BOOL              */
#define MUIA_Image_FreeVert                 0x8042ea28 /* V4  i.. BOOL              */
#define MUIA_Image_OldImage                 0x80424f3d /* V4  i.. struct Image *    */
#define MUIA_Image_Spec                     0x804233d5 /* V4  i.. char *            */
#define MUIA_Image_State                    0x8042a3ad /* V4  is. LONG              */

extern const struct __MUIBuiltinClass _MUI_Image_desc;

#endif
