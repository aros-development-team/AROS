/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_BITMAP_H
#define _MUI_CLASSES_BITMAP_H

/****************************************************************************/
/** Bitmap                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Bitmap[];
#else
#define MUIC_Bitmap "Bitmap.mui"
#endif

/* Attributes */

#define MUIA_Bitmap_Bitmap                  0x804279bd /* V8  isg struct BitMap *   */
#define MUIA_Bitmap_Height                  0x80421560 /* V8  isg LONG              */
#define MUIA_Bitmap_MappingTable            0x8042e23d /* V8  isg UBYTE *           */
#define MUIA_Bitmap_Precision               0x80420c74 /* V11 isg LONG              */
#define MUIA_Bitmap_RemappedBitmap          0x80423a47 /* V11 ..g struct BitMap *   */
#define MUIA_Bitmap_SourceColors            0x80425360 /* V8  isg ULONG *           */
#define MUIA_Bitmap_Transparent             0x80422805 /* V8  isg LONG              */
#define MUIA_Bitmap_UseFriend               0x804239d8 /* V11 i.. BOOL              */
#define MUIA_Bitmap_Width                   0x8042eb3a /* V8  isg LONG              */

extern const struct __MUIBuiltinClass _MUI_Bitmap_desc;

#endif
