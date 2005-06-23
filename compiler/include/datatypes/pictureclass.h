#ifndef DATATYPES_PICTURECLASS_H
#define DATATYPES_PICTURECLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Includes for pictureclass
    Lang: English
*/


#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#define PICTUREDTCLASS          "picture.datatype"

struct BitMapHeader
{
    UWORD        bmh_Width;
    UWORD        bmh_Height;
    WORD         bmh_Left;
    WORD         bmh_Top;
    UBYTE        bmh_Depth;
    UBYTE        bmh_Masking;
    UBYTE        bmh_Compression;
    UBYTE        bmh_Pad;
    UWORD        bmh_Transparent;
    UBYTE        bmh_XAspect;
    UBYTE        bmh_YAspect;
    WORD         bmh_PageWidth;
    WORD         bmh_PageHeight;
};

struct ColorRegister
{
    UBYTE red, green, blue;
};


#define PDTA_ModeID             (DTA_Dummy + 200)
#define PDTA_BitMapHeader       (DTA_Dummy + 201)
#define PDTA_BitMap             (DTA_Dummy + 202)
#define PDTA_ColorRegisters     (DTA_Dummy + 203)
#define PDTA_CRegs              (DTA_Dummy + 204)
#define PDTA_GRegs              (DTA_Dummy + 205)
#define PDTA_ColorTable         (DTA_Dummy + 206)
#define PDTA_ColorTable2        (DTA_Dummy + 207)
#define PDTA_Allocated          (DTA_Dummy + 208)
#define PDTA_NumColors          (DTA_Dummy + 209)
#define PDTA_NumAlloc           (DTA_Dummy + 210)
#define PDTA_Remap              (DTA_Dummy + 211)
#define PDTA_Screen             (DTA_Dummy + 212)
#define PDTA_FreeSourceBitMap   (DTA_Dummy + 213)
#define PDTA_Grab               (DTA_Dummy + 214)
#define PDTA_DestBitMap         (DTA_Dummy + 215)
#define PDTA_ClassBitMap        (DTA_Dummy + 216)
#define PDTA_NumSparse          (DTA_Dummy + 217)
#define PDTA_SparseTable        (DTA_Dummy + 218)

#define PDTA_SourceMode		(DTA_Dummy + 250) /* Set the interface mode for the sub datatype. See below. */
#define PDTA_DestMode		(DTA_Dummy + 251) /* Set the interface mode for the app datatype. See below. */
#define PDTA_UseFriendBitMap	(DTA_Dummy + 255) /* Make the allocated bitmap be a "friend" bitmap (BOOL) */

/* Interface modes */
#define PMODE_V42 (0)	/* Mode used for backward compatibility */
#define PMODE_V43 (1)	/* Use the new features*/


#define mskNone                 0
#define mskHasMask              1
#define mskHasTransparentColor  2
#define mskLasso                3
#define mskHasAlpha             4

#define cmpNone                 0
#define cmpByteRun1             1
#define cmpByteRun2             2


#define ID_ILBM         MAKE_ID('I','L','B','M')
#define ID_BMHD         MAKE_ID('B','M','H','D')
#define ID_BODY         MAKE_ID('B','O','D','Y')
#define ID_CMAP         MAKE_ID('C','M','A','P')
#define ID_CRNG         MAKE_ID('C','R','N','G')
#define ID_GRAB         MAKE_ID('G','R','A','B')
#define ID_SPRT         MAKE_ID('S','P','R','T')
#define ID_DEST         MAKE_ID('D','E','S','T')
#define ID_CAMG         MAKE_ID('C','A','M','G')

/*
 *  Support for the V44 picture.datatype
 *
 *  It is not clear, if AROS should support AmigaOS3.5 .
 *
 *  But if you want V44-support define DT_V44_SUPPORT
 *
 *  Joerg Dietrich
 */
#ifndef DT_V44_SUPPORT
#define DT_V44_SUPPORT 1
#endif

#ifdef DT_V44_SUPPORT

#define PMODE_V42 (0)
#define PMODE_V43 (1)

#define PDTANUMPICTURES_Unknown (0)

#define PDTA_WhichPicture       (DTA_Dummy + 219)
#define PDTA_GetNumPictures     (DTA_Dummy + 220)
#define PDTA_MaxDitherPens      (DTA_Dummy + 221)
#define PDTA_DitherQuality      (DTA_Dummy + 222)
#define PDTA_AllocatedPens      (DTA_Dummy + 223)
#define PDTA_ScaleQuality	(DTA_Dummy + 224)
#define PDTA_DelayRead		(DTA_Dummy + 225)
#define PDTA_DelayedRead	(DTA_Dummy + 226)

#define PDTA_SourceMode         (DTA_Dummy + 250)
#define PDTA_DestMode           (DTA_Dummy + 251)
#define PDTA_UseFriendBitMap    (DTA_Dummy + 255)
#define PDTA_MaskPlane          (DTA_Dummy + 258)

#define PDTM_Dummy              (DTM_Dummy + 0x60)
#define PDTM_WRITEPIXELARRAY    (PDTM_Dummy + 0)
#define PDTM_READPIXELARRAY     (PDTM_Dummy + 1)
#define PDTM_SCALE              (PDTM_Dummy + 2)

struct pdtBlitPixelArray
{
	ULONG   MethodID;
	APTR    pbpa_PixelData;
	ULONG   pbpa_PixelFormat;
	ULONG   pbpa_PixelArrayMod;
	ULONG   pbpa_Left;
	ULONG   pbpa_Top;
	ULONG   pbpa_Width;
	ULONG   pbpa_Height;
};

struct pdtScale
{
	ULONG MethodID;
	ULONG ps_NewWidth;
	ULONG ps_NewHeight;
	ULONG ps_Flags;
};

/* Flags for ps_Flags, for AROS only */
#define PScale_KeepAspect	0x10	/* Keep aspect ratio when scaling, fit inside given x,y coordinates */


#define PBPAFMT_RGB     0
#define PBPAFMT_RGBA    1
#define PBPAFMT_ARGB    2
#define PBPAFMT_LUT8    3
#define PBPAFMT_GREY8   4

#endif /* DT_V44_SUPPORT */

#endif  /* DATATYPES_PICTURECLASS_H */
