#ifndef GLOBALS_H
#define GLOBALS_H

#include <dos/bptr.h>

#define DEBUG_FONTBITMAP(x)	x;

#ifdef __AROS__
#define dprintf kprintf
#endif

#ifndef MAKE_ID
#	define MAKE_ID(a,b,c,d)	(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

#define DEBUG_MAIN(x)		x;
#define DEBUG_FONTBITMAP(x)	x;
#define DEBUG_FONTINFO(x)	x;
#define DEBUG_FONTWINDOW(x)	x;
#define DEBUG_ADDDIR(x)		x;

#include <ft2build.h>
#include FT_FREETYPE_H
//#include FT_IMAGE_H
//#include FT_RENDER_H
//#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H

#include <freetype/freetype.h>

#ifndef OT_GlyphMap8Bit
#define OT_GlyphMap8Bit		(OT_Level1 | 0x108)

#define OT_Spec1_FontFile	(OT_Spec1  | OT_Indirect)
#define OT_Spec2_CodePage	(OT_Level1 | OT_Indirect | 0x102)
#define OT_Spec3_AFMFile	(OT_Level1 | OT_Indirect | 0x103)
#define OT_Spec4_Metric		(OT_Level1 | 0x104)
#define OT_Spec5_BBox		(OT_Level1 | 0x105)
#define OT_Spec6_FaceNum	(OT_Level1 | 0x106)			// index for .ttc files
#define OT_Spec7_BMSize		(OT_Level1 | 0x107)			// embbeded bitmap size
//
// Values for OT_Spec4_Metric
#define METRIC_GLOBALBBOX	0	// default
#define METRIC_RAW_EM		1
#define METRIC_ASCEND		2
#define METRIC_TYPOASCEND	3
#define METRIC_USWINASCEND	4
#define METRIC_CUSTOMBBOX	5
#define METRIC_BMSIZE		6
#endif

#define UFHN(func)	((IPTR (*)())&func)

#ifndef __AROS__

#define UFH2(rt, func, p1, p2) \
	rt func(void); \
	struct EmulLibEntry func##Gate = { TRAP_LIB, 0, (void (*)(void)) func }; \
	rt func(void) \
	{ \
		p1; \
		p2;

#define UFH3(rt, func, p1, p2, p3) \
	rt func(void); \
	struct EmulLibEntry func##Gate = { TRAP_LIB, 0, (void (*)(void)) func }; \
	rt func(void) \
	{ \
		p1; \
		p2; \
		p3;

#endif

extern BPTR destdir;
extern UWORD codepage[256];

BOOL IsDefaultCodePage(void);
FT_Library ftlibrary;

#endif
