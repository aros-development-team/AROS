/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_BITMAP_H
#define _MUI_CLASSES_BITMAP_H

#define MUIC_Bitmap "Bitmap.mui"

/* Bitmap Attributes */
#define MUIA_Bitmap_Bitmap          (TAG_USER|0x004279bd) /* MUI: V8  isg struct BitMap *   */
#define MUIA_Bitmap_Height          (TAG_USER|0x00421560) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_MappingTable    (TAG_USER|0x0042e23d) /* MUI: V8  isg UBYTE *           */
#define MUIA_Bitmap_Precision       (TAG_USER|0x00420c74) /* MUI: V11 isg LONG              */
#define MUIA_Bitmap_RemappedBitmap  (TAG_USER|0x00423a47) /* MUI: V11 ..g struct BitMap *   */
#define MUIA_Bitmap_SourceColors    (TAG_USER|0x00425360) /* MUI: V8  isg ULONG *           */
#define MUIA_Bitmap_Transparent     (TAG_USER|0x00422805) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_UseFriend       (TAG_USER|0x004239d8) /* MUI: V11 i.. BOOL              */
#define MUIA_Bitmap_Width           (TAG_USER|0x0042eb3a) /* MUI: V8  isg LONG              */

extern const struct __MUIBuiltinClass _MUI_Bitmap_desc; /* PRIV */

#endif
