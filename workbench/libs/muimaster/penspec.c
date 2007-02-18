/*
    Copyright  2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/graphics.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "penspec.h"
#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/* From ASCII to internal representation */
BOOL zune_pen_spec_to_intern (const struct MUI_PenSpec *spec,
			      struct MUI_PenSpec_intern *intern)
{
    if (!spec || !intern)
	return FALSE;

    memset(intern, 0, sizeof(*intern));

    D(bug("zune_pen_spec_to_intern: parsing %s\n", spec->ps_buf));
    switch (spec->ps_buf[0])
    {
	case 'm':
	    if (!StrToLong(spec->ps_buf + 1, &intern->p_mui))
		return FALSE;
	    intern->p_type = PST_MUI;
	    break;

	case 'p':
	    if (!StrToLong(spec->ps_buf + 1, &intern->p_cmap))
		return FALSE;
	    intern->p_type = PST_CMAP;
	    break;

	case 's':
	    if (!StrToLong(spec->ps_buf + 1, &intern->p_sys))
		return FALSE;
	    intern->p_type = PST_SYS;
	    break;

	case 'r':
	default:
	{
	    const char *s;
	    const char *t;

	    s = spec->ps_buf;
	    if (*s == 'r')
	    {
		s++;
	    }
	    t = s;
	    intern->p_rgb.red = strtoul(s, (char **)&s, 16);
	    if (s == t)
		return FALSE;

	    s++;
	    t = s;
	    intern->p_rgb.green = strtoul(s, (char **)&s, 16);
	    if (s == t)
		return FALSE;

	    s++;
	    t = s;
	    intern->p_rgb.blue = strtoul(s, (char **)&s, 16);
	    if (s == t)
		return FALSE;

	    intern->p_type = PST_RGB;
	    break;
	}
    }
    return TRUE;
}

BOOL zune_pen_string_to_intern (CONST_STRPTR spec,
				struct MUI_PenSpec_intern *intern)
{
    return zune_pen_spec_to_intern((const struct MUI_PenSpec *)spec, intern);
}

BOOL zune_pen_intern_to_spec (const struct MUI_PenSpec_intern *intern,
			      struct MUI_PenSpec *spec)
{
    if (!spec || !intern)
	return FALSE;

    switch (intern->p_type)
    {
	case PST_MUI:
	    spec->ps_buf[0] = 'm';
	    sprintf(spec->ps_buf + 1, "%ld", intern->p_mui);
	    break;

	case PST_CMAP:
	    spec->ps_buf[0] = 'p';
	    sprintf(spec->ps_buf + 1, "%ld", intern->p_cmap);
	    break;

	case PST_RGB:
	    spec->ps_buf[0] = 'r';
	    sprintf(spec->ps_buf + 1, "%08lx,%08lx,%08lx",
		    intern->p_rgb.red, intern->p_rgb.green, intern->p_rgb.blue);
	    break;

	case PST_SYS:
	    spec->ps_buf[0] = 's';
	    sprintf(spec->ps_buf + 1, "%ld", intern->p_sys);
	    break;

	default:
	    return FALSE;
    }
    return TRUE;
}

void zune_penspec_fill_muipen(struct MUI_PenSpec_intern *psi, LONG muipen)
{
    if (psi)
    {
	psi->p_type = PST_MUI;
	psi->p_mui = muipen;
    }
}

void zune_penspec_fill_rgb(struct MUI_PenSpec_intern *psi, ULONG r, ULONG g, ULONG b)
{
    if (psi)
    {
	psi->p_type = PST_RGB;
	psi->p_rgb.red = r;
	psi->p_rgb.green = g;
	psi->p_rgb.blue = b;
    }
}

static void set_pen_from_rgb (struct MUI_PenSpec_intern *psi, struct ColorMap *cm,
			      ULONG r, ULONG g, ULONG b)
{
    LONG pen;
    struct TagItem obp_tags[] =
    {
	{ OBP_FailIfBad, FALSE  },
	{ TAG_DONE,      0L     }
    };

    pen = ObtainBestPenA(cm, r, g, b, obp_tags);
    if (pen == -1)
    {
	psi->p_is_allocated = FALSE;
	psi->p_pen = FindColor(cm, r, g, b, -1);
    }
    else
    {
	psi->p_is_allocated = TRUE;
	psi->p_pen = pen;
    }
}

BOOL zune_penspec_setup(struct MUI_PenSpec_intern *psi, struct MUI_RenderInfo *mri)
{
    if (!psi || !mri)
	return FALSE;

    D(bug("zune_penspec_setup(%lx) type=%ld\n", psi, psi->p_type));
    psi->p_mri = mri;

    switch (psi->p_type)
    {
	case PST_MUI:
	    if ((psi->p_mui >= 0) && (psi->p_mui < MPEN_COUNT))
		psi->p_pen = mri->mri_Pens[psi->p_mui];
	    else
		return FALSE;
	    break;

	case PST_CMAP:
	{
	    psi->p_pen = (psi->p_cmap >= 0) ?
		psi->p_cmap :
		mri->mri_Colormap->Count + psi->p_cmap;
	    if ((psi->p_pen >= 0) && (psi->p_pen < mri->mri_Colormap->Count))
		psi->p_is_allocated = FALSE;
	    else
		return FALSE;
	    break;
	}
	case PST_RGB:
	    set_pen_from_rgb(psi, mri->mri_Colormap,
			     psi->p_rgb.red, psi->p_rgb.green, psi->p_rgb.blue);
	    D(bug("zune_penspec_setup(%lx)=%ld RGB(%lx,%lx,%lx)\n", psi,
		  psi->p_pen, psi->p_rgb.red, psi->p_rgb.green, psi->p_rgb.blue));
	    break;

	case PST_SYS:
	    break;

	default:
	    return FALSE;
    }
    return TRUE;
}

BOOL zune_penspec_cleanup(struct MUI_PenSpec_intern *psi)
{
    if (!psi || !psi->p_mri)
	return FALSE;

    switch (psi->p_type)
    {
	case PST_MUI:
	    break;

	case PST_CMAP:
	    if (psi->p_is_allocated)
		ReleasePen(psi->p_mri->mri_Colormap, psi->p_pen);
	    break;
	
	case PST_RGB:
	    if (psi->p_is_allocated)
		ReleasePen(psi->p_mri->mri_Colormap, psi->p_pen);
	    break;

	case PST_SYS:
	    break;

	default:
	    return FALSE;
    }
    return TRUE;
}

void zune_penspec_drawdirect(struct MUI_PenSpec_intern *psi, struct RastPort *rp, struct MUI_RenderInfo *mri,
		       LONG left, LONG top, LONG right, LONG bottom)
{
    if (!psi || !mri || !rp)
	return;

    if ( psi->p_type == PST_RGB)
    {
	D(bug("drawing with %lx, pen=%ld, at %ld, %ld => %ld, %ld\n",
	      psi, psi->p_pen, left, top, right, bottom));
    }
    SetAPen(rp, psi->p_pen);
    RectFill(rp, left, top, right, bottom);
}

void zune_penspec_draw(struct MUI_PenSpec_intern *psi, struct MUI_RenderInfo *mri,
		       LONG left, LONG top, LONG right, LONG bottom)
{
 zune_penspec_drawdirect(psi, mri->mri_RastPort, mri, left, top, right, bottom);
}
