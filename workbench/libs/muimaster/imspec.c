/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <graphics/gfxmacros.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"

#include "datatypescache.h"
#include "imspec.h"
#include "support.h"

#include "muimaster_intern.h"
#include "prefs.h"

extern struct Library *MUIMasterBase;

#ifdef _AROS
static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}
#endif

typedef enum {
    IST_MUICOLOR, /* one the MUI preset colors, mimic WB colors */
    IST_PATTERN,  /* a mix of the MUI preset colors, to get even more colors */
    IST_COLOR,    /* an arbitrary RGB color */
    IST_BITMAP,   /* a picture to tile in ackground */
    IST_VECTOR,   /* has code to draw */
    IST_EXTERNAL, /* small images for gadgets */
} ImageSpecType;

#define CHECKBOX_IMAGE 4

/* should really contain an union */
struct MUI_ImageSpec
{
    ImageSpecType type;
    UWORD flags;                  /* see MUI_ImageSpec_Flags */
    struct MUI_RenderInfo *mri;
    UBYTE muicolor;
    UBYTE pattern;

    ULONG r,g,b;
    LONG color;

    char *filename;
    struct dt_node *dt;

    LONG vectortype;
    void (*vector_draw)(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state);
};

enum MUI_ImageSpec_Flags { IMSPEC_REALIZED = (1<<0) /* struct is between _setup and _cleanup */ };

static void draw_thick_line(struct RastPort *rp,int x1, int y1, int x2, int y2)
{
    Move(rp,x1,y1);
    Draw(rp,x2,y2);
    Move(rp,x1+1,y1);
    Draw(rp,x2+1,y2);
}

#define VECTOR_DRAW_FUNC(x) (((void (*)(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)(x))))


#define SPACING 1
#define HSPACING 1
#define VSPACING 1

void arrowup_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    Move(rport, left + HSPACING + 1, top + height - 1 - VSPACING);
    Draw(rport, left + width - cx, top + VSPACING);
    Move(rport, left + HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + width - cx - 1, top + VSPACING);

    Move(rport, left + width - 1 - HSPACING - 1, top + height - 1 - VSPACING);
    Draw(rport, left + cx - 1, top + VSPACING);
    Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + cx, top + VSPACING);
}

void arrowdown_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cx;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    Move(rport, left + HSPACING + 1, top + VSPACING);
    Draw(rport, left + width - cx, top + height - 1 - VSPACING);
    Move(rport, left + HSPACING, top + VSPACING);
    Draw(rport, left + width - cx - 1, top + height - 1 - VSPACING);

    Move(rport, left + width - 1 - HSPACING - 1, top + VSPACING);
    Draw(rport, left + cx - 1, top + height - 1 - VSPACING);
    Move(rport, left + width - 1 - HSPACING, top + VSPACING);
    Draw(rport, left + cx, top + height - 1 - VSPACING);
}

void arrowleft_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    Move(rport, left + width - 1 - HSPACING, top + VSPACING + 1);
    Draw(rport, left + HSPACING, top + height - cy);
    Move(rport, left + width - 1 - HSPACING, top + VSPACING);
    Draw(rport, left + HSPACING, top + height - cy - 1);

    Move(rport, left + width - 1 - HSPACING, top + height - 1- VSPACING - 1);
    Draw(rport, left + HSPACING, top + cy - 1);
    Move(rport, left + width - 1 - HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + HSPACING, top + cy);
}

void arrowright_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int cy;
    struct RastPort *rport = mri->mri_RastPort;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cy = height / 2;

    Move(rport, left + HSPACING, top + VSPACING + 1);
    Draw(rport, left + width - 1 - HSPACING, top + height - cy);
    Move(rport, left + HSPACING, top + VSPACING);
    Draw(rport, left + width - 1 - HSPACING, top + height - cy - 1);

    Move(rport, left + HSPACING, top + height - 1- VSPACING - 1);
    Draw(rport, left + width - 1 - HSPACING, top + cy - 1);
    Move(rport, left + HSPACING, top + height - 1 - VSPACING);
    Draw(rport, left + width - 1 - HSPACING, top + cy);
}

void checkbox_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int h_spacing = width / 4;
    int v_spacing = height / 4;
    int bottom = top + height - 1;
    int right = left + width - 1;

    /* Draw checkmark (only if image is in selected state) */

    if (state)
    {
	left += h_spacing;right -= h_spacing;width -= h_spacing * 2;
	top += v_spacing;bottom -= v_spacing;height -= v_spacing * 2;

        SetAPen(mri->mri_RastPort, mri->mri_Pens[MPEN_TEXT]);

	draw_thick_line(mri->mri_RastPort, left, top + height / 3 , left, bottom);
	draw_thick_line(mri->mri_RastPort, left + 1, bottom, right - 1, top);
    }
}

void mx_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int bottom = top + height - 1;
    int right = left + width - 1;
    int col1;
    int col2;

    if (state)
    {
	col1 = MPEN_SHADOW;
	col2 = MPEN_SHINE;
    } else
    {
	col1 = MPEN_SHINE;
	col2 = MPEN_SHADOW;
    }

    /* Draw checkmark (only if image is in selected state) */

    SetAPen(rport, mri->mri_Pens[col1]);
    RectFill(rport, left + 3, top, right - 3, top);
    WritePixel(rport, left + 2, top + 1);
    RectFill(rport, left + 1, top + 2, left + 1, top + 3);
    RectFill(rport, left, top + 4, left, bottom - 4);
    RectFill(rport, left + 1, bottom - 3, left + 1, bottom - 2);
    WritePixel(rport, left + 2, bottom - 1);
	
    SetAPen(rport, mri->mri_Pens[col2]);
    WritePixel(rport, right - 2, top + 1);
    RectFill(rport, right - 1, top + 2, right - 1, top + 3);
    RectFill(rport, right, top + 4, right, bottom - 4);
    RectFill(rport, right - 1, bottom - 3, right - 1, bottom - 2);
    WritePixel(rport, right - 2, bottom - 1);
    RectFill(rport, left + 3, bottom, right - 3, bottom);
	
    if (state)
    {
	left += 3;right -= 3;width -= 6;
	top += 3;bottom -= 3;height -= 6;
	    
        SetAPen(rport, mri->mri_Pens[MPEN_FILL]);
	if ((width >= 5) && (height >= 5))
	{
	    RectFill(rport, left, top + 2, left, bottom - 2);
	    RectFill(rport, left + 1, top + 1, left + 1, bottom - 1);
	    RectFill(rport, left + 2, top, right - 2, bottom);
	    RectFill(rport, right - 1, top + 1, right - 1, bottom - 1);
	    RectFill(rport, right, top + 2, right, bottom - 2);
	} else {
	    RectFill(rport, left, top, right, bottom);
	}
    }
}

void cycle_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    struct RastPort *rport = mri->mri_RastPort;
    int bottom = top + height - 1;
    int right = left + width - 1;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    Move(rport,left,top+1);
    Draw(rport,left,bottom-1);
    Move(rport,left+1,top);
    Draw(rport,left+1,bottom);
    Draw(rport,right-7,bottom);
    Move(rport,right-7,bottom-1);
    Draw(rport,right-6,bottom-1);
    Move(rport,left+2,top);
    Draw(rport,right-7,top);
    Move(rport,right-7,top+1);
    Draw(rport,right-6,top+1);

    /* The small arrow */
    Move(rport,right - 6 - 3, top+2);
    Draw(rport,right - 7 + 3, top+2);
    Move(rport,right - 6 - 2, top+3);
    Draw(rport,right - 7 + 2, top+3);
    Move(rport,right - 6 - 1, top+4);
    Draw(rport,right - 7 + 1, top+4);

    /* The right bar */
    SetAPen(rport, mri->mri_Pens[MPEN_SHADOW]);
    Move(rport,right - 1, top);
    Draw(rport,right - 1, bottom);
    SetAPen(rport, mri->mri_Pens[MPEN_SHINE]);
    Move(rport,right, top);
    Draw(rport,right, bottom);
}

void popup_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom,cx;
    struct RastPort *rport = mri->mri_RastPort;

    height -= 3;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);

    cx = width / 2;

    Move(rport, left + HSPACING + 1, top + VSPACING);
    Draw(rport, left + width - cx, top + height - 1 - VSPACING);
    Move(rport, left + HSPACING, top + VSPACING);
    Draw(rport, left + width - cx - 1, top + height - 1 - VSPACING);

    Move(rport, left + width - 1 - HSPACING - 1, top + VSPACING);
    Draw(rport, left + cx - 1, top + height - 1 - VSPACING);
    Move(rport, left + width - 1 - HSPACING, top + VSPACING);
    Draw(rport, left + cx, top + height - 1 - VSPACING);

    bottom = top + height - 1 + 3;
    right = left + width - 1;
    Move(rport, left, bottom-2);
    Draw(rport, right, bottom-2);
    Move(rport, left, bottom-1);
    Draw(rport, right, bottom-1);
}

void popfile_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom;
    int edgex,edgey;
    struct RastPort *rport = mri->mri_RastPort;

    right = left + width - 1;
    bottom = top + height - 1;

    edgex = left + width * 5 / 8;
    edgey = top + height * 5 / 8;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);
    Move(rport, left, top);
    Draw(rport, left, bottom);
    Move(rport, left+1, top);
    Draw(rport, left+1, bottom);

    Move(rport, left, bottom);
    Draw(rport, right, bottom);
    Move(rport, left, bottom-1);
    Draw(rport, right, bottom-1);

    Move(rport, right, bottom-1);
    Draw(rport, right, edgey);
    Move(rport, right-1, bottom-1);
    Draw(rport, right-1, edgey);

    Move(rport, right, edgey-1);
    Draw(rport, edgex, edgey-1);
    Draw(rport, edgex, top);
    Draw(rport, left+2,top);
    Move(rport, left+2,top+1);
    Draw(rport, edgex-1,top+1);

    Move(rport, edgex+1, top);
    Draw(rport, right, edgey-1);

}

void popdrawer_draw(struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img, LONG left, LONG top, LONG width, LONG height, LONG state)
{
    int right,bottom;
    int halfx,halfy,quartery;
    struct RastPort *rport = mri->mri_RastPort;

    right = left + width - 1;
    bottom = top + height - 1;

    halfx = (left + right) / 2;
    halfy = (top + bottom) / 2;
    quartery = top + height / 4;

    SetAPen(rport, mri->mri_Pens[MPEN_TEXT]);
    Move(rport, left, quartery);
    Draw(rport, left, bottom);
    Move(rport, left+1, quartery);
    Draw(rport, left+1, bottom);
    Draw(rport, right, bottom);
    Draw(rport, right, halfy);
    Draw(rport, halfx, halfy);
    Draw(rport, halfx, quartery);
    Draw(rport, left+1,quartery);

    Move(rport, halfx, quartery-1);
    Draw(rport, halfx + 2, top);
    Draw(rport, right - 2, top);
    Draw(rport, right, quartery-1);
    Draw(rport, right, halfy);
}

struct vector_image
{
    int minwidth;
    int minheight;
    void *draw_func;
};

static struct vector_image vector_table[] =
{
    {10,8,arrowup_draw},
    {10,8,arrowdown_draw},
    {8,10,arrowleft_draw},
    {8,10,arrowright_draw},
    {16,10,checkbox_draw},
    {16,10,mx_draw},
    {15,8,cycle_draw},
    {10,11,popup_draw},
    {10,11,popfile_draw},
    {10,11,popdrawer_draw},
};

#define VECTOR_TABLE_ENTRIES (sizeof(vector_table)/sizeof(vector_table[0]))

const static UWORD pattern[] = {
    0x5555,
    0xaaaa,
};

const static MPenCouple patternPens[] = {
    {MPEN_SHADOW, MPEN_BACKGROUND},     /* MUII_SHADOWBACK */
    {MPEN_SHADOW, MPEN_FILL},           /* MUII_SHADOWFILL */
    {MPEN_SHADOW, MPEN_SHINE},          /* MUII_SHADOWSHINE */
    {MPEN_FILL, MPEN_BACKGROUND},       /* MUII_FILLBACK */
    {MPEN_FILL, MPEN_SHINE},            /* MUII_FILLSHINE */
    {MPEN_SHINE, MPEN_BACKGROUND},      /* MUII_SHINEBACK */
    {MPEN_FILL, MPEN_BACKGROUND},       /* MUII_FILLBACK2 */
    {MPEN_HALFSHINE, MPEN_BACKGROUND},  /* MUII_HSHINEBACK */
    {MPEN_HALFSHADOW, MPEN_BACKGROUND}, /* MUII_HSHADOWBACK */
    {MPEN_HALFSHINE, MPEN_SHINE},       /* MUII_HSHINESHINE */
    {MPEN_HALFSHADOW, MPEN_SHADOW},     /* MUII_HSHADOWSHADOW */
    {MPEN_MARK, MPEN_SHINE},            /* MUII_MARKSHINE */
    {MPEN_MARK, MPEN_HALFSHINE},        /* MUII_MARKHALFSHINE */
    {MPEN_MARK, MPEN_BACKGROUND},       /* MUII_MARKBACKGROUND */
};

#define PATTERN_COUNT (MUII_LASTPAT - MUII_BACKGROUND + 1)

static struct MUI_ImageSpec *get_config_imspec(LONG in, Object *obj)
{
    if (!obj) return NULL;

    if (in >= MUII_WindowBack && in <= MUII_ReadListBack)
    {
	return zune_image_spec_to_structure((IPTR)muiGlobalInfo(obj)->mgi_Prefs->imagespecs[in],NULL);
    }
    return NULL;
}

static struct MUI_ImageSpec *get_pattern_imspec(LONG in)
{
    struct MUI_ImageSpec *spec = NULL;

    if (in >= MUII_BACKGROUND && in <= MUII_FILL)
    {
	if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
	{
	    UWORD color;
	    spec->type = IST_MUICOLOR;
	    if (in == MUII_BACKGROUND) color = MPEN_BACKGROUND;
	    else if (in == MUII_SHADOW) color = MPEN_SHADOW;
	    else if (in == MUII_SHINE) color = MPEN_SHINE;
	    else color = MPEN_FILL;

	    spec->muicolor = color;
	}
    	return spec;
    }
    else if (in >= MUII_SHADOWBACK && in <= MUII_MARKBACKGROUND)
    {
	if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
	{
	    spec->type = IST_PATTERN;
	    spec->pattern = in - MUII_SHADOWBACK;
	}
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec *get_vector_imspec(LONG vect)
{
    struct MUI_ImageSpec *spec;

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
    {
	spec->type = IST_VECTOR;
	spec->vectortype = vect;

	if (vect >= 0 && vect < VECTOR_TABLE_ENTRIES)
	    spec->vector_draw = vector_table[vect].draw_func;
    }
    return spec;
}

static struct MUI_ImageSpec *get_color_imspec(ULONG r, ULONG g, ULONG b)
{
    struct MUI_ImageSpec *spec;
    if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
    {
	spec->type = IST_COLOR;
	spec->r = r;
	spec->g = g;
	spec->b = b;
	spec->color = -1;
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec *get_muicolor_imspec(ULONG muicolor)
{
    struct MUI_ImageSpec *spec;
    if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
    {
	spec->type = IST_MUICOLOR;
	spec->muicolor = muicolor;
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec *get_bitmap_imspec(char *filename)
{
    struct MUI_ImageSpec *spec;
    if ((spec = mui_alloc_struct(struct MUI_ImageSpec)))
    {
	spec->type = IST_BITMAP;
	spec->filename = StrDup(filename);
	D(bug("get_bitmap_imspec(%s): spec=%lx, fname=%lx\n", spec, spec->filename));
	return spec;
    }
    return NULL;
}

/**************************************************************************
 Duplicates a image spec. In in may be one of the MUII_#? identifiers
 (but it will always return a string).
 The returned string must be freed with zune_image_spec_free() because
 in the future it might be that the MUII_#? stuff is not converted to
 a string
**************************************************************************/
char *zune_image_spec_duplicate(IPTR in)
{
    char *spec;
    char spec_buf[20];

    if (in >= MUII_WindowBack && in < MUII_BACKGROUND)
    {
	sprintf(spec_buf,"6:%ld",in);
	spec = spec_buf;
    } else
    {
	if (in >= MUII_BACKGROUND && in < MUII_LASTPAT)
	{
	    sprintf(spec_buf,"0:%ld",in);
	    spec = spec_buf;
	} else spec = (char*)in;
    }

    return StrDup(spec);
}

/**************************************************************************
 Use this function to free the zune_image_spec_duplicate() result
**************************************************************************/
void zune_image_spec_free(char *spec)
{
    if (spec) FreeVec(spec);
}

/**************************************************************************
 Create a image spec. obj is a AreaObject. It is used to access the config
 data.

 TODO: merge this with zune_imspec_setup() because this function should
 be called in MUIM_Setup (configdata)
**************************************************************************/
struct MUI_ImageSpec *zune_image_spec_to_structure(IPTR in, Object *obj)
{
    char *s;

    if (in >= MUII_WindowBack && in <= MUII_ReadListBack)
	return get_config_imspec(in,obj);

    if (in >= MUII_BACKGROUND && in <= MUII_MARKBACKGROUND)
	return get_pattern_imspec(in);

    s = (char*)in;

    switch (*s)
    {
	case	'0':
		{
		    LONG pat;
             	    StrToLong(s+2,&pat);
             	    return get_pattern_imspec(pat);
		}

	case	'1':
		{
		    LONG vect;
		    StrToLong(s+2,&vect);
		    return get_vector_imspec(vect);
		}
		break;

	case	'2':
	    	s += 2;	
		if (s[0] == 'm')
		{
		    LONG muicolor;
		    
		    s++;
		    
		    StrToLong(s, &muicolor);
		    return get_muicolor_imspec(muicolor);
		}
		else
		{
		    ULONG r,g,b;
		    r = strtoul(s,&s, 16);
		    s++;
		    g = strtoul(s,&s, 16);
		    s++;
		    b = strtoul(s,&s, 16);
		    return get_color_imspec(r,g,b);
		}

	case	'5':
		return get_bitmap_imspec(s+2);

	case    '6':
		{
		    LONG img;
             	    StrToLong(s+2,&img);

		    if (img >= MUII_WindowBack && img <= MUII_ReadListBack)
			return get_config_imspec(img,obj);
	        }
	        break;
    }
    return NULL;
}

#if 0
static struct MUI_ImageSpec *zune_imspec_copy(struct MUI_ImageSpec *spec)
{
    struct MUI_ImageSpec *nspec;
    
    if (!spec) return NULL;
    
    nspec = mui_alloc_struct(struct MUI_ImageSpec);
    if (nspec) memcpy(nspec,spec,sizeof(struct MUI_ImageSpec));
    return nspec;
}
#endif

void zune_imspec_free(struct MUI_ImageSpec *spec)
{
    D(bug("zune_imspec_free(0x%lx)\n", spec));
    if (!spec) return;
    if (spec->flags & IMSPEC_REALIZED)
    {
	D(bug("zune_imspec_free(0x%lx) : cleanup, with 0x%lx\n", spec, spec->mri));
	zune_imspec_cleanup(&spec, spec->mri);
    }
    if (spec->type == IST_BITMAP)
    {
/*  	  D(bug("zune_imspec_free(0x%lx): filename FreeVec(0x%lx)\n", spec, spec->filename)); */
	FreeVec(spec->filename);
    }
/*      D(bug("zune_imspec_free(0x%lx): FreeVec()\n", spec)); */
    mui_free(spec);
}

void zune_imspec_setup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri)
{
    if (!spec || !(*spec)) return;
    if ((*spec)->flags & IMSPEC_REALIZED)
	zune_imspec_cleanup(spec, mri);
    (*spec)->mri = mri;
    switch ((*spec)->type)
    {
	case	IST_COLOR:
		(*spec)->color = ObtainBestPenA(mri->mri_Screen->ViewPort.ColorMap, (*spec)->r, (*spec)->g, (*spec)->b, NULL);
		break;

	case	IST_BITMAP:
		if ((*spec)->filename)
		    (*spec)->dt = dt_load_picture((*spec)->filename,mri->mri_Screen);
		break;
	case    IST_MUICOLOR:
	case	IST_PATTERN:
	case	IST_VECTOR:
	        break;
		/* IST_EXTERNAL is to be implemented */
    }
    (*spec)->flags |= IMSPEC_REALIZED;
}

void zune_imspec_cleanup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri)
{
    if (!spec || !(*spec)) return;
    D(bug("zune_imspec_cleanup(0x%lx)\n", *spec));
    if (!((*spec)->flags & IMSPEC_REALIZED))
	return;
    switch ((*spec)->type)
    {
	case	IST_COLOR:
		if ((*spec)->color != -1)
		{
		    ReleasePen(mri->mri_Screen->ViewPort.ColorMap, (*spec)->color);
		    (*spec)->color = -1;
		}
		break;

	case	IST_BITMAP:
		if ((*spec)->dt)
		{
		    dt_dispose_picture((*spec)->dt);
		    (*spec)->dt = NULL;
		}
		break;
	case    IST_MUICOLOR:
	case	IST_PATTERN:
	case	IST_VECTOR:
	        break;
    }
    (*spec)->flags &= ~IMSPEC_REALIZED;
}

/* This is very very uneligant but only a test */
int zune_imspec_get_minwidth(struct MUI_ImageSpec *spec)
{
    if (!spec)
    {
    	return 0;
    }
    
    if (spec->type == IST_VECTOR)// && spec->vectortype == 4)
    {
	if (spec->vectortype >= 0 && spec->vectortype < VECTOR_TABLE_ENTRIES)
	{
	    return vector_table[spec->vectortype].minwidth;
	}
    }
    return 0;
}

int zune_imspec_get_minheight(struct MUI_ImageSpec *spec)
{
    if (!spec)
    {
    	return 0;
    }
    
    if (spec->type == IST_VECTOR)// && spec->vectortype == 4)
    {
	if (spec->vectortype >= 0 && spec->vectortype < VECTOR_TABLE_ENTRIES)
	{
	    return vector_table[spec->vectortype].minheight;
	}
    }
    return 0;
}

void zune_imspec_show(struct MUI_ImageSpec *spec, Object *obj)
{
}

void zune_imspec_hide(struct MUI_ImageSpec *spec)
{
}

void zune_draw_image (struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG flags)
{
    LONG right = left + width - 1;
    LONG bottom = top + height - 1;
    struct RastPort *rp = mri->mri_RastPort;
    struct MUI_ImageSpec def;

    if (!img)
    {
    	def.type = IST_MUICOLOR;
    	def.muicolor = MPEN_BACKGROUND;
    	img = &def;
    }

    if (img->type == IST_BITMAP && !img->dt)
    {
    	def.type = IST_MUICOLOR;
    	def.muicolor = MPEN_BACKGROUND;
    	img = &def;
    }

    switch (img->type)
    {
	case	IST_MUICOLOR:
		{
		    LONG pen = mri->mri_Pens[img->muicolor];
		    SetAPen(mri->mri_RastPort, pen);
		    RectFill(mri->mri_RastPort, left, top, right, bottom);
		}
		break;

	case	IST_PATTERN:
		{
		    LONG fg = mri->mri_Pens[patternPens[img->pattern].fg];
		    LONG bg = mri->mri_Pens[patternPens[img->pattern].bg];
		    SetDrMd(rp,JAM2);
		    SetAPen(rp,fg);
		    SetBPen(rp,bg);
		    SetAfPt(rp,pattern,1);
		    RectFill(rp, left, top, right, bottom);
		    SetAfPt(rp,NULL,0);
		}
		break;

	case	IST_COLOR:
		{
		    LONG pen = img->color;
		    SetAPen(mri->mri_RastPort, pen);
		    RectFill(mri->mri_RastPort, left, top, right, bottom);
		}
		break;

	case	IST_BITMAP:
		if (img->dt)
		{
		    dt_put_on_rastport_tiled(img->dt, mri->mri_RastPort, left, top, right, bottom, xoffset - left, yoffset - top);
		}
		break;

	case	IST_VECTOR:
		if (img->vector_draw)
		{
		    img->vector_draw(mri, img, left, top, width, height,!!(flags & IMSPECF_SELECTED));
		}
		break;
    }
}

void zune_imspec_set_scaled_size (struct MUI_ImageSpec *img, LONG w, LONG h)
{
}

