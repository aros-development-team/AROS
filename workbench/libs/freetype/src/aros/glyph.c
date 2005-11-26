/*
 * Based on the code from the ft2.library from MorphOS and the ttf.library by
 * Richard Griffith
 */
#include "ftglyphengine.h"
#include "glyph.h"

//#define DEBUG 1
#include <aros/debug.h>
#include <diskfont/oterrors.h>
#include <diskfont/glyph.h>
#include <exec/memory.h>

/* convert freetype FT_F26Dot6 to Amiga FIXED */
#define F266toFX(x)  (((x)<<10))

/* convert freetype FT_F26Dot6 to Integer */
//#define F266Round(x) (((x)+32>>6))
//#define F266Floor(x) (((x)>>6))
//#define F266Ceil(x)  ((((x)+63)>>6))
#define F266Round(x) ((((x)+0x10000000+32)>>6)-0x00400000)
#define F266Floor(x) ((((x)+0x10000000)>>6)-0x00400000)
#define F266Ceil(x)  ((((x)+0x10000000+63)>>6)-0x00400000)

void set_transform(FT_GlyphEngine *ge)
{
    if (ge->do_shear)
    {
	if (ge->do_rotate)
	{
	    ge->matrix = ge->shear_matrix;
	    FT_Matrix_Multiply(&ge->rotate_matrix, &ge->matrix);
	    FT_Set_Transform(ge->face, &ge->matrix, NULL);
	}
	else
	    FT_Set_Transform(ge->face, &ge->shear_matrix, NULL);
    }
    else if (ge->do_rotate)
	FT_Set_Transform(ge->face, &ge->rotate_matrix, NULL);
    else
	FT_Set_Transform(ge->face, NULL, NULL);
}


int char_to_glyph(FT_GlyphEngine *ge,int charcode)
{
    return FT_Get_Char_Index(ge->face, charcode & 0xffffff00 ? charcode : ge->codepage[charcode]);
}


int UnicodeToGlyphIndex(FT_GlyphEngine *ge)
{
    ge->glyph_code = FT_Get_Char_Index(ge->face,
				       ge->request_char & 0xffffff00 ? ge->request_char : ge->codepage[ge->request_char]);
    D(bug("Unicode(%d)=index(%d)=%d\n", ge->request_char,
	  ge->request_char & 0xffffff00 ? ge->request_char : ge->codepage[ge->request_char],
	  ge->glyph_code));

    return ge->glyph_code;
}


int SetInstance(FT_GlyphEngine *ge)
{
    FT_Error error;
    long yMin, yMax, alt_point, a, d;

    /* set instance characteristics */
    error = FT_Set_Char_Size( ge->face, 0, ge->point_size * 64, ge->xres, ge->yres );
    if (error)
    {
	D(bug("Could not set resolution.\n"));
	return set_last_error(ge,OTERR_Failure);
    }

    D(bug("units_per_EM %d metric source %d\n", ge->face->units_per_EM, ge->metric_source));

    ge->corrected_upem = ge->face->units_per_EM;

    /* adjust point size according to chosen metric source
     * each has pros and cons, so user gets to decide
     */
    if(ge->metric_source==METRIC_RAW_EM)
    {
	/* (nearly) raw em */
	alt_point = ge->point_size - 1;
	ge->corrected_upem = ge->face->units_per_EM;
    }
    else
    {
	TT_OS2 *os2 = NULL;
	
	if (ge->metric_source == METRIC_TYPOASCEND ||
	    ge->metric_source == METRIC_USWINASCEND)
	{
	    os2 = FT_Get_Sfnt_Table(ge->face, ft_sfnt_os2);
	    if (!os2)
	    {
		D(bug("No os2 table\n"));
		ge->metric_source = METRIC_GLOBALBBOX;
	    }
	}

	switch(ge->metric_source)
	{
	case METRIC_ASCEND: /* Horizontal Header */
	    yMax = ge->face->ascender;
	    yMin = ge->face->descender;
	    break;

	case METRIC_TYPOASCEND: /* Typo */
	    yMax = os2->sTypoAscender;
	    yMin = os2->sTypoDescender;
	    break;

	case METRIC_USWINASCEND: /* usWin */
	    yMax = os2->usWinAscent;
	    yMin = os2->usWinDescent;
	    break;

	case METRIC_CUSTOMBBOX:	/* custom bbox */
	    yMin=(WORD)ge->metric_custom;
	    yMax=ge->metric_custom >> 16;
	    break;

	case METRIC_BMSIZE:
	    yMin=0;
	    yMax=ge->face->height;
	    break;
	    
	default:
	case METRIC_GLOBALBBOX:	/* global bbox */
	    yMin=ge->face->bbox.yMin;
	    yMax=ge->face->bbox.yMax;
	    break;
	}

	D(bug("yMin %d yMax %d\n", yMin, yMax));

	/* normalize, provide defaults for bad fonts */
	if(yMax==0)
	{
	    /* not set, default */
	    yMin=ge->face->bbox.yMin;
	    yMax=ge->face->bbox.yMax;
	}
	if(yMin>0)
	    yMin = -yMin;

	D(bug("yMin %d yMax %d\n", yMin, yMax));

	/* split min/max to whole units on each side of baseline */
	ge->corrected_upem = yMax-yMin;

	D(bug("corrected_upem %d\n", ge->corrected_upem));

	alt_point = (ge->face->units_per_EM * ge->point_size) / (yMax-yMin);

	a= alt_point * yMax / (yMax-yMin);
	d= alt_point * (0-yMin) / (yMax-yMin);
	alt_point = alt_point - (alt_point - a - d);
    }

    D(bug("alt_point %d\n", alt_point));

    error = FT_Set_Pixel_Sizes(ge->face, alt_point, 0);
    if (error)
    {
	D(bug("Could not set point size.\n"));
	return set_last_error(ge,OTERR_Failure);
    }

    //FT_Get_Instance_Metrics(ge->instance, &ge->imetrics );

    set_transform(ge);

    ge->instance_changed=FALSE;

    //FT_Select_Charmap(ge->face, ft_encoding_unicode);
    //ChooseEncoding(ge);

    return set_last_error(ge,OTERR_Success);
}

/* render a glyph into GMap of engine
   caller in expected to have allocated GMap space (but not bitmap)
 */
void RenderGlyph(FT_GlyphEngine *ge, int glyph_8bits)
{
    FT_Error error;
    int do_transform;
    FT_Vector offset;
    FT_Glyph_Metrics *metrics;
    FT_Bitmap bitmap;
    FT_Raster_Params params;
    FT_BBox bbox;
    struct GlyphMap *GMap = ge->GMap;

    /* these two values indicate no glyph rendered */
    GMap->glm_Width = 0;
    GMap->glm_BitMap = NULL;

    D(bug("RenderGlyph(%d, %d, %d)\n", ge->request_char, ge->glyph_code, glyph_8bits));

    error = FT_Load_Glyph(ge->face,
			  ge->glyph_code,
			  FT_LOAD_NO_BITMAP);
    /*TTLOAD_SCALE_GLYPH); */ /* test disable hinting */

    if(error || ge->face->glyph->format != ft_glyph_format_outline)
    {
	D(bug("Error loading glyph %ld  code = %ld.\n",
	      (LONG)ge->request_char, (LONG)error ));
	return;
    }

    FT_Outline_Get_CBox(&ge->face->glyph->outline, &bbox);

    metrics = &ge->face->glyph->metrics;

    do_transform=(ge->do_rotate || ge->do_shear);

    /* how big is glyph */
    bitmap.width = F266Ceil(bbox.xMax) - F266Floor(bbox.xMin);
    bitmap.rows  = F266Ceil(bbox.yMax) - F266Floor(bbox.yMin);

    D(bug("BBox %f %f %f %f bitmap %d×%d\n",
	  bbox.xMin/64.0, bbox.yMin/64.0, bbox.xMax/64.0, bbox.yMax/64.0,
	  bitmap.width, bitmap.rows));

    if (glyph_8bits)
    {
	bitmap.pixel_mode = ft_pixel_mode_grays;
	bitmap.pitch = bitmap.width;
	bitmap.num_grays = 256;
	params.flags = ft_raster_flag_aa;
    }
    else
    {
	/* adjust bitmap to 4 byte (32 bit) alignment for width */
	bitmap.width = (bitmap.width + 31) & ~31;
	bitmap.pixel_mode = ft_pixel_mode_mono;
	bitmap.pitch = bitmap.width >> 3; /* byte count */
	params.flags = ft_raster_flag_default;
    }

    /*bitmap.bitmap= AllocPooled(ge->GlyphPool,(ULONG)bitmap.size); */
    
    /* stegerg: Always add 1 to allocation size to prevent debugging tools
                to warn about AllocVec(0) calls */
		
    bitmap.buffer = AllocVec(bitmap.rows*bitmap.pitch+1, MEMF_PUBLIC | MEMF_CLEAR);

    if (bitmap.buffer == NULL)
    {
	D(bug("bitmap allocation for %ld bytes failed\n", bitmap.rows*bitmap.pitch));
	return;
    }

    offset.x = -F266Floor(bbox.xMin) << 6;
    offset.y = -F266Floor(bbox.yMin) << 6;

    FT_Outline_Translate(&ge->face->glyph->outline, offset.x, offset.y);

    params.target = &bitmap;

    error = FT_Outline_Render(ge->engine, &ge->face->glyph->outline, &params);
    if(error)
    {
	D(bug("Error rendering glyph %ld  code = %ld.\n",
	      (LONG)ge->request_char, (LONG) error ));
	FreeVec(bitmap.buffer);
	return;
    }

    GMap->glm_BMModulo    = bitmap.pitch;
    GMap->glm_BMRows      = bitmap.rows;
    GMap->glm_BlackLeft   = 0;
    GMap->glm_BlackTop    = 0;
    GMap->glm_BlackWidth  = F266Ceil(bbox.xMax) - F266Floor(bbox.xMin);
    GMap->glm_BlackHeight = bitmap.rows;
    GMap->glm_XOrigin     = F266toFX(-bbox.xMin);
    GMap->glm_YOrigin     = F266toFX(bbox.yMax);
    GMap->glm_X0          = offset.x >> 6;//F266Ceil(-bbox.xMin);
    GMap->glm_Y0          = F266Ceil(bbox.yMax);
    GMap->glm_X1          = F266Ceil(ge->face->glyph->advance.x - bbox.xMin);
    GMap->glm_Y1          = F266Ceil(-ge->face->glyph->advance.y + bbox.yMax);

#if 0
    if (GMap->glm_X0 < 0)
    {
	D(bug("####### X0 < 0\n"));
	GMap->glm_X0 = 0;
    }
    if (GMap->glm_X0 > GMap->glm_BlackWidth)
    {
	D(bug("####### X0 > W\n"));
	GMap->glm_X0 = GMap->glm_BlackWidth;
    }
    if (GMap->glm_Y0 >= GMap->glm_BlackHeight)
    {
	D(bug("####### Y0 > H\n"));
	GMap->glm_Y0 = GMap->glm_BlackHeight -1;
	GMap->glm_Y1 = GMap->glm_BlackHeight -1;
    }
    if (GMap->glm_Y0 < 0)
    {
	D(bug("####### Y0 < 0\n"));
	GMap->glm_Y0 = 0;
    }
#endif

    /* horizontal pixels per EM square as 16.16 */
    GMap->glm_Width =
	(metrics->horiAdvance << 10) /
	(( ge->face->size->metrics.x_ppem
	   * ge->corrected_upem)
	 / ge->face->units_per_EM);


    GMap->glm_BitMap      = bitmap.buffer;

#if 0
    D(bug("render glyph %ld to %ld\n",(LONG)ge->glyph_code,(LONG)ge->request_char));

    //D(bug("  bbox.xMin=%lx  ",(LONG)ge->metrics.bbox.xMin));
    //D(bug("bbox.yMin=%lx  ",(LONG)ge->metrics.bbox.yMin));
    //D(bug("bbox.xMax=%lx  ",(LONG)ge->metrics.bbox.xMax));
    //D(bug("bbox.yMax=%lx\n",(LONG)ge->metrics.bbox.yMax));

    D(bug("   horiBearingX=%lx  ",(LONG)metrics->horiBearingX));
    D(bug("horiBearingY=%lx\n",(LONG)metrics->horiBearingY));
    
    D(bug("   vertBearingX=%lx  ",(LONG)metrics->vertBearingX));
    D(bug("vertBearingY=%lx\n",(LONG)metrics->vertBearingY));

    D(bug("   horiAdvance=%lx  ",(LONG)metrics->horiAdvance));
    D(bug("vertAdvance=%lx\n",(LONG)metrics->vertAdvance));
#endif
#if 1
    D(bug(" glm_BMModulo    = %lx\n",(LONG)GMap->glm_BMModulo));
    D(bug(" glm_BMRows      = %lx\n",(LONG)GMap->glm_BMRows));
    D(bug(" glm_BlackLeft   = %lx\n",(LONG)GMap->glm_BlackLeft));
    D(bug(" glm_BlackTop    = %lx\n",(LONG)GMap->glm_BlackTop));
    D(bug(" glm_BlackWidth  = %lx\n",(LONG)GMap->glm_BlackWidth));
    D(bug(" glm_BlackHeight = %lx\n",(LONG)GMap->glm_BlackHeight));
    
    D(bug(" glm_XOrigin     = %lx\n",(LONG)GMap->glm_XOrigin));
    D(bug(" glm_YOrigin     = %lx\n",(LONG)GMap->glm_YOrigin));
    D(bug(" glm_X0          = %lx\n",(LONG)GMap->glm_X0));
    D(bug(" glm_Y0          = %lx\n",(LONG)GMap->glm_Y0));

    D(bug(" glm_X1          = %lx\n",(LONG)GMap->glm_X1));
    D(bug(" glm_Y1          = %lx\n",(LONG)GMap->glm_Y1));
    D(bug(" glm_Width       = %lx\n",(LONG)GMap->glm_Width));
#endif
}


/* given an Amiga character, return a filled in GlyphMap */
struct GlyphMap *GetGlyph(FT_GlyphEngine *ge, int glyph_8bits)
{
    if (ge->instance_changed)
    {
	/* reset everything first */
	if(SetInstance(ge)!=OTERR_Success)
	    return NULL;
    }

    /*if(ge->cmap_index==NO_CMAP_SET)
     {
     //if(ChooseEncoding(ge)!=0)
     return NULL;
     }*/

#if 0
    if(ge->request_char>255) {			/* out of range */
	XeroGlyph(ge);
	return ge->GMap;
    }
#endif

    UnicodeToGlyphIndex(ge);
    if(ge->glyph_code)
    {	/* has code, try to render */
	/* first, get a GlyphMap structure to fill in */
	ge->GMap=AllocVec((ULONG)sizeof(struct GlyphMap),
			  MEMF_PUBLIC | MEMF_CLEAR);
	if(ge->GMap)
	    RenderGlyph(ge, glyph_8bits);
    }
    else
    {
	set_last_error(ge,OTERR_UnknownGlyph);
	return NULL;
    }

    /* odd ball glyph (includes SPACE, arrrgghhh) mimic bullet */
    if (ge->GMap->glm_Width == 0)
    {
	set_last_error(ge,OTERR_UnknownGlyph);
	FreeVec(ge->GMap);
	ge->GMap = NULL;
	return NULL;
    }

    return ge->GMap;
}
