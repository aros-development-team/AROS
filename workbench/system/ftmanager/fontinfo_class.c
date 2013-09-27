#include <proto/alib.h>

#define NO_INLINE_STDARG

#include <aros/debug.h>
#include <exec/types.h>
#include <diskfont/diskfonttag.h>
#define MUI_OBSOLETE
#include <libraries/mui.h>

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <proto/dos.h>

#include "fontinfo_class.h"
#include "fontbitmap_class.h"
#include "globals.h"
#include "locale.h"

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

	IPTR entry = 0;
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

	IPTR t = 0;

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

	IPTR t = 0;
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


IPTR fiNew(Class *cl, Object *o, struct opSet *msg)
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
	char name_buf[50];

	static CONST_STRPTR stem_weight_names[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        stem_weight_names[0] =_(MSG_WEIGHT_ULTRATHIN);
        stem_weight_names[1] =_(MSG_WEIGHT_EXTRATHIN);
        stem_weight_names[2] =_(MSG_WEIGHT_THIN);
        stem_weight_names[3] =_(MSG_WEIGHT_EXTRALIGHT);
        stem_weight_names[4] =_(MSG_WEIGHT_LIGHT);
        stem_weight_names[5] =_(MSG_WEIGHT_DEMILIGHT);
        stem_weight_names[6] =_(MSG_WEIGHT_SEMILIGHT);
        stem_weight_names[7] =_(MSG_WEIGHT_BOOK);
        stem_weight_names[8] =_(MSG_WEIGHT_MEDIUM);
        stem_weight_names[9] =_(MSG_WEIGHT_SEMIBOLD);
        stem_weight_names[10] =_(MSG_WEIGHT_DEMIBOLD);
        stem_weight_names[11] =_(MSG_WEIGHT_BOLD);
        stem_weight_names[12] =_(MSG_WEIGHT_EXTRABOLD);
        stem_weight_names[13] =_(MSG_WEIGHT_BLACK);
        stem_weight_names[14] =_(MSG_WEIGHT_EXTRABLACK);
        stem_weight_names[15] =_(MSG_WEIGHT_ULTRABLACK);
        stem_weight_names[16] =_(MSG_WEIGHT_CUSTOM);

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

	static CONST_STRPTR slant_style_names[] = {NULL, NULL, NULL, NULL};
		slant_style_names[0] = _(MSG_STYLE_UPRIGHT);
		slant_style_names[1] = _(MSG_STYLE_ITALIC);
		slant_style_names[2] = _(MSG_STYLE_LEFTITALIC);

	static CONST_STRPTR horiz_style_names[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		horiz_style_names[0] = _(MSG_HSTYLE_ULTRACOMPRESSED);
		horiz_style_names[1] = _(MSG_HSTYLE_EXTRACOMPRESSED);
		horiz_style_names[2] = _(MSG_HSTYLE_COMPRESSED);
		horiz_style_names[3] = _(MSG_HSTYLE_CONDENSED);
		horiz_style_names[4] = _(MSG_HSTYLE_NORMAL);
		horiz_style_names[5] = _(MSG_HSTYLE_SEMIEXPANDED);
		horiz_style_names[6] = _(MSG_HSTYLE_EXPANDED);
		horiz_style_names[7] = _(MSG_HSTYLE_EXTRAEXPANDED);
		horiz_style_names[8] = _(MSG_HSTYLE_CUSTOM);

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

	static CONST_STRPTR metric_names[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		metric_names[0] = _(MSG_METRIC_GLOBALBOUNDINGBOX);
		metric_names[1] = _(MSG_METRIC_RAWFONTMETRIC);
		metric_names[2] = _(MSG_METRIC_ASCENDER);
		metric_names[3] = _(MSG_METRIC_TYPOASCENDER);
		metric_names[4] = _(MSG_METRIC_USWINASCENDER);
		metric_names[5] = _(MSG_METRIC_CUSTOMBOUNDINGBOX);
		//"Bitmap size"


	if (!filename || !face)
	{
		DEBUG_FONTINFO(dprintf("FontInfo: filename 0x%x face 0x%x\n", filename, face));
		return 0;
	}

	strlcpy(name_buf, face->family_name, sizeof name_buf);
	strlcat(name_buf, " ", sizeof name_buf);
	strlcat(name_buf, face->style_name, sizeof name_buf);

	postscript = FT_Get_Sfnt_Table(face, ft_sfnt_post);
	os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);

	tags[0].ti_Tag = MUIA_Group_Child;
	tags[0].ti_Data = (IPTR)ColGroup(2),
		Child, Label2(_(MSG_LABEL_EXTRAFILE)),
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
		Child, Label2(_(MSG_LABEL_FACENUM)),
		Child, face_num = StringObject,
			StringFrame,
			MUIA_String_Integer, 0,
			MUIA_String_Accept, "0123456789",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 5,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2(_(MSG_LABEL_NAME)),
		Child, name = StringObject,
			StringFrame,
			MUIA_String_Contents, name_buf,
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 50,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2(_(MSG_LABEL_FAMILY)),
		Child, family = StringObject,
			StringFrame,
			MUIA_String_Contents, face->family_name,
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2(_(MSG_LABEL_METRIC)),
		Child, metric = CycleObject,
			MUIA_Cycle_Entries, metric_names,
			MUIA_Cycle_Active, 1,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2(_(MSG_LABEL_BOUNDINGBOX)),
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
		Child, Label2(_(MSG_LABEL_SIZEFACTOR)),
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
		Child, Label2(_(MSG_LABEL_SPACEWIDTH)),
		Child, space_width = StringObject,
			StringFrame,
			MUIA_String_Accept, "0123456789",
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_String_MaxLen, 6,
			MUIA_String_Integer, (IPTR)(face->max_advance_width * 250.0 / 72.307),
			MUIA_CycleChain, TRUE,
			End,
		Child, Label1(_(MSG_LABEL_FIXEDWIDTH)),
		Child, HGroup,
			Child, fixed = CheckMark((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0),
			Child, RectangleObject,
				End,
			End,
		Child, Label1(_(MSG_LABEL_SERIF)),
		Child, HGroup,
			Child, serif = CheckMark(os2 && (unsigned)(((os2->sFamilyClass >> 8) &
							0xff) - 1) < 5),
			Child, RectangleObject,
				End,
			End,
		Child, Label2(_(MSG_LABEL_STEMWEIGHT)),
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
		Child, Label2(_(MSG_LABEL_SLANTSTYLE)),
		Child, slant_style = CycleObject,
			MUIA_Cycle_Entries, slant_style_names,
			MUIA_Cycle_Active, face->style_flags & FT_STYLE_FLAG_ITALIC ?
				(postscript && postscript->italicAngle > 0 ?
				 OTS_LeftItalic : OTS_Italic) : OTS_Upright,
			MUIA_CycleChain, TRUE,
			End,
		Child, Label2(_(MSG_LABEL_HORIZSTYLE)),
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
	tags[1].ti_Data = (IPTR)VGroup,
		GroupFrameT(_(MSG_FRAME_PREVIEW)),
		Child, test_string = StringObject,
			StringFrame,
			MUIA_String_Contents, _(MSG_PREVIEW_STRING),
			MUIA_String_AdvanceOnCR, TRUE,
			MUIA_CycleChain, TRUE,
			End,
		Child, HGroup,
			Child, Label2(_(MSG_LABEL_SIZE)),
			Child, test_size = StringObject,
				StringFrame,
				MUIA_String_Accept, "0123456789",
				MUIA_String_Integer, 30,
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_CycleChain, TRUE,
				End,
			Child, Label2(_(MSG_LABEL_ANTIALIASING)),
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
	tags[2].ti_Data = (IPTR)msg->ops_AttrList;

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

	return (IPTR)o;
}

#if 0
IPTR fiDispose(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);

	return DoSuperMethod(cl, o, OM_DISPOSE);
}
#endif

IPTR fiSetOTags(Class *cl, Object *o)
{
	FontInfoData *dat = INST_DATA(cl, o);
	struct TagItem *tag = dat->OTags;
	IPTR x = 0;
	IPTR y = 0;

	tag->ti_Tag = OT_FileIdent;
	++tag;

	tag->ti_Tag = OT_Engine;
	tag->ti_Data = (IPTR)"freetype2";
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
	tag->ti_Data = (IPTR)dat->Filename;
	++tag;

	tag->ti_Tag = OT_Spec3_AFMFile;
	get(dat->AttachedFile, MUIA_String_Contents, &tag->ti_Data);
	++tag;

	tag->ti_Tag = OT_Spec4_Metric;
	get(dat->Metric, MUIA_Cycle_Active, &tag->ti_Data);
	++tag;

	if (tag[-1].ti_Data == METRIC_CUSTOMBBOX)
	{
		IPTR ymin = 0;
		IPTR ymax = 0;

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
		tag->ti_Data = (IPTR)codepage;
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
	STRPTR str = NULL;
	IPTR gray = 0;
	IPTR size = 0;

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
	STRPTR base = NULL;
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

		/*
		 * In the file we store 32-bit form of taglist.
		 * Each complete tag can be represented by UQUAD (two ULONGs),
		 * and we also append one ULONG for TAG_DONE terminator
		 */
		size = sizeof(ULONG) + (fiSetOTags(cl, o) + 2) * sizeof(UQUAD);
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
		    ULONG *write_tags;
		    ULONG *dest;

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

		    dest = write_tags;
		    for (tag = dat->OTags; tag->ti_Tag != TAG_END; tag++)
		    {
			dest[0] = AROS_LONG2BE(tag->ti_Tag);
			dest[1] = AROS_LONG2BE(tag->ti_Data);
			dest += 2;
		    }
		    dest[0] = AROS_LONG2BE(TAG_END);

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
