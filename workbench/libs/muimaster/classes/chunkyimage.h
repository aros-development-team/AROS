/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_CHUNKYIMAGE_H
#define _MUI_CLASSES_CHUNKYIMAGE_H

#define MUIC_ChunkyImage "ChunkyImage.mui"

/* ChunkyImage attributes */

/* stegerg: CHECKME: uses same tag values as bodychunk class! */ /* PRIV */

#define MUIA_ChunkyImage_Pixels     (TAG_USER|0x0042ca67) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_Palette    (TAG_USER|0x0042de5f) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_NumColors  (TAG_USER|0x0042c392) /* V8  isg LONG    */
#define MUIA_ChunkyImage_Modulo     (TAG_USER|0x00423b0e) /* V8  isg LONG    */

extern const struct __MUIBuiltinClass _MUI_ChunkyImage_desc; /* PRIV */

#endif
