#ifndef DATATYPES_ANIMATIONCLASS_H
#define DATATYPES_ANIMATIONCLASS_H

/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Includes for animationclass
    Lang: English
*/

#ifndef	UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef	DATATYPES_DATATYPESCLASS_H
#   include <datatypes/datatypesclass.h>
#endif

#ifndef	DATATYPES_PICTURECLASS_H
#   include <datatypes/pictureclass.h>
#endif

#ifndef	DATATYPES_SOUNDCLASS_H
#   include <datatypes/soundclass.h>
#endif

#ifndef	LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#define	ANIMATIONDTCLASS	"animation.datatype"


/* Tags */
#define	ADTA_Dummy		(DTA_Dummy + 600)
#define	ADTA_ModeID		PDTA_ModeID
#define	ADTA_KeyFrame		PDTA_BitMap
#define	ADTA_ColorRegisters	PDTA_ColorRegisters
#define	ADTA_CRegs		PDTA_CRegs
#define	ADTA_GRegs		PDTA_GRegs
#define	ADTA_ColorTable		PDTA_ColorTable
#define	ADTA_ColorTable2	PDTA_ColorTable2
#define	ADTA_Allocated		PDTA_Allocated
#define	ADTA_NumColors		PDTA_NumColors
#define	ADTA_NumAlloc		PDTA_NumAlloc
#define	ADTA_NumSparse		PDTA_NumSparse
#define	ADTA_SparseTable		PDTA_SparseTable
#define	ADTA_Remap		PDTA_Remap
#define	ADTA_Screen		PDTA_Screen
#define	ADTA_Width		(ADTA_Dummy + 1)
#define	ADTA_Height		(ADTA_Dummy + 2)
#define	ADTA_Depth		(ADTA_Dummy + 3)
#define	ADTA_Frames		(ADTA_Dummy + 4)
#define	ADTA_Frame		(ADTA_Dummy + 5)
#define	ADTA_FramesPerSecond	(ADTA_Dummy + 6)
#define	ADTA_FrameIncrement	(ADTA_Dummy + 7)
#define	ADTA_Sample		SDTA_Sample
#define	ADTA_SampleLength	SDTA_SampleLength
#define	ADTA_Period		SDTA_Period
#define	ADTA_Volume		SDTA_Volume
#define	ADTA_Cycles		SDTA_Cycles

/* New in V44 */
#define	ADTA_PreloadFrameCount	(ADTA_Dummy + 8)
#define	ADTA_LeftSample		SDTA_LeftSample
#define	ADTA_RightSample	SDTA_RightSample
#define	ADTA_SamplesPerSec	SDTA_SamplesPerSec

/* IFF ANIM chunks */

#define ID_ANIM             	MAKE_ID('A','N','I','M')
#define ID_ANHD             	MAKE_ID('A','N','H','D')
#define ID_DLTA             	MAKE_ID('D','L','T','A')
#define ID_SBDY             	MAKE_ID('S','B','D','Y')

struct AnimHeader
{
    UBYTE	 ah_Operation;
    UBYTE	 ah_Mask;
    UWORD	 ah_Width;
    UWORD	 ah_Height;
    WORD	 ah_Left;
    WORD	 ah_Top;
    ULONG	 ah_AbsTime;
    ULONG	 ah_RelTime;
    UBYTE	 ah_Interleave;
    UBYTE	 ah_Pad0;
    ULONG	 ah_Flags;
    UBYTE	 ah_Pad[16];
} __packed;

/* Methods */

#define	ADTM_Dummy		0x700
#define	ADTM_LOADFRAME		0x701
#define	ADTM_UNLOADFRAME	0x702
#define	ADTM_START		0x703
#define	ADTM_PAUSE		0x704
#define	ADTM_STOP		0x705
#define	ADTM_LOCATE		0x706

/* New in V44 */
#define	ADTM_LOADNEWFORMATFRAME	    0x707
#define	ADTM_UNLOADNEWFORMATFRAME   0x708

struct adtFrame
{
    STACKED ULONG	     MethodID;
    STACKED ULONG	     alf_TimeStamp;
    STACKED ULONG	     alf_Frame;
    STACKED ULONG	     alf_Duration;
    STACKED struct BitMap   *alf_BitMap;
    STACKED struct ColorMap *alf_CMap;
    STACKED BYTE	    *alf_Sample;
    STACKED ULONG	     alf_SampleLength;
    STACKED ULONG	     alf_Period;
    STACKED APTR	     alf_UserData;
};

struct adtNewFormatFrame
{
    STACKED ULONG	     MethodID;
    STACKED ULONG	     alf_TimeStamp;
    STACKED ULONG	     alf_Frame;
    STACKED ULONG	     alf_Duration;
    STACKED struct BitMap   *alf_BitMap;
    STACKED struct ColorMap *alf_CMap;
    STACKED BYTE    	    *alf_Sample;
    STACKED ULONG	     alf_SampleLength;
    STACKED ULONG	     alf_Period;
    STACKED APTR	     alf_UserData;
    STACKED ULONG	     alf_Size;
    STACKED BYTE    	    *alf_LeftSample;
    STACKED BYTE    	    *alf_RightSample;
    STACKED ULONG	     alf_SamplesPerSec;
};

struct adtStart
{
    STACKED ULONG	    MethodID;
    STACKED ULONG	    asa_Frame;
};

#endif  /* DATATYPES_ANIMATIONCLASS_H */
