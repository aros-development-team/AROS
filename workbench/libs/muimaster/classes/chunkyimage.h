#ifndef _MUI_CLASSES_CHUNKYIMAGE_H
#define _MUI_CLASSES_CHUNKYIMAGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_ChunkyImage            "ChunkyImage.mui"

/*** Identifier base ********************************************************/
#define MUIB_ChunkyImage            (MUIB_ZUNE | 0x00004000)

/*** Attributes *************************************************************/
#define MUIA_ChunkyImage_Pixels     (MUIB_ChunkyImage | 0x00000000) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_Palette    (MUIB_ChunkyImage | 0x00000001) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_NumColors  (MUIB_ChunkyImage | 0x00000002) /* V8  isg LONG    */
#define MUIA_ChunkyImage_Modulo     (MUIB_ChunkyImage | 0x00000003) /* V8  isg LONG    */


extern const struct __MUIBuiltinClass _MUI_ChunkyImage_desc; /* PRIV */

#endif /* _MUI_CLASSES_CHUNKYIMAGE_H */
