/*
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/* This is the implementation of a MUI-like image engine
 * (see MUIA_Image_Spec for more information about MUI image specs)
 * Their external form is a string "<type>:<parameters>"
 * with type being a single char. See zune_image_spec_to_structure().
 *
 * Basically an ImageSpec can be anything which can be displayed:
 * gfx datas, drawing code, ...
 * See ImageSpecType for the known types.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <graphics/gfxmacros.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

//#define MYDEBUG 1
#include "debug.h"

#include "mui.h"

#include "datatypescache.h"
#include "imspec.h"
#include "support.h"

#include "muimaster_intern.h"
#include "prefs.h"
#include "penspec.h"
#include "imspec_intern.h"

extern struct Library *MUIMasterBase;

static struct MUI_ImageSpec_intern *get_brush_imspec(CONST_STRPTR filename);

const static UWORD gridpattern1[] = {
    0x5555,
    0xaaaa,
};

const static UWORD gridpattern2[] = {
    0x4444,
    0x1111,
};

typedef struct {
    MPen bg;
    MPen fg;
    const UWORD *pattern;
} MPattern;

const static MPattern patternPens[] = {
    { MPEN_SHADOW,     MPEN_BACKGROUND, gridpattern1 }, /* MUII_SHADOWBACK     */
    { MPEN_SHADOW,     MPEN_FILL      , gridpattern1 }, /* MUII_SHADOWFILL     */
    { MPEN_SHADOW,     MPEN_SHINE     , gridpattern1 }, /* MUII_SHADOWSHINE    */
    { MPEN_FILL,       MPEN_BACKGROUND, gridpattern1 }, /* MUII_FILLBACK       */
    { MPEN_FILL,       MPEN_SHINE     , gridpattern1 }, /* MUII_FILLSHINE      */
    { MPEN_SHINE,      MPEN_BACKGROUND, gridpattern1 }, /* MUII_SHINEBACK      */
    { MPEN_FILL,       MPEN_BACKGROUND, gridpattern2 }, /* MUII_FILLBACK2      */
    { MPEN_HALFSHINE,  MPEN_BACKGROUND, gridpattern1 }, /* MUII_HSHINEBACK     */
    { MPEN_HALFSHADOW, MPEN_BACKGROUND, gridpattern1 }, /* MUII_HSHADOWBACK    */
    { MPEN_HALFSHINE,  MPEN_SHINE     , gridpattern1 }, /* MUII_HSHINESHINE    */
    { MPEN_HALFSHADOW, MPEN_SHADOW    , gridpattern1 }, /* MUII_HSHADOWSHADOW  */
    { MPEN_MARK,       MPEN_SHINE     , gridpattern1 }, /* MUII_MARKSHINE      */
    { MPEN_MARK,       MPEN_HALFSHINE , gridpattern1 }, /* MUII_MARKHALFSHINE  */
    { MPEN_MARK,       MPEN_BACKGROUND, gridpattern1 }, /* MUII_MARKBACKGROUND */
};

#define PATTERN_COUNT (MUII_LASTPAT - MUII_BACKGROUND + 1)

static struct MUI_ImageSpec_intern *get_pattern_imspec(LONG in)
{
    struct MUI_ImageSpec_intern *spec = NULL;

    if (in >= MUII_BACKGROUND && in <= MUII_FILL)
    {
	if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
	{
	    UWORD color;
	    if (in == MUII_BACKGROUND) color = MPEN_BACKGROUND;
	    else if (in == MUII_SHADOW) color = MPEN_SHADOW;
	    else if (in == MUII_SHINE) color = MPEN_SHINE;
	    else color = MPEN_FILL;

	    spec->type = IST_COLOR;
	    zune_penspec_fill_muipen(&spec->u.penspec, color);
	}
    	return spec;
    }
    else if (in >= MUII_SHADOWBACK && in <= MUII_MARKBACKGROUND)
    {
	if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
	{
	    spec->type = IST_PATTERN;
	    spec->u.pattern = in - MUII_SHADOWBACK;
	}
    	return spec;
    }
    return NULL;
}


static struct MUI_ImageSpec_intern *get_pen_imspec(CONST_STRPTR str)
{
    struct MUI_ImageSpec_intern *spec;

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	if (!zune_pen_string_to_intern(str, &spec->u.penspec))
	{
	    D(bug("*** zune_pen_string_to_intern failed\n"));
	    mui_free(spec);
	    return NULL;
	}
	spec->type = IST_COLOR;
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec_intern *get_scaled_gradient_imspec(CONST_STRPTR str)
{
    struct MUI_ImageSpec_intern *spec;

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	if (!zune_gradient_string_to_intern(str, spec))
	{
	    D(bug("*** zune_gradient_string_to_intern failed\n"));
	    mui_free(spec);
	    return NULL;
	}
	spec->type = IST_SCALED_GRADIENT;
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec_intern *get_tiled_gradient_imspec(CONST_STRPTR str)
{
    struct MUI_ImageSpec_intern *spec;

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	if (!zune_gradient_string_to_intern(str, spec))
	{
	    D(bug("*** zune_gradient_string_to_intern failed\n"));
	    mui_free(spec);
	    return NULL;
	}
	spec->type = IST_TILED_GRADIENT;
    	return spec;
    }
    return NULL;
}

static struct MUI_ImageSpec_intern *get_boopsi_imspec(CONST_STRPTR filename)
{
    struct MUI_ImageSpec_intern *spec;

    if (!filename)
	return NULL;
    if (!strstr(filename, ".image"))
	return get_brush_imspec(filename);

    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	spec->u.boopsi.filename = StrDup(filename);
	if (!spec->u.boopsi.filename)
	    return NULL;
	spec->u.boopsi.obj = NULL;
	spec->type = IST_BOOPSI;
	return spec;
    }
    return NULL;
}


static struct MUI_ImageSpec_intern *get_brush_imspec(CONST_STRPTR filename)
{
    struct MUI_ImageSpec_intern *spec;
    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	size_t last_idx;
	spec->u.brush.filename[0] = StrDup(filename);
	if (!spec->u.brush.filename[0])
	    return NULL;
	last_idx = strlen(spec->u.brush.filename[0]) - 1;
	if (spec->u.brush.filename[0][last_idx] == '0')
	{
	    char *tmpstr;
	    tmpstr = StrDup(filename);
	    if (!tmpstr)
	    {
		FreeVec((APTR)spec->u.brush.filename[0]);
		return NULL;
	    }
	    tmpstr[last_idx] = '1';
	    spec->u.brush.filename[1] = tmpstr;
	}
	spec->u.brush.dt[0] = NULL;
	spec->u.brush.dt[1] = NULL;
	spec->type = IST_BRUSH;
	return spec;
    }
    return NULL;
}


static struct MUI_ImageSpec_intern *get_bitmap_imspec(CONST_STRPTR filename)
{
    struct MUI_ImageSpec_intern *spec;
    if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
    {
	spec->u.bitmap.filename = StrDup(filename);
	if (!spec->u.bitmap.filename)
	    return NULL;
	spec->u.bitmap.dt = NULL;
	spec->type = IST_BITMAP;
	return spec;
    }
    return NULL;
}


static struct MUI_ImageSpec_intern *get_config_imspec(LONG img)
{
    if ((img >= MUII_WindowBack) && (img <= MUII_ReadListBack))
    {
	struct MUI_ImageSpec_intern *spec;
	if ((spec = mui_alloc_struct(struct MUI_ImageSpec_intern)))
	{
	    spec->u.cfg.muiimg = img;
	    spec->type = IST_CONFIG;
	    return spec;
	}
    }
    return NULL;
}


#ifdef MYDEBUG
static const char *zune_imspec_to_string(struct MUI_ImageSpec_intern *spec)
{
    static char buf[64];

    if (!spec)
    {
	buf[0] = 0;
	return buf;
    }
    switch (spec->type)
    {
	case IST_PATTERN:
	    sprintf(buf, "0:%ld", spec->u.pattern);
	    break;

	case IST_VECTOR:
	    sprintf(buf, "1:%ld", spec->u.vect.type);
	    break;

	case IST_COLOR:
	    zune_pen_intern_to_spec(&spec->u.penspec, (struct MUI_PenSpec *)buf);
	    break;

	case IST_BOOPSI:
	    sprintf(buf, "3:%s", spec->u.boopsi.filename);
	    break;

	case IST_BRUSH: /* this is really 3: too */
	    sprintf(buf, "3:%s", spec->u.brush.filename[0]);
	    break;

	case IST_BITMAP:
	    sprintf(buf, "5:%s", spec->u.bitmap.filename);
	    break;

	case IST_CONFIG:
	    sprintf(buf, "6:%ld", spec->u.cfg.muiimg);
	    break;

        case IST_SCALED_GRADIENT:
	    zune_scaled_gradient_intern_to_string(spec, buf);
	    break;

        case IST_TILED_GRADIENT:
	    zune_tiled_gradient_intern_to_string(spec, buf);
	    break;
    }
    return buf;
}
#endif

/**************************************************************************
 Create a image spec from a string or a magic value.
 in : contains magic or string
 obj: is a AreaObject. It is used to access the config data.

 TODO: merge this with zune_imspec_setup() because this function should
 be called in MUIM_Setup (configdata)
**************************************************************************/
static struct MUI_ImageSpec_intern *zune_image_spec_to_structure(IPTR in)
{
    struct MUI_ImageSpec_intern *spec = NULL;
    CONST_STRPTR s;

    if (in >= MUII_WindowBack && in <= MUII_ReadListBack)
    {
	D(bug("zune_image_spec_to_structure [config] : in=%ld\n", in));
	spec = get_config_imspec(in);
    }
    else if (in >= MUII_BACKGROUND && in <= MUII_MARKBACKGROUND)
    {
	D(bug("zune_image_spec_to_structure [pattern] : in=%ld\n", in));
	spec = get_pattern_imspec(in);
    }
    else
    {
	s = (CONST_STRPTR)in;
	D(bug("zune_image_spec_to_structure [string] : in=%s\n", s));

	switch (*s)
	{
	    case '0': /* builtin pattern */
	    {
		LONG pat;
		StrToLong(s+2, &pat);
		spec = get_pattern_imspec(pat);
		break;
	    }

	    case '1': /* builtin standard image, obsoleted by 6: */
	    {
		LONG vect;
		StrToLong(s+2, &vect);
		spec = zune_imspec_create_vector(vect);
		break;
	    }

	    case '2': /* a penspec */
		spec = get_pen_imspec(s+2);
		D(bug("zune_image_spec_to_structure : penspec %lx\n", &spec->u.penspec));
		break;

	    case '3': /* BOOPSI image class name */
		spec = get_boopsi_imspec(s+2);
		break;

	    case '4': /* external MUI brush name */
		spec = get_brush_imspec(s+2);
		break;

	    case '5': /* external bitmap loaded with datatypes */
		spec = get_bitmap_imspec(s+2);
		break;

	    case '6': /* preconfigured image or background */
	    {
		LONG img;
		StrToLong(s+2, &img);

		if (img >= MUII_WindowBack && img <= MUII_ReadListBack)
		    spec = get_config_imspec(img);
		break;
	    }

            case '7': /* scaled gradient */
		spec = get_scaled_gradient_imspec(s+2);
		break;

            case '8': /* tiled gradient */
		spec = get_tiled_gradient_imspec(s+2);
		break;

	} /* switch(*s) */
    }
    D(bug("zune_image_spec_to_structure : out=0x%lx [%s]\n",
	  spec, zune_imspec_to_string(spec)));
    return spec;
}

#if 0
static struct MUI_ImageSpec_intern *zune_imspec_copy(struct MUI_ImageSpec_intern *spec)
{
    struct MUI_ImageSpec_intern *nspec;

    if (!spec) return NULL;

    nspec = mui_alloc_struct(struct MUI_ImageSpec_intern);
    if (nspec) memcpy(nspec,spec,sizeof(struct MUI_ImageSpec_intern));
    return nspec;
}
#endif



static void zune_imspec_free(struct MUI_ImageSpec_intern *spec)
{
    if (!spec)
	return;
    D(bug("zune_imspec_free(0x%lx) [%s]\n",
	   spec, zune_imspec_to_string(spec)));

    switch (spec->type)
    {
	case IST_BOOPSI:
	    if (spec->u.boopsi.filename)
		FreeVec((APTR)spec->u.boopsi.filename);
	    break;

	case IST_BRUSH:
	    if (spec->u.brush.filename[0])
		FreeVec((APTR)spec->u.brush.filename[0]);
	    if (spec->u.brush.filename[1])
		FreeVec((APTR)spec->u.brush.filename[1]);
	    break;

	case IST_BITMAP:
	    if (spec->u.bitmap.filename)
		FreeVec((APTR)spec->u.bitmap.filename);
	    break;

	default:
	    break;
    }

    mui_free(spec);
}

struct MUI_ImageSpec_intern *zune_imspec_setup(IPTR s, struct MUI_RenderInfo *mri)
{
    struct MUI_ImageSpec_intern *spec;

    if (!mri)
    {
	D(bug("zune_imspec_setup: param error: mri=%p\n", mri));
	return NULL;
    }

    spec = zune_image_spec_to_structure(s);

    D(bug("zune_imspec_setup(%lx) [%s]\n",
	  spec, zune_imspec_to_string(spec)));
    if (!spec)
	return NULL;

    switch (spec->type)
    {
	case IST_PATTERN:
	    break;

	case IST_VECTOR:
	    break;

	case IST_COLOR:
	    zune_penspec_setup(&spec->u.penspec, mri);
	    break;

	case IST_BOOPSI:
	    break;

	case IST_BRUSH:
	{
	    int i;

	    for (i = 0; i < 2; i++)
	    {
		if (spec->u.brush.filename[i])
		{
                    spec->u.brush.dt[i] = dt_load_picture
		    (
			spec->u.brush.filename[i], mri->mri_Screen
		    );

		    if (!spec->u.brush.dt[i] &&  !strchr(spec->u.brush.filename[i], ':'))
		    {
 		        int size;
			STRPTR fullpath;

		        size = strlen(IMSPEC_EXTERNAL_PREFIX)
			    + strlen(spec->u.brush.filename[i]) + 1;
		        fullpath = (STRPTR)AllocVec(size, MEMF_ANY);

		        if (fullpath != NULL)
		        {
			    strcpy(fullpath, IMSPEC_EXTERNAL_PREFIX);
			    strcat(fullpath, spec->u.brush.filename[i]);
			    fullpath[size - 1] = 0;
                            spec->u.brush.dt[i] = dt_load_picture
			    (
			        fullpath, mri->mri_Screen
			    );
			    FreeVec(fullpath);
		        }
		    }
		}
		else
		{
		    spec->u.brush.dt[i] = spec->u.brush.dt[0];
		}
	    }
	}
	break;

	case IST_BITMAP:
	    if (spec->u.bitmap.filename)
	    {
		spec->u.bitmap.dt = dt_load_picture
		(
		    spec->u.bitmap.filename, mri->mri_Screen
		);
	    }
	    break;

	case IST_CONFIG:
	{
	    Object *win = mri->mri_WindowObject;
	    struct ZunePrefsNew *prefs =  muiGlobalInfo(win)->mgi_Prefs;
	    /* potential for deadloop if Zune prefs images contain a 6: */
	    CONST_STRPTR spec_desc = prefs->imagespecs[spec->u.cfg.muiimg];
	    zune_imspec_free(spec);
	    spec = NULL;

	    if (spec_desc && (spec_desc[0] == '6'))
	    {
		D(bug("*** zune_imspec_setup (%s recursive config)\n",
		      zune_imspec_to_string(spec)));
	    }
	    else
	    {
		spec = zune_imspec_setup((IPTR)spec_desc, mri);
	    }
	    break;
	}

        case IST_SCALED_GRADIENT:
        case IST_TILED_GRADIENT:
	    zune_gradientspec_setup(spec, mri);
            break;
    }
    return spec;
}

/* bug : never called in textengine, fix this */
void zune_imspec_cleanup(struct MUI_ImageSpec_intern *spec)
{
    if (!spec)
	return;

    D(bug("zune_imspec_cleanup(0x%lx) [%s]\n",
	  spec, zune_imspec_to_string(spec)));

    switch (spec->type)
    {
	case IST_PATTERN:
	    break;

	case IST_VECTOR:
	    break;

	case IST_COLOR:
	    zune_penspec_cleanup(&spec->u.penspec);
	    break;

	case IST_BOOPSI:
	    break;

	case IST_BRUSH:
	{
	    int i;

	    for (i = 0; i < 2; i++)
	    {
		if (spec->u.brush.filename[i])
		{
		    dt_dispose_picture(spec->u.brush.dt[i]);
		}
		spec->u.brush.dt[i] = NULL;
	    }
	    break;
	}
	case IST_BITMAP:
	    if (spec->u.bitmap.dt)
	    {
		dt_dispose_picture(spec->u.bitmap.dt);
		spec->u.bitmap.dt = NULL;
	    }
	    break;

	case IST_CONFIG:
	    D(bug("*** zune_imspec_cleanup : IST_CONFIG\n"));
	    break;

        case IST_SCALED_GRADIENT:
        case IST_TILED_GRADIENT:
	    zune_gradientspec_cleanup(spec);
            break;

    }

    zune_imspec_free(spec);
}


BOOL zune_imspec_askminmax(struct MUI_ImageSpec_intern *spec, struct MUI_MinMax *minmax)
{
    if ((!spec) || (!minmax))
	return FALSE;

    switch (spec->type)
    {
	case IST_PATTERN:
        case IST_SCALED_GRADIENT:
        case IST_TILED_GRADIENT:
	case IST_COLOR:
	    minmax->MinWidth = 3;
	    minmax->MinHeight = 3;
	    minmax->DefWidth = 8;
	    minmax->DefHeight = 8;
	    minmax->MaxWidth = MUI_MAXMAX;
	    minmax->MaxHeight = MUI_MAXMAX;
	    break;

	case IST_VECTOR:
	    return zune_imspec_vector_get_minmax(spec, minmax);
	    break;

	case IST_BOOPSI:
	    /* ??? */
	    break;

	case IST_BRUSH:
	    if (spec->u.brush.dt[0])
	    {
		minmax->MinWidth = dt_width(spec->u.brush.dt[0]);
		minmax->MinHeight = dt_height(spec->u.brush.dt[0]);
		minmax->DefWidth = minmax->MinWidth;
		minmax->DefHeight = minmax->MinHeight;
		minmax->MaxWidth = minmax->MinWidth;
		minmax->MaxHeight = minmax->MinHeight;
	    }
	    else
	    {
		minmax->MinWidth = 3;
		minmax->MinHeight = 3;
		minmax->DefWidth = 8;
		minmax->DefHeight = 8;
		minmax->MaxWidth = MUI_MAXMAX;
		minmax->MaxHeight = MUI_MAXMAX;
		return FALSE;
	    }
	    break;

	case IST_BITMAP:
	    minmax->MinWidth = 3;
	    minmax->MinHeight = 3;
	    minmax->DefWidth = 8;
	    minmax->DefHeight = 8;
	    minmax->MaxWidth = MUI_MAXMAX;
	    minmax->MaxHeight = MUI_MAXMAX;
	    if (!spec->u.bitmap.dt)
		return FALSE;
	    break;

	case IST_CONFIG:
	    D(bug("*** zune_imspec_askminmax : IST_CONFIG\n"));
	    break;
    }
    return TRUE;
}


void zune_imspec_show(struct MUI_ImageSpec_intern *spec, Object *obj)
{
    if ((!spec) || (!obj))
	return;

    D(bug("zune_imspec_show(0x%lx) [%s]\n", spec,
	  zune_imspec_to_string(spec)));

    /* scaled gradient generation made here */
    switch (spec->type)
    {
	case IST_CONFIG:
	    D(bug("*** zune_imspec_show : IST_CONFIG\n"));
	    break;

        case IST_SCALED_GRADIENT:
        case IST_TILED_GRADIENT:
            spec->u.gradient.obj = obj;
            break;
            
	default:
	    break;
    }
}


void zune_imspec_hide(struct MUI_ImageSpec_intern *spec)
{
    if (!spec)
	return;

    D(bug("zune_imspec_hide(0x%lx) [%s]\n", spec,
	  zune_imspec_to_string(spec)));

    switch (spec->type)
    {
	case IST_CONFIG:
	    D(bug("*** zune_imspec_hide : IST_CONFIG\n"));
	    break;
	default:
	    break;
    }
}


void zune_imspec_draw (struct MUI_ImageSpec_intern *spec, struct MUI_RenderInfo *mri,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG state)
{
    LONG right = left + width - 1;
    LONG bottom = top + height - 1;
    struct RastPort *rp = mri->mri_RastPort;
    struct MUI_ImageSpec_intern def;

    if (!spec)
    {
	D(bug("*** zune_imspec_draw called on null imspec\n"));
	return;
    }

    if ((spec->type == IST_BITMAP && !spec->u.bitmap.dt)
	|| (spec->type == IST_BRUSH && !spec->u.brush.dt[0]))
    {
    	def.type = IST_COLOR;
	zune_penspec_fill_muipen(&def.u.penspec, MPEN_BACKGROUND);
    	spec = &def;
    }

    switch (spec->type)
    {
	case IST_PATTERN:
	{
	    LONG fg = mri->mri_Pens[patternPens[spec->u.pattern].fg];
	    LONG bg = mri->mri_Pens[patternPens[spec->u.pattern].bg];
	    SetDrMd(rp, JAM2);
	    SetAPen(rp, fg);
	    SetBPen(rp, bg);
	    SetAfPt(rp, patternPens[spec->u.pattern].pattern, 1);
	    RectFill(rp, left, top, right, bottom);
	    SetAfPt(rp, NULL, 0);
	}
	break;

	case IST_VECTOR:
	    if (spec->u.vect.draw)
	    {
		spec->u.vect.draw(mri, left, top, width, height, state);
	    }
	    break;

	case IST_COLOR:
	    zune_penspec_draw(&spec->u.penspec, mri, left, top, right, bottom);
	    break;

	case IST_BOOPSI:
	    break;

	case IST_BRUSH:
	    if (state < 0 || state > 1)
		state = 0;
	    if (spec->u.brush.dt[state])
	    {
		dt_put_on_rastport(spec->u.brush.dt[state], mri->mri_RastPort,
					 left, top);
/*  		dt_put_on_rastport_tiled(spec->u.brush.dt[state], mri->mri_RastPort, */
/*  					 left, top, right, bottom, */
/*  					 xoffset - left, yoffset - top); */
	    }
	    break;

	case IST_BITMAP:
	    if (spec->u.bitmap.dt)
	    {
		dt_put_on_rastport_tiled(spec->u.bitmap.dt, mri->mri_RastPort,
					 left, top, right, bottom,
					 xoffset - left, yoffset - top);
	    }
	    break;

	case IST_CONFIG:
	    D(bug("*** zune_imspec_draw : IST_CONFIG\n"));
	    break;

        case IST_SCALED_GRADIENT:
            zune_gradient_draw(spec, mri, left, top, right, bottom, 0, 0);
            break;

        case IST_TILED_GRADIENT:
            zune_gradient_draw(spec, mri, left, top, right, bottom, xoffset, yoffset);
            break;
    }
}

/**************************************************************************
 Duplicates a image spec. In in may be one of the MUII_#? identifiers
 (but it will always return a string).
 The returned string must be freed with zune_image_spec_free() because
 in the future it might be that the MUII_#? stuff is not converted to
 a string
**************************************************************************/
STRPTR zune_image_spec_duplicate(IPTR in)
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
void zune_image_spec_free(CONST_STRPTR spec)
{
    if (spec) FreeVec((APTR)spec);
}
