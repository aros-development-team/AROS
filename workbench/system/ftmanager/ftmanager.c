/*
 * Based on source from ftmanager from MorphOS for their ft2.library
 */
#define NO_INLINE_STDARG

#include <stdlib.h>
#include <string.h>
#include <dos/exall.h>
#include <dos/dostags.h>
#include <exec/memory.h>

#define MUI_OBSOLETE
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>
#include <diskfont/oterrors.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/freetype2.h>
#include <aros/debug.h>
#include <aros/macros.h>

#include "etask.h"

#include <ft2build.h>
#include FT_FREETYPE_H
//#include FT_IMAGE_H
//#include FT_RENDER_H
//#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H

#define DEBUG_MAIN(x)		x;
#define DEBUG_FONTBITMAP(x)	x;
#define DEBUG_FONTINFO(x)	x;
#define DEBUG_FONTWINDOW(x)	x;
#define DEBUG_ADDDIR(x)		x;

#ifdef __AROS__
#define dprintf kprintf
#endif

#ifndef MAKE_ID
#	define MAKE_ID(a,b,c,d)	(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

#ifndef OT_GlyphMap8Bit
#define OT_GlyphMap8Bit		(OT_Level1 | 0x108)

#define OT_Spec1_FontFile	(OT_Spec1  | OT_Indirect)
#define OT_Spec2_CodePage       (OT_Level1 | OT_Indirect | 0x102)
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

/***********************************************************************/

Object *app;
FT_Library ftlibrary;
BPTR destdir;
UWORD codepage[256];

BOOL IsDefaultCodePage(void);

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

struct FontBitmapData
{
	int Width, Height;
	struct BitMap BitMap;
	struct BitMap *GrayBitMap;
};
typedef struct FontBitmapData FontBitmapData;

#define FONTBITMAP_BASE			TAG_USER
#define MUIA_FontBitmap_Filename	(FONTBITMAP_BASE + 1)
#define MUIA_FontBitmap_OTags		(FONTBITMAP_BASE + 2)
#define MUIA_FontBitmap_Size		(FONTBITMAP_BASE + 3)
#define MUIA_FontBitmap_String		(FONTBITMAP_BASE + 4)
#define MUIA_FontBitmap_Gray		(FONTBITMAP_BASE + 5)

struct MUI_CustomClass *FontBitmapClass;

#define FontBitmapObject	NewObject(FontBitmapClass->mcc_Class, NULL //)


ULONG fbNew(Class *cl, Object *o, struct opSet *msg)
{
	struct opSet method;
	struct TagItem tags[5];
	STRPTR filename = (STRPTR)GetTagData(MUIA_FontBitmap_Filename, (IPTR) NULL, msg->ops_AttrList);
	STRPTR string = (STRPTR)GetTagData(MUIA_FontBitmap_String, (ULONG) "?", msg->ops_AttrList);
	struct TagItem *otags = (struct TagItem *)GetTagData(MUIA_FontBitmap_OTags,
			(IPTR) NULL, msg->ops_AttrList);
	struct
	{
		struct GlyphMap *glyph;
		int x;
		int y;
	} *info;
	APTR engine;
	struct BitMap bitmap;
	struct BitMap *gray_bitmap = NULL;
	int width, height;
	int length = strlen(string);
	int x, y, k;
	int xmin, xmax, ymin, ymax;
	int space_width, size, gray;
	int previous;
	Tag tag;

	if (filename == NULL)
	{
		DEBUG_FONTBITMAP(dprintf("FontBitmap: no filename.\n"));
		return 0;
	}

	engine = OpenEngine();
	if (engine == NULL)
	{
		DEBUG_FONTBITMAP(dprintf("FontBitmap: no engine.\n"));
		return 0;
	}

	size = GetTagData(MUIA_FontBitmap_Size, 30, msg->ops_AttrList);
	gray = GetTagData(MUIA_FontBitmap_Gray, FALSE, msg->ops_AttrList);

	SetInfo(engine,
			OT_OTagList, (ULONG) otags,
			OT_DeviceDPI, 72 | (72 << 16),
			OT_PointHeight, size << 16,
			TAG_END);

	space_width = (int)(GetTagData(OT_SpaceWidth, 0, otags) / 65536.0 * size) ;

	info = AllocVec(length * sizeof(*info), MEMF_CLEAR);
	if (info == NULL)
	{
		DEBUG_FONTBITMAP(dprintf("FontBitmap: can't alloc glyphs.\n"));
		length = 0;
	}

	x = 0;
	y = 0;
	previous = 0;
	xmin = ymin = 0x7fffffff;
	xmax = ymax = -0x80000000;
	tag = gray ? OT_GlyphMap8Bit : OT_GlyphMap;

	for (k = 0; k < length; ++k)
	{
		int code = string[k];
		int x1, y1, x2, y2;
		struct GlyphMap *g;

		if (previous)
		{
			ULONG kerning;

			SetInfo(engine,
					OT_GlyphCode, previous,
					OT_GlyphCode2, code,
					TAG_END);
			ObtainInfo(engine,
					OT_TextKernPair, (ULONG)&kerning,
					TAG_END);

			x -= (int)(kerning / 65536.0 * size);
		}

		info[k].x = x;
		info[k].y = y;

		SetInfo(engine,
				OT_GlyphCode, code,
				TAG_END);
		ObtainInfo(engine,
				tag, (ULONG)&info[k].glyph,
				TAG_END);

		g = info[k].glyph;

		if (!g)
		{
			x += space_width;
			continue;
		}

		x1 = x - g->glm_X0 + g->glm_BlackLeft;
		y1 = y - g->glm_Y0 + g->glm_BlackTop;
		x2 = x1 + g->glm_BlackWidth;
		y2 = y1 + g->glm_BlackHeight;

		if (x1 < xmin)
			xmin = x1;
		if (y1 < ymin)
			ymin = y1;
		if (x2 > xmax)
			xmax = x2;
		if (y2 > ymax)
			ymax = y2;

		x += g->glm_X1 - g->glm_X0;
		y += g->glm_Y1 - g->glm_Y0;

		previous = code;
	}

	width = xmax - xmin + 1;
	height = ymax - ymin + 1;

	DEBUG_FONTBITMAP(dprintf("FontBitmap: bbox %d %d %d %d\n", xmin, ymin, xmax, ymax));
	DEBUG_FONTBITMAP(dprintf("FontBitmap: width %d height %d\n", width, height));

	if (width > 0 && height > 0 && width < 32000 && height < 32000)
	{
		if (gray)
		{
			UBYTE *array;
			int width1 = (width + 15) & ~15;

			array = AllocVec(width1 * height, MEMF_CLEAR);
			if (array)
			{
				for (k = 0; k < length; ++k)
				{
					struct GlyphMap *g = info[k].glyph;
					int x1, x2, y1, y2;
					UBYTE *p;

					if (!g)
					{
						x += space_width;
						continue;
					}

					x = info[k].x - xmin;
					y = info[k].y - ymin;
					x -= g->glm_X0;
					y -= g->glm_Y0;
					x += g->glm_BlackLeft;
					y += g->glm_BlackTop;

					p = g->glm_BitMap;
					x1 = x;
					y1 = y;
					x2 = x + g->glm_BlackWidth;
					y2 = y + g->glm_BlackHeight;

					if (x1 > width || x2 < 0 || y1 > height || y2 < 0)
						continue;

					if (x1 < 0)
					{
						p -= x1;
						x1 = 0;
					}
					if (y1 < 0)
					{
						p -= y1 * g->glm_BMModulo;
						y1 = 0;
					}
					if (x2 > width)
					{
						x2 = width;
					}
					if (y2 > height)
					{
						y2 = height;
					}

					while (y1 < y2)
					{
						int x;

						for (x = x1; x < x2; ++x)
						{
							int t = array[width1 * y1 + x] + p[x - x1];
							if (t > 255)
								t = 255;
							array[width1 * y1 + x] = t;
						}
						p += g->glm_BMModulo;
						++y1;
					}
				}

				gray_bitmap = AllocBitMap(width, height, 8, 0, NULL);
				if (gray_bitmap)
				{
					struct RastPort rp, tmp_rp;

					InitRastPort(&rp);
					InitRastPort(&tmp_rp);

					rp.BitMap = gray_bitmap;
					tmp_rp.BitMap = AllocBitMap(width, 1, 8, 0, NULL);

					if (tmp_rp.BitMap)
					{
						WritePixelArray8(&rp,
								0,
								0,
								width - 1,
								height - 1,
								array,
								&tmp_rp);
						FreeBitMap(tmp_rp.BitMap);
					}
				}

				FreeVec(array);
			}
		}
		else
		{
			InitBitMap(&bitmap, 1, width, height);
			bitmap.Planes[0] = AllocRaster(width, height);

			if (bitmap.Planes[0])
			{
				struct RastPort rp;

				InitRastPort(&rp);
				rp.BitMap = &bitmap;
				SetRast(&rp, 0);
				SetAPen(&rp, 1);
				SetDrMd(&rp, JAM1);

				for (k = 0; k < length; ++k)
				{
					struct GlyphMap *g = info[k].glyph;

					if (!g)
						continue;

					x = info[k].x - xmin;
					y = info[k].y - ymin;
					x -= g->glm_X0;
					y -= g->glm_Y0;
					x += g->glm_BlackLeft;
					y += g->glm_BlackTop;

					/* glm_BitMap is not in chip mem.
					 * Oh well.
					 */
					BltTemplate((const PLANEPTR)(g->glm_BitMap +
								g->glm_BMModulo *
								g->glm_BlackTop),
							g->glm_BlackLeft,
							g->glm_BMModulo,
							&rp,
							x, y,
							g->glm_BlackWidth,
							g->glm_BlackHeight);
				}
			}
		}

		tags[0].ti_Tag = MUIA_Bitmap_Width;
		tags[0].ti_Data = width;
		tags[1].ti_Tag = MUIA_Bitmap_Height;
		tags[1].ti_Data = height;
		tags[2].ti_Tag = MUIA_FixWidth;
		tags[2].ti_Data = width;
		tags[3].ti_Tag = MUIA_FixHeight;
		tags[3].ti_Data = height;
		tags[4].ti_Tag = TAG_MORE;
		tags[4].ti_Data = (ULONG)msg->ops_AttrList;

		method.MethodID = OM_NEW;
		method.ops_AttrList = tags;
		method.ops_GInfo = NULL;

		o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);

		if (o)
		{
			FontBitmapData *dat = INST_DATA(cl, o);

			dat->Width = width;
			dat->Height = height;
			dat->GrayBitMap = gray_bitmap;

			if (gray)
			{
				static ULONG colors[256 * 3];
				static BOOL init;

				if (!init)
				{
					int k;
					ULONG *p = colors;
					for (k = 256; --k >= 0; p += 3)
					{
						p[0] = p[1] = p[2] = k * 0x01010101;
					}
					init = TRUE;
				}

				SetAttrs(o,
						MUIA_Bitmap_Bitmap, gray_bitmap,
						MUIA_Bitmap_SourceColors, colors,
						TAG_END);
			}
			else
			{
				dat->BitMap = bitmap;
				set(o, MUIA_Bitmap_Bitmap, &dat->BitMap);
			}
		}
		else
		{
			if (gray)
			{
				FreeBitMap(gray_bitmap);
			}
			else if (bitmap.Planes[0])
			{
				FreeRaster(bitmap.Planes[0], width, height);
			}
		}
	}
	else
	{
		o = NULL;
	}

	for (k = 0; k < length; ++k)
	{
		if (info[k].glyph)
		{
			ReleaseInfo(engine,
					tag, (ULONG)info[k].glyph,
					TAG_END);
		}
	}

	FreeVec(info);

	CloseEngine(engine);

	DEBUG_FONTBITMAP(dprintf("FontBitmap: created object 0x%lx.\n", o));

	return (ULONG)o;
}

ULONG fbDispose(Class *cl, Object *o)
{
	FontBitmapData *dat = INST_DATA(cl, o);

	DEBUG_FONTBITMAP(dprintf("FontBitmap: destroy object 0x%lx.\n", o));

	if (dat->GrayBitMap)
	{
		FreeBitMap(dat->GrayBitMap);
	}
	else if (dat->BitMap.Planes[0])
	{
		FreeRaster(dat->BitMap.Planes[0], dat->Width, dat->Height);
	}

	return DoSuperMethod(cl, o, OM_DISPOSE);
}

AROS_UFH3(ULONG, FontBitmapDispatch,
		AROS_UFHA(Class *, cl, A0),
		AROS_UFHA(Object *, o, A2),
		AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG ret;

	switch (msg->MethodID)
	{
		case OM_NEW:
			ret = fbNew(cl, o, (struct opSet *)msg);
			break;

		case OM_DISPOSE:
			ret = fbDispose(cl, o);
			break;

		default:
			ret = DoSuperMethodA(cl, o, msg);
			break;
	}

	return ret;
	
	AROS_USERFUNC_EXIT
}


void CleanupFontBitmapClass(void)
{
	if (FontBitmapClass)
	{
		MUI_DeleteCustomClass(FontBitmapClass);
		FontBitmapClass = NULL;
	}
}

int InitFontBitmapClass(void)
{
	FontBitmapClass = MUI_CreateCustomClass(NULL, MUIC_Bitmap, NULL,
			sizeof(FontBitmapData), UFHN(FontBitmapDispatch));
	return FontBitmapClass != NULL;
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

struct FontInfoData
{
	STRPTR Filename;
	FT_Face Face;
	Object *AttachedFile;
	Object *Name;
	Object *YSizeFactorHigh;
	Object *YSizeFactorLow;
	Object *StemWeight;
	Object *SlantStyle;
	Object *HorizStyle;
	Object *Family;
	Object *Fixed;
	Object *Serif;
	//Object *AlgoStyle;
	Object *FaceNum;
	Object *Metric;
	Object *BBoxYMin;
	Object *BBoxYMax;
	Object *SpaceWidth;
	Object *Preview;
	Object *PreviewGroup;
	Object *Gray;
	Object *TestSize;
	Object *TestString;
	struct TagItem OTags[26];
	UWORD AvailSizes[OT_MAXAVAILSIZES];
};
typedef struct FontInfoData FontInfoData;

#define FONTINFO_BASE			TAG_USER
#define MUIA_FontInfo_Filename		(FONTINFO_BASE + 1)
#define MUIA_FontInfo_Face		(FONTINFO_BASE + 2)

#define FONTINFO_MBASE			TAG_USER
#define MUIM_FontInfo_UpdatePreview	(FONTINFO_MBASE + 1)
#define MUIM_FontInfo_SetOTags		(FONTINFO_MBASE + 2)
#define MUIM_FontInfo_WriteFiles	(FONTINFO_MBASE + 3)

struct MUI_CustomClass *FontInfoClass;

#define FontInfoObject	NewObject(FontInfoClass->mcc_Class, NULL //)

struct CycleToStringP
{
	Object *String;
	int NumValues;
	const UBYTE *Values;
};

AROS_UFH3(void, CycleToString,
		AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(Object *, cycle, A2),
		AROS_UFHA(struct CycleToStringP *, p, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG entry;
	Object *str = p->String;
	get(cycle, MUIA_Cycle_Active, &entry);
	if (entry == p->NumValues)
	{
		set(str, MUIA_Disabled, FALSE);
	}
	else
	{
		SetAttrs(str,
				MUIA_Disabled, TRUE,
				MUIA_String_Integer, p->Values[entry],
				TAG_END);
	}
	
	AROS_USERFUNC_EXIT
}


struct Hook CycleToStringHook = { {NULL, NULL}, UFHN(CycleToString) };


struct IntegerBoundsP
{
	LONG Min;
	LONG Max;
};

AROS_UFH3(void, IntegerBounds,
		AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(Object *, obj, A2),
		AROS_UFHA(struct IntegerBoundsP *, p, A1))
{
	AROS_USERFUNC_INIT

	LONG t;
	get(obj, MUIA_String_Integer, &t);
	if (t < p->Min)
	{
		set(obj, MUIA_String_Integer, p->Min);
	}
	else if (t > p->Max)
	{
		set(obj, MUIA_String_Integer, p->Max);
	}
	
	AROS_USERFUNC_EXIT
}


struct Hook IntegerBoundsHook = { {NULL, NULL}, UFHN(IntegerBounds) };


AROS_UFH3(void, Metric,
		AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(Object *, obj, A2),
		AROS_UFHA(Object **, p, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG t;
	ULONG state;
	int k;

	get(obj, MUIA_Cycle_Active, &t);
	state = t != 5;
	for (k = 0; k < 2; ++k)
	{
		set(p[k], MUIA_Disabled, state);
	}
	
	AROS_USERFUNC_EXIT
}


struct Hook MetricHook = { {NULL, NULL}, UFHN(Metric) };


ULONG fiNew(Class *cl, Object *o, struct opSet *msg)
{
	struct opSet method;
	struct TagItem tags[3];
	STRPTR filename = (STRPTR)GetTagData(MUIA_FontInfo_Filename, (IPTR) NULL, msg->ops_AttrList);
	FT_Face face = (FT_Face)GetTagData(MUIA_FontInfo_Face, (IPTR) NULL, msg->ops_AttrList);
	TT_Postscript *postscript;
	TT_OS2 *os2;
	Object *name, *attached_file, *size_factor_low, *size_factor_high;
	Object *stem_weight, *slant_style, *horiz_style, *stem_weight_cycle, *horiz_style_cycle;
	Object *family, *fixed, /**algo_style,*/ *face_num, *preview, *preview_group;
	Object *gray, *test_string, *test_size, *serif, *space_width;
	Object *metric, *bbox_ymin, *bbox_ymax;
	char name_buf[27];
	int k, l;
	const char *q, *r;

	static const char * const stem_weight_names[] =
	{
		"UltraThin",
		"ExtraThin",
		"Thin",
		"ExtraLight",
		"Light",
		"DemiLight",
		"SemiLight",
		"Book",
		"Medium",
		"SemiBold",
		"DemiBold",
		"Bold",
		"ExtraBold",
		"Black",
		"ExtraBlack",
		"UltraBlack",
		"Custom",
		NULL
	};
	static const UBYTE stem_weight_values[] =
	{
		OTS_UltraThin,
		OTS_ExtraThin,
		OTS_Thin,
		OTS_ExtraLight,
		OTS_Light,
		OTS_DemiLight,
		OTS_SemiLight,
		OTS_Book,
		OTS_Medium,
		OTS_SemiBold,
		OTS_DemiBold,
		OTS_Bold,
		OTS_ExtraBold,
		OTS_Black,
		OTS_ExtraBlack,
		OTS_UltraBlack,
	};

	static const char * const slant_style_names[] =
	{
		"Upright",
		"Italic",
		"LeftItalic",
		NULL
	};

	static const char * const horiz_style_names[] =
	{
		"UltraCompressed",
		"ExtraCompressed",
		"Compressed",
		"Condensed",
		"Normal",
		"SemiExpanded",
		"Expanded",
		"ExtraExpanded",
		"Custom",
		NULL
	};
	static const UBYTE horiz_style_values[] =
	{
		OTH_UltraCompressed,
		OTH_ExtraCompressed,
		OTH_Compressed,
		OTH_Condensed,
		OTH_Normal,
		OTH_SemiExpanded,
		OTH_Expanded,
		OTH_ExtraExpanded,
	};

	static const char *metric_names[] =
	{
		"Global bounding box",
		"Raw font metric",
		"Ascender",
		"Typo ascender",
		"USWin ascender",
		"Custom bounding box",
		//"Bitmap size",
		NULL,
	};

	if (!filename || !face)
	{
		DEBUG_FONTINFO(dprintf("FontInfo: filename 0x%x face 0x%x\n", filename, face));
		return 0;
	}

	q = face->family_name;
	r = face->style_name;
	k = 0;
	l = -1;
	while (k < sizeof(name_buf) - 1)
	{
		while (*q == ' ')
			++q;
		if (!*q)
		{
			if (r)
			{
				q = r;
				r = NULL;
				continue;
			}
			break;
		}
		if (*q == '.')
			l = k;
		name_buf[k] = ToLower(*q);
		++k;
		++q;
	}
	if (l > 0)
		k = l;
	name_buf[k] = '\0';

	postscript = FT_Get_Sfnt_Table(face, ft_sfnt_post);
	os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);

	tags[0].ti_Tag = MUIA_Group_Child;
	tags[0].ti_Data = (ULONG)ColGroup(2),
		Child, Label2("Extra file"),
		Child, attached_file = PopaslObject,
			MUIA_Popasl_Type, ASL_FileRequest,
			MUIA_Popstring_String, StringObject,
				StringFrame,
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_CycleChain, TRUE,
				End,
			MUIA_Popstring_Button, PopButton(MUII_PopFile),
			ASLFR_RejectIcons, TRUE,
			End,
		Child, Label2("Face num"),
		Child, face_num = StringObject,
			StringFrame,
			MUIA_String_Integer, 0,
			MUIA_String_Accept, "0123456789",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 5,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2("Name"),
		Child, name = StringObject,
			StringFrame,
			MUIA_String_Contents, name_buf,
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 27,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2("Family"),
		Child, family = StringObject,
			StringFrame,
			MUIA_String_Contents, face->family_name,
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2("Metric"),
		Child, metric = CycleObject,
			MUIA_Cycle_Entries, metric_names,
			MUIA_Cycle_Active, 1,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2("Bounding box"),
		Child, HGroup,
			Child, Label2("yMin"),
			Child, bbox_ymin = StringObject,
				StringFrame,
				MUIA_String_Accept, "-0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 6,
				MUIA_String_Integer, face->bbox.yMin,
				MUIA_CycleChain, TRUE,
				End,
			Child, Label2("yMax"),
			Child, bbox_ymax = StringObject,
				StringFrame,
				MUIA_String_Accept, "-0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 6,
				MUIA_String_Integer, face->bbox.yMax,
				MUIA_CycleChain, TRUE,
				End,
			End,
		Child, Label2("Size factor"),
		Child, HGroup,
			Child, size_factor_low = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 6,
				MUIA_String_Integer, 1,
				MUIA_CycleChain, TRUE,
				End,
			Child, TextObject,
				MUIA_Text_Contents, "/",
				MUIA_Text_SetMax, TRUE,
				End,
			Child, size_factor_high = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 6,
				MUIA_String_Integer, 1,
				MUIA_CycleChain, TRUE,
				End,
			End,
		Child, Label2("Space width"),
		Child, space_width = StringObject,
			StringFrame,
			MUIA_String_Accept, "0123456789",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 6,
			MUIA_String_Integer, face->max_advance_width * 250.0 / 72.307,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label1("Fixed width"),
		Child, HGroup,
			Child, fixed = CheckMark((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0),
			Child, RectangleObject,
				End,
			End,
		Child, Label1("Serif"),
		Child, HGroup,
			Child, serif = CheckMark(os2 && (unsigned)(((os2->sFamilyClass >> 8) &
							0xff) - 1) < 5),
			Child, RectangleObject,
				End,
			End,
		Child, Label2("Stem weight"),
		Child, HGroup,
			Child, stem_weight_cycle = CycleObject,
				MUIA_Cycle_Entries, stem_weight_names,
				MUIA_CycleChain, TRUE,
				End,
			Child, stem_weight = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 4,
				MUIA_String_Integer, 128,
				MUIA_CycleChain, TRUE,
				End,
			End,
		Child, Label2("Slant style"),
		Child, slant_style = CycleObject,
			MUIA_Cycle_Entries, slant_style_names,
			MUIA_Cycle_Active, face->style_flags & FT_STYLE_FLAG_ITALIC ?
				(postscript && postscript->italicAngle > 0 ?
				 OTS_LeftItalic : OTS_Italic) : OTS_Upright,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2("Horiz style"),
		Child, HGroup,
			Child, horiz_style_cycle = CycleObject,
				MUIA_Cycle_Entries, horiz_style_names,
				MUIA_CycleChain, TRUE,
				End,
			Child, horiz_style = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_MaxLen, 4,
				MUIA_String_Integer, 128,
				MUIA_CycleChain, TRUE,
				End,
			End,
		/*Child, Label1("No algo style"),
		Child, HGroup,
			Child, algo_style = CheckMark(FALSE),
			Child, RectangleObject,
				End,
			End,*/
		End;
	tags[1].ti_Tag = MUIA_Group_Child;
	tags[1].ti_Data = (ULONG)VGroup,
		GroupFrameT("Preview"),
		Child, test_string = StringObject,
			StringFrame,
			MUIA_String_Contents, "The quick brown fox jumped over the lazy dog.",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_CycleChain, TRUE,
			End,
		Child, HGroup,
			Child, Label2("Size"),
			Child, test_size = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_Integer, 30,
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_CycleChain, TRUE,
				End,
			Child, Label2("Anti-aliasing"),
			Child, gray = CheckMark(FALSE),
			End,
		Child, ScrollgroupObject,
			MUIA_Scrollgroup_Contents, VirtgroupObject,
				VirtualFrame,
				Child, VCenter(HCenter((preview_group = VGroup,
					Child, preview = RectangleObject,
						End,
					End))),
				End,
			End,
		End;
	tags[2].ti_Tag = TAG_MORE;
	tags[2].ti_Data = (ULONG)msg->ops_AttrList;

	method.MethodID = OM_NEW;
	method.ops_AttrList = tags;
	method.ops_GInfo = NULL;

	o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);
	if (o)
	{
		FontInfoData *dat = INST_DATA(cl, o);

		dat->Filename = filename;
		dat->Face = face;
		dat->AttachedFile = attached_file;
		dat->Name = name;
		dat->Family = family;
		dat->YSizeFactorLow = size_factor_low;
		dat->YSizeFactorHigh = size_factor_high;
		dat->StemWeight = stem_weight;
		dat->SlantStyle = slant_style;
		dat->HorizStyle = horiz_style;
		dat->SpaceWidth = space_width;
		dat->Fixed = fixed;
		dat->Serif = serif;
		//dat->AlgoStyle = algo_style;
		dat->FaceNum = face_num;
		dat->Metric = metric;
		dat->BBoxYMin = bbox_ymin;
		dat->BBoxYMax = bbox_ymax;
		dat->Preview = preview;
		dat->PreviewGroup = preview_group;
		dat->TestString = test_string;
		dat->TestSize = test_size;
		dat->Gray = gray;

		DoMethod(size_factor_low, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				size_factor_low, 4, MUIM_CallHook, &IntegerBoundsHook, 1, 65535);
		DoMethod(size_factor_high, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				size_factor_high, 4, MUIM_CallHook, &IntegerBoundsHook, 1, 65535);

		DoMethod(stem_weight_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
				stem_weight_cycle, 5, MUIM_CallHook, &CycleToStringHook,
				stem_weight, sizeof(stem_weight_values), stem_weight_values);
		DoMethod(stem_weight, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				stem_weight, 4, MUIM_CallHook, &IntegerBoundsHook, 0, 255);

		DoMethod(horiz_style_cycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
				horiz_style_cycle, 5, MUIM_CallHook, &CycleToStringHook,
				horiz_style, sizeof(horiz_style_values), horiz_style_values);
		DoMethod(horiz_style, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				horiz_style, 4, MUIM_CallHook, &IntegerBoundsHook, 0, 255);

		set(stem_weight_cycle, MUIA_Cycle_Active,
				face->style_flags & FT_STYLE_FLAG_BOLD ? 11 : 7);
		set(horiz_style_cycle, MUIA_Cycle_Active, 4);

		DoMethod(test_string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				o, 1, MUIM_FontInfo_UpdatePreview);
		DoMethod(test_size, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				o, 1, MUIM_FontInfo_UpdatePreview);
		DoMethod(gray, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
				o, 1, MUIM_FontInfo_UpdatePreview);

		DoMethod(bbox_ymin, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				bbox_ymin, 4, MUIM_CallHook, &IntegerBoundsHook, -32768, 32767);
		DoMethod(bbox_ymax, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				bbox_ymax, 4, MUIM_CallHook, &IntegerBoundsHook, -32768, 32767);

		DoMethod(metric, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
				metric, 6, MUIM_CallHook, &MetricHook, bbox_ymin, bbox_ymax);

		set(metric, MUIA_Cycle_Active, 0);

		if (//!(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) &&
			FT_Set_Char_Size(face, 250 * 64, 250 * 64, 2540, 2540) == 0 &&
			FT_Load_Char(face, ' ', FT_LOAD_DEFAULT) == 0)
		{
			set(space_width, MUIA_String_Integer, face->glyph->metrics.horiAdvance >> 6);
		}

		if (os2)
		{
			int weight = os2->usWeightClass;
			int value;

			value = 0; /* UltraThin */
			if (weight < 10) weight *= 100;
			if (weight >= 200) value = 2; /* Thin */
			if (weight >= 300) value = 4; /* Light */
			if (weight >= 400) value = 7; /* Book */
			if (weight >= 500) value = 8; /* Medium */
			if (weight >= 600) value = 10; /* DemiBold */
			if (weight >= 700) value = 11; /* Bold */
			if (weight >= 800) value = 13; /* Black */
			if (weight >= 900) value = 15; /* UltraBlack */
			set(stem_weight_cycle, MUIA_Cycle_Active, value);

			set(horiz_style_cycle, MUIA_Cycle_Active, os2->usWidthClass - 1);
		}

		DoMethod(o, MUIM_FontInfo_UpdatePreview);
	}

	DEBUG_FONTINFO(dprintf("FontInfo: created object 0x%lx\n", o));

	return (ULONG)o;
}

#if 0
ULONG fiDispose(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);

	return DoSuperMethod(cl, o, OM_DISPOSE);
}
#endif

ULONG fiSetOTags(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);
	struct TagItem *tag = dat->OTags;
	ULONG x, y;

	tag->ti_Tag = OT_FileIdent;
	++tag;

	tag->ti_Tag = OT_Engine;
	tag->ti_Data = (ULONG)"freetype2";
	++tag;

	tag->ti_Tag = OT_Family;
	get(dat->Family, MUIA_String_Contents, &tag->ti_Data);
	++tag;

	//tag->ti_Tag = OT_BName;
	//tag->ti_Tag = OT_IName;
	//tag->ti_Tag = OT_BIName;
	//tag->ti_Tag = OT_SymbolSet; charmap index

	get(dat->YSizeFactorLow, MUIA_String_Integer, &x);
	get(dat->YSizeFactorHigh, MUIA_String_Integer, &y);
	x = (UWORD)x;
	y = (UWORD)y;
	if (x == 0) x = 1;
	if (y == 0) y = 1;
	tag->ti_Tag = OT_YSizeFactor;
	tag->ti_Data = x | (y << 16);
	++tag;

	tag->ti_Tag = OT_SpaceWidth;
	get(dat->SpaceWidth, MUIA_String_Integer, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_IsFixed;
	get(dat->Fixed, MUIA_Selected, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_SerifFlag;
	get(dat->Serif, MUIA_Selected, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_StemWeight;
	get(dat->StemWeight, MUIA_String_Integer, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_SlantStyle;
	get(dat->SlantStyle, MUIA_Cycle_Active, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_HorizStyle;
	get(dat->HorizStyle, MUIA_String_Integer, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_SpaceFactor;
	tag->ti_Data = 0x10000;
	++tag;

	tag->ti_Tag = OT_InhibitAlgoStyle;
	tag->ti_Data = FSF_UNDERLINED | FSF_BOLD;
	//get(dat->AlgoStyle, MUIA_Selected, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_SpecCount;
	tag->ti_Data = 4;
	++tag;

	tag->ti_Tag = OT_Spec1_FontFile;
	tag->ti_Data = (ULONG)dat->Filename;
	++tag;

	tag->ti_Tag = OT_Spec3_AFMFile;
	get(dat->AttachedFile, MUIA_String_Contents, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_Spec4_Metric;
	get(dat->Metric, MUIA_Cycle_Active, &tag->ti_Data);
	++tag;

	if (tag[-1].ti_Data == METRIC_CUSTOMBBOX)
	{
		ULONG ymin, ymax;
		get(dat->BBoxYMin, MUIA_String_Integer, &ymin);
		get(dat->BBoxYMax, MUIA_String_Integer, &ymax);
		tag->ti_Tag = OT_Spec5_BBox;
		tag->ti_Data = (ymax << 16) | (UWORD)ymin;
		++tag;
	}

	tag->ti_Tag = OT_Spec6_FaceNum;
	get(dat->FaceNum, MUIA_String_Integer, &tag->ti_Data);
	++tag;

	if (!IsDefaultCodePage())
	{
		tag->ti_Tag = OT_Spec2_CodePage;
		tag->ti_Data = (ULONG)codepage;
		++tag;
	}

	tag->ti_Tag = TAG_END;

	dat->AvailSizes[0] = AROS_WORD2BE(2);   // <- number of entries...
	dat->AvailSizes[1] = AROS_WORD2BE(10);
	dat->AvailSizes[2] = AROS_WORD2BE(15);

	return tag - dat->OTags;
}

ULONG fiUpdatePreview(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);
	Object *preview;
	STRPTR str;
	ULONG gray;
	ULONG size;

	fiSetOTags(cl, o);

	get(dat->TestString, MUIA_String_Contents, &str);
	get(dat->Gray, MUIA_Selected, &gray);
	get(dat->TestSize, MUIA_String_Integer, &size);

	preview = FontBitmapObject,
		MUIA_FontBitmap_Filename, dat->Filename,
		MUIA_FontBitmap_OTags, dat->OTags,
		MUIA_FontBitmap_String, str,
		MUIA_FontBitmap_Gray, gray,
		MUIA_FontBitmap_Size, size,
		End;

	DEBUG_FONTINFO(dprintf("FontInfo::UpdatePreview: new 0x%lx\n", preview));

	if (preview)
	{
		DoMethod(dat->PreviewGroup, MUIM_Group_InitChange);
		if (dat->Preview)
		{
			DoMethod(dat->PreviewGroup, OM_REMMEMBER, dat->Preview);
			DisposeObject(dat->Preview);
		}
		DoMethod(dat->PreviewGroup, OM_ADDMEMBER, preview);
		DoMethod(dat->PreviewGroup, MUIM_Group_ExitChange);
		dat->Preview = preview;
	}

	return preview != NULL;
}

ULONG fiWriteFiles(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);
	BPTR file;
	char name[32];
	STRPTR base;
	BPTR olddir;

	get(dat->Name, MUIA_String_Contents, &base);
	if (!base || !base[0])
		return FALSE;

	olddir = CurrentDir(destdir);

	strcpy(name, base);
	strcat(name, ".otag");

	file = Open(name, MODE_NEWFILE);
	if (file)
	{
		struct TagItem *tag;
		ULONG size, indirect_size;
		UBYTE *buffer;
		int num_sizes;

		size = sizeof(tag->ti_Tag) + (fiSetOTags(cl, o) + 2) * sizeof(*tag);
		indirect_size = 0;

		for (tag = dat->OTags; tag->ti_Tag != TAG_END; ++tag)
		{
			if (tag->ti_Tag == OT_Spec2_CodePage)
			{
				indirect_size += 1;
				indirect_size &= ~1;
				indirect_size += sizeof(codepage);
			}
			else if (tag->ti_Tag & OT_Indirect && tag->ti_Data)
			{
				indirect_size += strlen((const char *)tag->ti_Data) + 1;
			}
		}

		indirect_size += 1;
		indirect_size &= ~1;

		num_sizes = 1 + dat->AvailSizes[0];
		indirect_size += num_sizes * sizeof(UWORD);

		dat->OTags[0].ti_Data = size + indirect_size;

		buffer = malloc(indirect_size);
		if (buffer)
		{
			size_t offset = 0;
		    struct TagItem *write_tags;
		    
			memset(buffer, 0, indirect_size);

			for (tag = dat->OTags; tag->ti_Tag != TAG_END; ++tag)
			{
				if (tag->ti_Tag == OT_Spec2_CodePage)
				{
					offset += 1;
					offset &= ~1;
					memcpy(buffer + offset, codepage, sizeof(codepage));
					tag->ti_Data = size + offset;
					offset += sizeof(codepage);
				}
				else if (tag->ti_Tag & OT_Indirect && tag->ti_Data)
				{
					size_t len = strlen((const char *)tag->ti_Data) + 1;
					memcpy(buffer + offset, (const char *)tag->ti_Data, len);
					tag->ti_Data = size + offset;
					offset += len;
				}
			}

			offset += 1;
			offset &= ~1;

			tag->ti_Tag = OT_AvailSizes;
			tag->ti_Data = size + offset;
			++tag;
		    
			tag->ti_Tag = TAG_END;

			memcpy(buffer + offset, dat->AvailSizes, num_sizes * sizeof(UWORD));
			offset += num_sizes * sizeof(UWORD);

		    write_tags = malloc(size);
		    memcpy(write_tags, dat->OTags, size);
		    for (tag = write_tags; tag->ti_Tag != TAG_END; tag++)
		    {
			tag->ti_Tag = AROS_LONG2BE(tag->ti_Tag);
			tag->ti_Data = AROS_LONG2BE(tag->ti_Data);
		    }
		    tag->ti_Tag = AROS_LONG2BE(TAG_END);
		    
			Write(file, write_tags, size);
		    free(write_tags);
			Write(file, buffer, offset);

			free(buffer);
		}

		Close(file);
	}

	strcpy(name, base);
	strcat(name, ".font");

	file = Open(name, MODE_NEWFILE);
	if (file)
	{
		static UBYTE data[] = {0x0f, 0x03, 0x00, 0x00 };
		Write(file, data, sizeof(data));
		Close(file);
	}

	CurrentDir(olddir);

	return 1;
}

AROS_UFH3(ULONG, FontInfoDispatch,
		AROS_UFHA(Class *, cl, A0),
		AROS_UFHA(Object *, o, A2),
		AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG ret;

	switch (msg->MethodID)
	{
		case OM_NEW:
			ret = fiNew(cl, o, (struct opSet *)msg);
			break;

#if 0
		case OM_DISPOSE:
			ret = fiDispose(cl, o);
			break;
#endif

		case MUIM_FontInfo_UpdatePreview:
			ret = fiUpdatePreview(cl, o);
			break;

		case MUIM_FontInfo_WriteFiles:
			ret = fiWriteFiles(cl, o);
			break;

		default:
			ret = DoSuperMethodA(cl, o, msg);
			break;
	}

	return ret;
	
	AROS_USERFUNC_EXIT
}


void CleanupFontInfoClass(void)
{
	if (FontInfoClass)
	{
		MUI_DeleteCustomClass(FontInfoClass);
		FontInfoClass = NULL;
	}
}

int InitFontInfoClass(void)
{
	FontInfoClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL,
			sizeof(FontInfoData), UFHN(FontInfoDispatch));
	return FontInfoClass != NULL;
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

struct FontWindowData
{
	FT_Face Face;
};
typedef struct FontWindowData FontWindowData;

#define FONTWINDOW_BASE			TAG_USER
#define MUIA_FontWindow_Filename	(FONTWINDOW_BASE + 1)

struct MUI_CustomClass *FontWindowClass;

#define FontWindowObject	NewObject(FontWindowClass->mcc_Class, NULL //)

AROS_UFH3(void, CloseWinFunc,
		AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(Object *, app, A2),
		AROS_UFHA(Object **, winp, A1))
{
	AROS_USERFUNC_INIT
	
	set(*winp, MUIA_Window_Open, FALSE);
	DoMethod(app, OM_REMMEMBER, *winp);
	MUI_DisposeObject(*winp);

	AROS_USERFUNC_EXIT
}

struct Hook CloseWinHook = {{NULL, NULL}, UFHN(CloseWinFunc) };

ULONG fwNew(Class *cl, Object *o, struct opSet *msg)
{
	struct opSet method;
	struct TagItem tags[4];
	STRPTR filename = (STRPTR)GetTagData(MUIA_FontWindow_Filename, (IPTR) NULL, msg->ops_AttrList);
	FT_Face face;
	FT_Error error;
	Object *install, *close, *info, *app;

	if (filename == NULL)
	{
		DEBUG_FONTWINDOW(dprintf("FontWindow: no filename.\n"));
		return 0;
	}

	error = FT_New_Face(ftlibrary, filename, 0, &face);
	if (error)
	{
		DEBUG_FONTWINDOW(dprintf("FontWindow: New_Face error %d.\n", error));
		return 0;
	}

	app = (Object *)GetTagData(MUIA_UserData, 0,
				   msg->ops_AttrList);
	if (NULL == app)
	{
		DEBUG_FONTWINDOW(dprintf("FontWindow: no app ptr.\n"));
		return 0;
	}

	tags[0].ti_Tag = MUIA_Window_ID;
	tags[0].ti_Data = MAKE_ID('F','O','N','T');
	tags[1].ti_Tag = MUIA_Window_Title;
	tags[1].ti_Data = (ULONG)filename;
	tags[2].ti_Tag = MUIA_Window_RootObject;
	tags[2].ti_Data = (ULONG)VGroup,
		Child, info = FontInfoObject,
			MUIA_FontInfo_Filename, filename,
			MUIA_FontInfo_Face, face,
			End,
		Child, HGroup,
			Child, install = SimpleButton("_Install"),
			Child, RectangleObject,
				End,
			Child, close = SimpleButton("_Close"),
			End,
		End;
	tags[3].ti_Tag = TAG_MORE;
	tags[3].ti_Data = (ULONG)msg->ops_AttrList;

	method.MethodID = OM_NEW;
	method.ops_AttrList = tags;
	method.ops_GInfo = NULL;

	o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);
	if (o)
	{
		FontWindowData *dat = INST_DATA(cl, o);
		dat->Face = face;

		DoMethod(install, MUIM_Notify, MUIA_Pressed, FALSE,
				info, 1, MUIM_FontInfo_WriteFiles);
		DoMethod(close, MUIM_Notify, MUIA_Pressed, FALSE,
			 app, 6, MUIM_Application_PushMethod, app, 3,
			 MUIM_CallHook, &CloseWinHook, o);

	}
	else
	{
		FT_Done_Face(face);
	}

	DEBUG_FONTWINDOW(dprintf("FontWindow: created object 0x%lx.\n", o));

	return (ULONG)o;
}

ULONG fwDispose(Class *cl, Object *o)
{
	FontWindowData *dat = INST_DATA(cl, o);

	DEBUG_FONTWINDOW(dprintf("FontWindow: destroy object 0x%lx\n", o));

	FT_Done_Face(dat->Face);

	return DoSuperMethod(cl, o, OM_DISPOSE);
}

AROS_UFH3(ULONG, FontWindowDispatch,
		AROS_UFHA(Class *, cl, A0),
		AROS_UFHA(Object *, o, A2),
		AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG ret;

	switch (msg->MethodID)
	{
		case OM_NEW:
			ret = fwNew(cl, o, (struct opSet *)msg);
			break;

		case OM_DISPOSE:
			ret = fwDispose(cl, o);
			break;

		default:
			ret = DoSuperMethodA(cl, o, msg);
			break;
	}

	return ret;
	
	AROS_USERFUNC_EXIT
}


void CleanupFontWindowClass(void)
{
	if (FontWindowClass)
	{
		MUI_DeleteCustomClass(FontWindowClass);
		FontWindowClass = NULL;
	}
}

int InitFontWindowClass(void)
{
	FontWindowClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL,
			sizeof(FontWindowData), UFHN(FontWindowDispatch));
	return FontWindowClass != NULL;
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

struct FontListData
{
	struct MinList	ScanDirTasks;
};
typedef struct FontListData FontListData;

struct ScanDirTaskInfo
{
	struct MinNode	Node;
	struct Process	*Proc;
	STRPTR			DirName;
	char			NameBuf[256];
	char			Buffer[4096];
};

#define FONTLIST_MBASE			TAG_USER
#define MUIM_FontList_AddDir		(FONTLIST_MBASE + 1)
#define MUIM_FontList_AddEntry		(FONTLIST_MBASE + 2)

struct MUI_CustomClass *FontListClass;

#define FontListObject	NewObject(FontListClass->mcc_Class, NULL //)

struct MUIS_FontList_Entry
{
	STRPTR FileName;
	STRPTR FamilyName;
	STRPTR StyleName;
};

AROS_UFH3(APTR, flConstructFunc,
    	    	AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(APTR, pool, A2),
		AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
	AROS_USERFUNC_INIT
	
	struct MUIS_FontList_Entry *new_entry;
	size_t len1 = strlen(entry->FileName) + 1;
	size_t len2 = strlen(entry->FamilyName) + 1;
	size_t len3 = strlen(entry->StyleName) + 1;

	new_entry = AllocPooled(pool, sizeof(*entry) + len1 + len2 + len3);
	if (new_entry)
	{
		STRPTR p = (STRPTR)(new_entry + 1);
		new_entry->FileName = p;
		memcpy(p, entry->FileName, len1);
		p += len1;
		new_entry->FamilyName = p;
		memcpy(p, entry->FamilyName, len2);
		p += len2;
		new_entry->StyleName = p;
		memcpy(p, entry->StyleName, len3);
	}

	return new_entry;
	
	AROS_USERFUNC_EXIT
}


struct Hook flConstructHook = {{NULL, NULL}, UFHN(flConstructFunc) };

AROS_UFH3(void, flDestructFunc,
    	    	AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(APTR, pool, A2),
		AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
	AROS_USERFUNC_INIT
	
	size_t len1 = strlen(entry->FileName) + 1;
	size_t len2 = strlen(entry->FamilyName) + 1;
	size_t len3 = strlen(entry->StyleName) + 1;

	FreePooled(pool, entry, sizeof(*entry) + len1 + len2 + len3);
	
	AROS_USERFUNC_EXIT
}


struct Hook flDestructHook = {{NULL, NULL}, UFHN(flDestructFunc) };

AROS_UFH3(ULONG, flDisplayFunc,
    	    	AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(STRPTR *, array, A2),
		AROS_UFHA(struct MUIS_FontList_Entry *, entry, A1))
{
	AROS_USERFUNC_INIT
	
	array[0] = entry->FamilyName;
	array[1] = entry->StyleName;
	return 0;
	
	AROS_USERFUNC_EXIT
}


struct Hook flDisplayHook = {{NULL, NULL}, UFHN(flDisplayFunc) };

AROS_UFH3(LONG, flCompareFunc,
		AROS_UFHA(struct Hook *, hook, A0),
		AROS_UFHA(struct MUIS_FontList_Entry *, entry2, A2),
		AROS_UFHA(struct MUIS_FontList_Entry *, entry1, A1))
{
	AROS_USERFUNC_INIT
	
	LONG ret = strcmp(entry1->FamilyName, entry2->FamilyName);
	if (ret == 0)
		ret = strcmp(entry1->StyleName, entry2->StyleName);
	return ret;
	
	AROS_USERFUNC_EXIT
}


struct Hook flCompareHook = {{NULL, NULL}, UFHN(flCompareFunc) };


ULONG flNew(Class *cl, Object *o, struct opSet *msg)
{
	struct opSet method;
	struct TagItem tags[8];

	tags[0].ti_Tag = MUIA_Frame;
	tags[0].ti_Data = MUIV_Frame_InputList;
	tags[1].ti_Tag = MUIA_Background;
	tags[1].ti_Data = MUII_ListBack;
	tags[2].ti_Tag = MUIA_List_ConstructHook;
	tags[2].ti_Data = (ULONG)&flConstructHook;
	tags[3].ti_Tag = MUIA_List_DestructHook;
	tags[3].ti_Data = (ULONG)&flDestructHook;
	tags[4].ti_Tag = MUIA_List_DisplayHook;
	tags[4].ti_Data = (ULONG)&flDisplayHook;
	tags[4].ti_Tag = MUIA_List_DisplayHook;
	tags[4].ti_Data = (ULONG)&flDisplayHook;
	tags[5].ti_Tag = MUIA_List_CompareHook;
	tags[5].ti_Data = (ULONG)&flCompareHook;
	tags[6].ti_Tag = MUIA_List_Format;
	tags[6].ti_Data = (ULONG)",";
	tags[7].ti_Tag = TAG_MORE;
	tags[7].ti_Data = (ULONG)msg->ops_AttrList;

	method.MethodID = OM_NEW;
	method.ops_AttrList = tags;
	method.ops_GInfo = NULL;

	o = (Object *)DoSuperMethodA(cl, o, (Msg)&method);
	if (o)
	{
		FontListData *dat = INST_DATA(cl, o);
		NewList((struct List *) &dat->ScanDirTasks);
	}

	DEBUG_FONTWINDOW(dprintf("FontList: created object 0x%lx.\n", o));

	return (ULONG)o;
}


ULONG flDispose(Class *cl, Object *o, Msg msg)
{
	FontListData *dat = INST_DATA(cl, o);
	struct ScanDirTaskInfo *info;
	BOOL done;

	do
	{
		done = TRUE;

		Forbid();
		for (info = (APTR)dat->ScanDirTasks.mlh_Head; info->Node.mln_Succ; info = (APTR)info->Node.mln_Succ)
		{
			done = FALSE;
			Signal(&info->Proc->pr_Task, SIGBREAKF_CTRL_C);
		}
		Permit();

		if (!done)
			Wait(SIGBREAKF_CTRL_F);
	}
	while (!done);

	return DoSuperMethodA(cl, o, msg);
}


struct MUIP_FontList_AddDir
{
	ULONG MethodID;
	STRPTR DirName;
};

struct ScanDirTaskInfo *_pass_info;
Object *_pass_app;
struct Task *_pass_parent;
Object *_pass_fl;
			 
void ScanDirTask(void)
{
    struct ScanDirTaskInfo *info = _pass_info;
    Object *app = _pass_app;
    struct Task *parent = _pass_parent;
    Object *fl = _pass_fl;
	BPTR lock;
	struct ExAllControl *eac;
	struct ExAllData *ead;
	ULONG more;
	BPTR olddir;
	FT_Library ftlibrary;

#warning FIXME: Possible thread race conflicts not checked !!!
        void *test = GetIntETask(parent)->iet_acpd;
        GetIntETask(FindTask(NULL))->iet_acpd = test;

	Signal(parent, SIGBREAKF_CTRL_F);

        DEBUG_ADDDIR(dprintf("flScanDirTask: dir <%s>\n", info->DirName));

	if (FT_Init_FreeType(&ftlibrary) == 0)
	{
		DEBUG_ADDDIR(dprintf("flScanDirTask: ftlibrary 0x%x\n", ftlibrary));

		lock = Lock(info->DirName, ACCESS_READ);
		if (lock)
		{
			DEBUG_ADDDIR(dprintf("flScanDirTask: lock 0x%lx\n", lock));

			olddir = CurrentDir(lock);

			eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
			if (eac)
			{
				DEBUG_ADDDIR(dprintf("flScanDirTask: eac 0x%lx\n", eac));

				eac->eac_LastKey = 0;

				do
				{
					more = ExAll(lock, (struct ExAllData *) info->Buffer, sizeof(info->Buffer), ED_NAME, eac);

					DEBUG_ADDDIR(dprintf("flScanDirTask: more %d entries %d\n", more, eac->eac_Entries));


					if (!more && IoErr() != ERROR_NO_MORE_ENTRIES)
					{
						DEBUG_ADDDIR(dprintf("flScanDirTask: err %d\n", IoErr()));
						break;
					}

					if (eac->eac_Entries == 0)
						continue;

					ead = (APTR)info->Buffer;

					do
					{
						FT_Face face;
						FT_Error error;

						DEBUG_ADDDIR(dprintf("flScanDirTask: ead 0x%x name %x <%s>\n", ead, ead->ed_Name, ead->ed_Name));
						error = FT_New_Face(ftlibrary, ead->ed_Name, 0, &face);
						DEBUG_ADDDIR(dprintf("flScanDirTask: error %d\n", error));
						if (error == 0)
						{
							struct MUIS_FontList_Entry *entry;
							size_t len1, len2, len3;

							DEBUG_ADDDIR(dprintf("flScanDirTask: family 0x <%s> style 0x%x <%s>\n", face->family_name, face->family_name, face->style_name, face->style_name));

							strncpy(info->NameBuf, info->DirName, sizeof(info->NameBuf));
							AddPart(info->NameBuf, ead->ed_Name, sizeof(info->NameBuf));

							len1 = strlen(info->NameBuf) + 1;
							len2 = strlen(face->family_name) + 1;
							len3 = strlen(face->style_name) + 1;

							entry = AllocVec(sizeof(*entry) + len1 + len2 + len3, MEMF_PUBLIC);
							if (entry)
							{
								char *p = (char *)(entry + 1);
								entry->FileName = p;
								memcpy(p, info->NameBuf, len1);
								p += len1;
								entry->FamilyName = p;
								memcpy(p, face->family_name, len2);
								p += len2;
								entry->StyleName = p;
								memcpy(p, face->style_name, len3);

								if (!DoMethod(app, MUIM_Application_PushMethod,
											fl, 2, MUIM_FontList_AddEntry, entry))
									FreeVec(entry);
							}

							FT_Done_Face(face);
						}

						ead = ead->ed_Next;
					}
					while (ead);
				}
				while (more);

				DEBUG_ADDDIR(dprintf("flScanDirTask: done\n"));

				FreeDosObject(DOS_EXALLCONTROL, eac);
			}

			CurrentDir(olddir);
			UnLock(lock);
		}

		FT_Done_FreeType(ftlibrary);
	}

	Forbid();
	REMOVE(&info->Node);
	FreeVec(info);
	Signal(parent, SIGBREAKF_CTRL_F);
}


ULONG flAddDir(Class *cl, Object *o, struct MUIP_FontList_AddDir *msg)
{
	FontListData *dat = INST_DATA(cl, o);
	struct ScanDirTaskInfo *info;
	int dirname_len = strlen(msg->DirName) + 1;

	info = AllocVec(sizeof(*info) + dirname_len, MEMF_CLEAR | MEMF_PUBLIC);
	if (info)
	{
		info->DirName = (STRPTR)(info + 1);
		memcpy(info->DirName, msg->DirName, dirname_len);

	        _pass_info = info;
	        _pass_app = app;
	        _pass_parent = FindTask(NULL);
	        _pass_fl = o;
	        Forbid();
		info->Proc = CreateNewProcTags(
				NP_Entry, ScanDirTask,
				TAG_END);
		if (info->Proc)
		{
			ADDTAIL((APTR)&dat->ScanDirTasks, (APTR)info);
			Permit();

			Wait(SIGBREAKF_CTRL_F);
			return TRUE;
		}
	    
	        Permit();
		FreeVec(info);
	}

	return FALSE;
}


struct MUIP_FontList_AddEntry
{
	ULONG	MethodID;
	struct MUIS_FontList_Entry *Entry;
};

ULONG flAddEntry(Class *cl, Object *o, struct MUIP_FontList_AddEntry *msg)
{
#ifdef __AROS__
#warning "MUIV_List_Insert_Sorted not yet working in AROS"
	DoMethod(o, MUIM_List_InsertSingle, msg->Entry, MUIV_List_Insert_Bottom);
#else
	DoMethod(o, MUIM_List_InsertSingle, msg->Entry, MUIV_List_Insert_Sorted);
#endif
	FreeVec(msg->Entry);
	return TRUE;
}


AROS_UFH3(ULONG, FontListDispatch,
		AROS_UFHA(Class *, cl, A0),
		AROS_UFHA(Object *, o, A2),
		AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	
	ULONG ret;

	switch (msg->MethodID)
	{
		case OM_NEW:
			ret = flNew(cl, o, (struct opSet *)msg);
			break;

		case OM_DISPOSE:
			ret = flDispose(cl, o, msg);
			break;

		case MUIM_FontList_AddDir:
			ret = flAddDir(cl, o, (struct MUIP_FontList_AddDir *)msg);
			break;

		case MUIM_FontList_AddEntry:
			ret = flAddEntry(cl, o, (struct MUIP_FontList_AddEntry *)msg);
			break;

		default:
			ret = DoSuperMethodA(cl, o, msg);
			break;
	}

	return ret;
	
	AROS_USERFUNC_EXIT
}


void CleanupFontListClass(void)
{
	if (FontListClass)
	{
		MUI_DeleteCustomClass(FontListClass);
		FontListClass = NULL;
	}
}

int InitFontListClass(void)
{
	FontListClass = MUI_CreateCustomClass(NULL, MUIC_List, NULL,
			sizeof(FontListData), UFHN(FontListDispatch));
	return FontListClass != NULL;
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

void Cleanup(void)
{
	CleanupFontListClass();
	CleanupFontWindowClass();
	CleanupFontInfoClass();
	CleanupFontBitmapClass();

	FT_Done_Library(ftlibrary);

	UnLock(destdir);
}

int Init(void)
{
	FT_Error error;

	error = FT_Init_FreeType(&ftlibrary);
	if (error != 0)
	{
		DEBUG_MAIN(dprintf("Init_FreeType error %d\n", error));
		return 0;
	}

	if (!InitFontBitmapClass() ||
			!InitFontInfoClass() ||
			!InitFontWindowClass() ||
			!InitFontListClass())
	{
		DEBUG_MAIN(dprintf("Can't create custom classes.\n"));
		return 0;
	}

	destdir = Lock("Fonts:", ACCESS_READ);

	return 1;
}


void SetDefaultCodePage(void)
{
	int k;
	for (k = 0; k < 256; ++k)
		codepage[k] = k;
}

BOOL IsDefaultCodePage(void)
{
	int k;
	for (k = 0; k < 256; ++k)
		if (codepage[k] != k)
			return FALSE;
	return TRUE;
}

BOOL LoadCodePage(const char *filename)
{
	BOOL ret = FALSE;
	BPTR file;
	UBYTE buf[256];

	SetDefaultCodePage();

	file = Open(filename, MODE_OLDFILE);
	if (file)
	{
		ret = TRUE;

		while (FGets(file, buf, sizeof(buf)))
		{
			STRPTR p = buf;
			LONG index, unicode;
			LONG chars;

			if (*p == '#')
				continue;

			chars = StrToLong(p, &index);

			if (chars <= 0 || index < 0 || index > 255)
				continue;

			p += chars;

			chars = StrToLong(p, &unicode);

			if (chars <= 0)
				continue;

			codepage[index] = (UWORD)unicode;
		}

		Close(file);
	}

	return ret;
}


#define ID_SetSrc	1
#define ID_SetDestDir	2
#define ID_ShowFont	3
#define ID_SetCodePage	4

int main(void)
{
	int ret = RETURN_FAIL;
	Object *win, *src, *dest, *fontlist, *fontlv, *codepagestr, *quit;

	if (!Init())
		return RETURN_FAIL;

	SetDefaultCodePage();

	app = ApplicationObject,
		MUIA_Application_Title, "FTManager",
		MUIA_Application_Version, "$VER: FTManager 1.1 (17.1.2003)",
		MUIA_Application_Copyright, "Copyright 2002-2003 by Emmanuel Lesueur",
		MUIA_Application_Author, "Emmanuel Lesueur",
		MUIA_Application_Description, "FreeType font manager",
		MUIA_Application_Base, "FTMANAGER",
		SubWindow, win = WindowObject,
			MUIA_Window_ID, MAKE_ID('F','T','2','M'),
			MUIA_Window_Title, "FreeType Font Manager",
                        MUIA_Window_Width, 400,
			MUIA_Window_RootObject,VGroup,
				Child, fontlv = ListviewObject,
					MUIA_Listview_List, fontlist = FontListObject,
						End,
					End,
				Child, ColGroup(2),
					Child, Label2("Source"),
					Child, PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, src = StringObject,
							StringFrame,
							MUIA_String_Contents, "Fonts:TrueType",
                                                        MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain, TRUE,
							End,
						MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
						ASLFR_RejectIcons, TRUE,
						ASLFR_DrawersOnly, TRUE,
						End,
					Child, Label2("Destination"),
					Child, PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, dest = StringObject,
							StringFrame,
							MUIA_String_Contents, "Fonts:",
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain, TRUE,
							End,
						MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
						ASLFR_DoSaveMode, TRUE,
						ASLFR_DrawersOnly, TRUE,
						ASLFR_RejectIcons, TRUE,
						End,
					Child, Label2("Codepage"),
					Child, PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, codepagestr = StringObject,
							StringFrame,
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain, TRUE,
							End,
						MUIA_Popstring_Button, PopButton(MUII_PopFile),
						ASLFR_RejectIcons, TRUE,
						End,
					End,
				Child, quit = SimpleButton("_Quit"),
				End,
			End,
		End;

	if (app)
	{
		ULONG t;

		ret = RETURN_OK;

		DoMethod(src, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				fontlist, 2, MUIM_FontList_AddDir, MUIV_TriggerValue);

		DoMethod(dest, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				app, 2, MUIM_Application_ReturnID, ID_SetDestDir);

		DoMethod(codepagestr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				app, 2, MUIM_Application_ReturnID, ID_SetCodePage);

		DoMethod(fontlv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
				app, 2, MUIM_Application_ReturnID, ID_ShowFont);

		DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
				app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                
		DoMethod(quit, MUIM_Notify, MUIA_Pressed, FALSE,
				app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                
                DoMethod(fontlist, MUIM_FontList_AddDir, XGET(src, MUIA_String_Contents));
                
		set(win, MUIA_Window_Open, TRUE);
		get(win, MUIA_Window_Open, &t);
		if (t)
		{
			BOOL running = TRUE;
			ULONG sigs = 0;
			ULONG id;
                        
			do
			{
				id = DoMethod(app, MUIM_Application_NewInput, &sigs);
				switch (id)
				{
					case MUIV_Application_ReturnID_Quit:
						running = FALSE;
						sigs = 0;
						break;

					case ID_SetDestDir:
						{
							STRPTR name;
							BPTR newdir;

							get(dest, MUIA_String_Contents, &name);
							newdir = Lock(name, ACCESS_READ);
							if (newdir)
							{
								UnLock(destdir);
								destdir = newdir;
							}
						}
						break;

					case ID_ShowFont:
						{
							struct MUIS_FontList_Entry *entry;
							Object *w;

							DoMethod(fontlist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

							w = FontWindowObject,
							    MUIA_FontWindow_Filename, entry->FileName,
							    MUIA_UserData, app,
								End;

							if (w)
							{
								DoMethod(w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
										app, 6, MUIM_Application_PushMethod, app, 3,
										MUIM_CallHook, &CloseWinHook, w);

								DoMethod(app, OM_ADDMEMBER, w);
								set(w, MUIA_Window_Open, TRUE);
								get(w, MUIA_Window_Open, &t);
								if (!t)
								{
									MUI_DisposeObject(w);
								}
							}

						}
						break;

					case ID_SetCodePage:
						{
							STRPTR filename;

							get(codepagestr, MUIA_String_Contents, &filename);
							LoadCodePage(filename);
						}
						break;
				}

				if (sigs)
				{
					sigs = Wait(sigs | SIGBREAKF_CTRL_C);
					if (sigs & SIGBREAKF_CTRL_C)
					{
						running = FALSE;
					}
				}
			}
			while (running);
		}
		else
		{
			printf("Can't open window.\n");
		}

		MUI_DisposeObject(app);
	}
	else
	{
		printf("Can't create MUI application.\n");
	}

	Cleanup();

	return ret;
}
