#ifndef	DATATYPES_PICTURECLASS_H
#define	DATATYPES_PICTURECLASS_H

/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Includes for pictureclass
    Lang: English
*/


#ifndef	UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef	DATATYPES_DATATYPESCLASS_H
#include <datatypes/datatypesclass.h>
#endif

#ifndef	LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#define	PICTUREDTCLASS		"picture.datatype"

struct BitMapHeader
{
    UWORD	 bmh_Width;
    UWORD	 bmh_Height;
    WORD	 bmh_Left;
    WORD	 bmh_Top;
    UBYTE	 bmh_Depth;
    UBYTE	 bmh_Masking;
    UBYTE	 bmh_Compression;
    UBYTE	 bmh_Pad;
    UWORD	 bmh_Transparent;
    UBYTE	 bmh_XAspect;
    UBYTE	 bmh_YAspect;
    WORD	 bmh_PageWidth;
    WORD	 bmh_PageHeight;
};

struct ColorRegister
{
    UBYTE red, green, blue;
};


#define	PDTA_ModeID		(DTA_Dummy + 200)
#define	PDTA_BitMapHeader	(DTA_Dummy + 201)
#define	PDTA_BitMap		(DTA_Dummy + 202)
#define	PDTA_ColorRegisters	(DTA_Dummy + 203)
#define	PDTA_CRegs		(DTA_Dummy + 204)
#define	PDTA_GRegs		(DTA_Dummy + 205)
#define	PDTA_ColorTable		(DTA_Dummy + 206)
#define	PDTA_ColorTable2	(DTA_Dummy + 207)
#define	PDTA_Allocated		(DTA_Dummy + 208)
#define	PDTA_NumColors		(DTA_Dummy + 209)
#define	PDTA_NumAlloc		(DTA_Dummy + 210)
#define	PDTA_Remap		(DTA_Dummy + 211)
#define	PDTA_Screen		(DTA_Dummy + 212)
#define	PDTA_FreeSourceBitMap	(DTA_Dummy + 213)
#define	PDTA_Grab		(DTA_Dummy + 214)
#define	PDTA_DestBitMap		(DTA_Dummy + 215)
#define	PDTA_ClassBitMap	(DTA_Dummy + 216)
#define	PDTA_NumSparse		(DTA_Dummy + 217)
#define	PDTA_SparseTable	(DTA_Dummy + 218)


#define	mskNone			0
#define	mskHasMask		1
#define	mskHasTransparentColor	2
#define	mskLasso		3
#define	mskHasAlpha		4

#define	cmpNone			0
#define	cmpByteRun1		1
#define	cmpByteRun2		2


#define	ID_ILBM		MAKE_ID('I','L','B','M')
#define	ID_BMHD		MAKE_ID('B','M','H','D')
#define	ID_BODY		MAKE_ID('B','O','D','Y')
#define	ID_CMAP		MAKE_ID('C','M','A','P')
#define	ID_CRNG		MAKE_ID('C','R','N','G')
#define	ID_GRAB		MAKE_ID('G','R','A','B')
#define	ID_SPRT		MAKE_ID('S','P','R','T')
#define	ID_DEST		MAKE_ID('D','E','S','T')
#define	ID_CAMG		MAKE_ID('C','A','M','G')

#endif	/* DATATYPES_PICTURECLASS_H */
