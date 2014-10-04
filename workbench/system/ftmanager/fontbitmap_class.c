#include <aros/debug.h>
#include <libraries/mui.h>
#include <graphics/gfx.h>
#include <diskfont/diskfonttag.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/freetype2.h>

#include <string.h>

#include "fontbitmap_class.h"
#include "globals.h"

struct FontBitmapData
{
	int Width, Height;
	struct BitMap BitMap;
	struct BitMap *GrayBitMap;
};

typedef struct FontBitmapData FontBitmapData;

IPTR fbNew(Class *cl, Object *o, struct opSet *msg)
{
	struct opSet method;
	struct TagItem tags[5];
	STRPTR filename = (STRPTR)GetTagData(MUIA_FontBitmap_Filename, (IPTR) NULL, msg->ops_AttrList);
	STRPTR string = (STRPTR)GetTagData(MUIA_FontBitmap_String, (IPTR) "?", msg->ops_AttrList);
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
			OT_OTagList, (IPTR) otags,
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
					OT_TextKernPair, (IPTR)&kerning,
					TAG_END);

			x -= (int)(kerning / 65536.0 * size);
		}

		info[k].x = x;
		info[k].y = y;

		SetInfo(engine,
				OT_GlyphCode, code,
				TAG_END);
		ObtainInfo(engine,
				tag, (IPTR)&info[k].glyph,
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
		tags[4].ti_Data = (IPTR)msg->ops_AttrList;

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
					ULONG k;
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
					tag, (IPTR)info[k].glyph,
					TAG_END);
		}
	}

	FreeVec(info);

	CloseEngine(engine);

	DEBUG_FONTBITMAP(dprintf("FontBitmap: created object 0x%lx.\n", o));

	return (IPTR)o;
}

IPTR fbDispose(Class *cl, Object *o)
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
