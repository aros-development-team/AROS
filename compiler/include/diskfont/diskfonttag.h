#ifndef DISKFONT_DISKFONTTAG_H
#define DISKFONT_DISKFONTTAG_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tags for outline font engines and .otag files
    Lang: english
*/

#ifndef	EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef	UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define  OT_Level0	    	TAG_USER
#define  OT_Level1	    	(TAG_USER | 0x1000)
#define  OT_Level2	    	(TAG_USER | 0x2000)
#define  OT_Level3	    	(TAG_USER | 0x3000)
#define  OT_Indirect	    	0x8000

/* Font engine tags */

#define  OT_DeviceDPI	    	(OT_Level0 | 0x01)
#define  OT_DotSize	    	(OT_Level0 | 0x02)
#define  OT_PointHeight	    	(OT_Level0 | 0x08)
#define  OT_SetFactor	    	(OT_Level0 | 0x09)
#define  OT_ShearSin	    	(OT_Level0 | 0x0a)
#define  OT_ShearCos	    	(OT_Level0 | 0x0b)
#define  OT_RotateSin	    	(OT_Level0 | 0x0c)
#define  OT_RotateCos	    	(OT_Level0 | 0x0d)
#define  OT_EmboldenX	    	(OT_Level0 | 0x0e)
#define  OT_EmboldenY	    	(OT_Level0 | 0x0f)
#define  OT_PointSize	    	(OT_Level0 | 0x10)
#define  OT_GlyphCode	    	(OT_Level0 | 0x11)
#define  OT_GlyphCode2	    	(OT_Level0 | 0x12)
#define  OT_GlyphWidth	    	(OT_Level0 | 0x13)
#define  OT_OTagPath	    	(OT_Level0 | OT_Indirect | 0x14)
#define  OT_OTagList	    	(OT_Level0 | OT_Indirect | 0x15)
#define  OT_GlyphMap	    	(OT_Level0 | OT_Indirect | 0x20)
#define  OT_WidthList	    	(OT_Level0 | OT_Indirect | 0x21)
#define  OT_TextKernPair    	(OT_Level0 | OT_Indirect | 0x22)
#define  OT_DesignKernPair  	(OT_Level0 | OT_Indirect | 0x23)
#define  OT_UnderLined		(OT_Level0 | 0x24)
#define  OT_StrikeThrough	(OT_Level0 | 0x25)
#define  OT_GlyphMap8Bits   	(OT_Level0 | OT_Indirect | 0x50)

#define  OTUL_None		0
#define  OTUL_Solid		1
#define  OTUL_Broken		2
#define  OTUL_DoubleSolid	3
#define  OUTL_DoubleBroken	4

/* .otag tags */

#define  OTSUFFIX	    	".otag"
#define  OTE_Bullet	    	"bullet"

#define  OT_FileIdent	    	(OT_Level1 | 0x01)
#define  OT_Engine	    	(OT_Level1 | OT_Indirect | 0x02)
#define  OT_Family	    	(OT_Level1 | OT_Indirect | 0x03)
#define  OT_BName	    	(OT_Level2 | OT_Indirect | 0x05)
#define  OT_IName	    	(OT_Level2 | OT_Indirect | 0x06)
#define  OT_BIName	    	(OT_Level2 | OT_Indirect | 0x07)
#define  OT_SymbolSet	    	(OT_Level1 | 0x10)
#define  OT_YSizeFactor	    	(OT_Level1 | 0x11)
#define  OT_SpaceWidth	    	(OT_Level2 | 0x12)
#define  OT_IsFixed	    	(OT_Level2 | 0x13)
#define  OT_SerifFlag	    	(OT_Level1 | 0x14)
#define  OT_StemWeight	    	(OT_Level1 | 0x15)
#define  OT_SlantStyle	    	(OT_Level1 | 0x16)
#define  OT_HorizStyle	    	(OT_Level1 | 0x17)
#define  OT_SpaceFactor	    	(OT_Level2 | 0x18)
#define  OT_InhibitAlgoStyle 	(OT_Level2 | 0x19)
#define  OT_AvailSizes	    	(OT_Level1 | OT_Indirect | 0x20)

#define  OT_SpecCount	    	(OT_Level1 | 0x100)
#define  OT_Spec	    	(OT_Level1 | 0x100)
#define  OT_Spec1	    	(OT_Level1 | 0x101)

#define  OTS_UltraThin	    	8
#define  OTS_ExtraThin	    	24
#define  OTS_Thin	    	40
#define  OTS_ExtraLight	    	56
#define  OTS_Light	       	72
#define  OTS_DemiLight	    	88
#define  OTS_SemiLight	    	104
#define  OTS_Book	    	120
#define  OTS_Medium	    	136
#define  OTS_SemiBold	    	152
#define  OTS_DemiBold	    	168
#define  OTS_Bold	    	184
#define  OTS_ExtraBold	    	200
#define  OTS_Black	    	216
#define  OTS_ExtraBlack	    	232
#define  OTS_UltraBlack	    	248

#define  OTS_Upright	    	0
#define  OTS_Italic	    	1
#define  OTS_LeftItalic	    	2

#define  OTH_UltraCompressed	16
#define  OTH_ExtraCompressed	48
#define  OTH_Compressed		80
#define  OTH_Condensed		112
#define  OTH_Normal		144
#define  OTH_SemiExpanded	176
#define  OTH_Expanded		208
#define  OTH_ExtraExpanded	240

#define  OT_MAXAVAILSIZES	20

#endif /* DISKFONT_DISKFONTTAG_H */

