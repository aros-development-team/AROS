#ifndef DATATYPES_SOUNDCLASS_H
#define DATATYPES_SOUNDCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Includes for soundclass
    Lang: English
*/

#ifndef	UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef	DATATYPES_DATATYPESCLASS_H
#   include <datatypes/datatypesclass.h>
#endif

#ifndef	LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#ifndef	DEVICES_TIMER_H
#   include <devices/timer.h>
#endif

#define	SOUNDDTCLASS		"sound.datatype"

/* Tags */

#define	SDTA_Dummy		(DTA_Dummy + 500)
#define	SDTA_VoiceHeader	(SDTA_Dummy + 1)
#define	SDTA_Sample		(SDTA_Dummy + 2)
#define	SDTA_SampleLength	(SDTA_Dummy + 3)
#define	SDTA_Period		(SDTA_Dummy + 4)
#define	SDTA_Volume		(SDTA_Dummy + 5)
#define	SDTA_Cycles		(SDTA_Dummy + 6)
#define	SDTA_SignalTask		(SDTA_Dummy + 7)
#define	SDTA_SignalBit		(SDTA_Dummy + 8)
#define	SDTA_SignalBitMask	SDTA_SignalBit
#define	SDTA_Continuous		(SDTA_Dummy + 9)

/* New in V44 */

#define	SDTA_SignalBitNumber	(SDTA_Dummy + 10)
#define	SDTA_SamplesPerSec	(SDTA_Dummy + 11)
#define	SDTA_ReplayPeriod	(SDTA_Dummy + 12)
#define	SDTA_LeftSample		(SDTA_Dummy + 13)
#define	SDTA_RightSample	(SDTA_Dummy + 14)
#define	SDTA_Pan		(SDTA_Dummy + 15)
#define	SDTA_FreeSampleData	(SDTA_Dummy + 16)
#define	SDTA_SyncSampleChange	(SDTA_Dummy + 17)


/* Data compression methods */

#define CMP_NONE     	    	0
#define CMP_FIBDELTA 	    	1

/* Unity = Fixed 1.0 = maximum volume */
#define Unity 	    	    	0x10000UL

struct VoiceHeader
{
    ULONG vh_OneShotHiSamples;
    ULONG vh_RepeatHiSamples;
    ULONG vh_SamplesPerHiCycle;
    UWORD vh_SamplesPerSec;
    UBYTE vh_Octaves;
    UBYTE vh_Compression;
    ULONG vh_Volume;
};


/* Channel allocation */

#define SAMPLETYPE_Left		2L
#define SAMPLETYPE_Right	4L
#define SAMPLETYPE_Stereo	6L

typedef LONG SampleType;

/* IFF types */

#define ID_8SVX	    	    	MAKE_ID('8','S','V','X')
#define ID_VHDR	    	    	MAKE_ID('V','H','D','R')
#define ID_CHAN	    	    	MAKE_ID('C','H','A','N')
#define ID_BODY	    	    	MAKE_ID('B','O','D','Y')

#endif  /* DATATYPES_SOUNDCLASS_H */
