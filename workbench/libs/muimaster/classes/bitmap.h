/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BITMAP_H
#define _MUI_CLASSES_BITMAP_H

/*** Name *******************************************************************/
#define MUIC_Bitmap                 "Bitmap.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Bitmap                 (MUIB_ZUNE | 0x00000400)

/*** Attributes *************************************************************/
#define MUIA_Bitmap_Alpha \
    (MUIB_MUI | 0x00423e71) /* MUI: V20 isg ULONG             */
#define MUIA_Bitmap_Bitmap \
    (MUIB_MUI | 0x004279bd) /* MUI: V8  isg struct BitMap *   */
#define MUIA_Bitmap_Height \
    (MUIB_MUI | 0x00421560) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_MappingTable \
    (MUIB_MUI | 0x0042e23d) /* MUI: V8  isg UBYTE *           */
#define MUIA_Bitmap_Precision \
    (MUIB_MUI | 0x00420c74) /* MUI: V11 isg LONG              */
#define MUIA_Bitmap_RemappedBitmap \
    (MUIB_MUI | 0x00423a47) /* MUI: V11 ..g struct BitMap *   */
#define MUIA_Bitmap_SourceColors \
    (MUIB_MUI | 0x00425360) /* MUI: V8  isg ULONG *           */
#define MUIA_Bitmap_Transparent \
    (MUIB_MUI | 0x00422805) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_UseFriend \
    (MUIB_MUI | 0x004239d8) /* MUI: V11 i.. BOOL              */
#define MUIA_Bitmap_Width \
    (MUIB_MUI | 0x0042eb3a) /* MUI: V8  isg LONG              */

extern const struct __MUIBuiltinClass _MUI_Bitmap_desc; /* PRIV */

#endif /* _MUI_CLASSES_BITMAP_H */
