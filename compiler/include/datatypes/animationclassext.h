#ifndef DATATYPES_ANIMATIONCLASSEXT_H
#define DATATYPES_ANIMATIONCLASSEXT_H

/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: v41 extensions for animationclass
    Lang: English
*/

#ifndef	DATATYPES_ANIMATIONCLASS_H
#   include <datatypes/animationclass.h>
#endif

#ifndef	LIBRARIES_REALTIME_H
#   include <libraries/realtime.h>
#endif

/* Tags */
#define ADTA_BitMapHeader	PDTA_BitMapHeader
#define ADTA_VoiceHeader	SDTA_VoiceHeader
#define ADTA_Grab		PDTA_Grab
#define ADTA_DestFrame		PDTA_DestBitMap

#define ADTA_CBMReserved0	(ADTA_Dummy +  8)	/* reserved */
#define ADTA_CBMReserved1	(ADTA_Dummy +  9)	/* reserved */
#define ADTA_CBMReserved2	(ADTA_Dummy + 10)	/* reserved */
#define ADTA_CBMReserved3	(ADTA_Dummy + 11)	/* reserved */
#define ADTA_CBMReserved4	(ADTA_Dummy + 12)	/* reserved */
#define ADTA_CBMReserved5	(ADTA_Dummy + 13)	/* reserved */
#define ADTA_CBMReserved6	(ADTA_Dummy + 14)	/* reserved */
#define ADTA_CBMReserved7	(ADTA_Dummy + 15)	/* reserved */
#define ADTA_CBMReserved8	(ADTA_Dummy + 16)	/* reserved */
#define ADTA_CBMReserved9	(ADTA_Dummy + 17)	/* reserved */
#define ADTA_CBMReserved10	(ADTA_Dummy + 18)	/* reserved */
#define ADTA_CBMReserved11	(ADTA_Dummy + 19)	/* reserved */
#define ADTA_CBMReserved12	(ADTA_Dummy + 20)	/* reserved */
#define ADTA_CBMReserved13	(ADTA_Dummy + 21)	/* reserved */
#define ADTA_CBMReserved14	(ADTA_Dummy + 22)	/* reserved */
#define ADTA_CBMReserved15	(ADTA_Dummy + 23)	/* reserved */
#define ADTA_TicksPerFrame	(ADTA_Dummy + 24)
#define ADTA_CurrTicksPerFrame	(ADTA_Dummy + 25)
#define ADTA_CurrFramesPerSecond	(ADTA_Dummy + 26)
#define ADTA_Private0		(ADTA_Dummy + 27)	/* reserved */
#define ADTA_Private1		(ADTA_Dummy + 28)	/* reserved */
#define ADTA_AdaptiveFPS	(ADTA_Dummy + 30)
#define ADTA_SmartSkip		(ADTA_Dummy + 31)
#define ADTA_NumPrefetchFrames	(ADTA_Dummy + 32)
#define ADTA_OvertakeScreen	(ADTA_Dummy + 33)

#endif  /* DATATYPES_ANIMATIONCLASSEXT_H */
