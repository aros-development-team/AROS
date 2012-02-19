#ifndef _MUI_CLASSES_PIXMAP_H
#define _MUI_CLASSES_PIXMAP_H

/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Pixmap "Pixmap.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Pixmap                     (MUIB_ZUNE|0x00003600)

/*** Methods ****************************************************************/
#define MUIM_Pixmap_DrawSection         (MUIB_MUI|0x0042ce0f) /* private */ /* V20 */
struct  MUIP_Pixmap_DrawSection          {STACKED ULONG MethodID; STACKED LONG sx; STACKED LONG sy; STACKED LONG sw; STACKED LONG sh; 
                                          STACKED struct MUI_RenderInfo *mri; STACKED LONG dx; STACKED LONG dy; }; /* private */

/*** Attributes *************************************************************/
#define MUIA_Pixmap_Alpha               (MUIB_MUI|0x00421fef) /* V20 isg ULONG             */
#define MUIA_Pixmap_CLUT                (MUIB_MUI|0x0042042a) /* V20 isg ULONG *           */
#define MUIA_Pixmap_CompressedSize      (MUIB_MUI|0x0042e7e4) /* V20 isg ULONG             */
#define MUIA_Pixmap_Compression         (MUIB_MUI|0x0042ce74) /* V20 isg ULONG             */
#define MUIA_Pixmap_Data                (MUIB_MUI|0x00429ea0) /* V20 isg APTR              */
#define MUIA_Pixmap_Format              (MUIB_MUI|0x0042ab14) /* V20 isg ULONG             */
#define MUIA_Pixmap_Height              (MUIB_MUI|0x004288be) /* V20 isg LONG              */
#define MUIA_Pixmap_UncompressedData    (MUIB_MUI|0x0042b085) /* V20 ..g APTR              */
#define MUIA_Pixmap_Width               (MUIB_MUI|0x0042ccb8) /* V20 isg LONG              */

#define MUIV_Pixmap_Compression_None 0
#define MUIV_Pixmap_Compression_RLE 1
#define MUIV_Pixmap_Compression_BZip2 2
#define MUIV_Pixmap_Format_CLUT8 0
#define MUIV_Pixmap_Format_RGB24 1
#define MUIV_Pixmap_Format_ARGB32 2


extern const struct __MUIBuiltinClass _MUI_Pixmap_desc; /* PRIV */

#endif
