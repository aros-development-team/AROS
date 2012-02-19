#ifndef RAWIMAGE_MCC_H
#define RAWIMAGE_MCC_H

/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Rawimage  "Rawimage.mcc"

/*** Identifier base ********************************************************/

/*** Protected Attributes ***************************************************/
#define MUIA_Rawimage_Data  0xFED10014ul   /* [IS.]  struct MUI_RawimageData *   v20.1 (06.01.2007)   */

/*** Macros *****************************************************************/
#define RawimageObject MUIOBJMACRO_START(MUIC_Rawimage)

#define RAWIMAGE_PIXELFORMAT_ARGB    0
#define RAWIMAGE_PIXELFORMAT_RGB     1

#define RAWIMAGE_FORMAT_RAW_ARGB_ID  RAWIMAGE_PIXELFORMAT_ARGB
#define RAWIMAGE_FORMAT_RAW_RGB_ID   RAWIMAGE_PIXELFORMAT_RGB
#define RAWIMAGE_FORMAT_BZ2_ARGB_ID  MAKE_ID('B', 'Z', '2', RAWIMAGE_PIXELFORMAT_ARGB)
#define RAWIMAGE_FORMAT_BZ2_RGB_ID   MAKE_ID('B', 'Z', '2', RAWIMAGE_PIXELFORMAT_RGB )
#define RAWIMAGE_FORMAT_Z_ARGB_ID    MAKE_ID('Z', 0x0, 0x0, RAWIMAGE_PIXELFORMAT_ARGB)
#define RAWIMAGE_FORMAT_Z_RGB_ID     MAKE_ID('Z', 0x0, 0x0, RAWIMAGE_PIXELFORMAT_RGB )
#define RAWIMAGE_FORMAT_RLE_ARGB_ID  MAKE_ID('R', 'L', 'E', RAWIMAGE_PIXELFORMAT_ARGB)
#define RAWIMAGE_FORMAT_RLE_RGB_ID   MAKE_ID('R', 'L', 'E', RAWIMAGE_PIXELFORMAT_RGB )
#define RAWIMAGE_FORMAT_LZMA_ARGB_ID MAKE_ID('L', 'Z', 'M', RAWIMAGE_PIXELFORMAT_ARGB)
#define RAWIMAGE_FORMAT_LZMA_RGB_ID  MAKE_ID('L', 'Z', 'M', RAWIMAGE_PIXELFORMAT_RGB )

/*** Structs ****************************************************************/
struct MUI_RawimageData
{
    ULONG ri_Width;
    ULONG ri_Height;
    ULONG ri_Format;
    ULONG ri_Size;
    ULONG ri_Data[0];
};

#endif /* RAWIMAGE_MCC_H */
