/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/graphics.h>
#endif

#include <zunepriv.h>
#include <imspec.h>
/*  #include <gc.h> */
#include <prefs.h>
#include <file.h>
#include <areadata.h>
#include <pixmap.h>
/* for sscanf */
#include <stdio.h>


/* This is the implementation of a MUI-like image engine
 * (see MUIA_Image_Spec for more information about MUI image specs)
 * Their external form is a string "<type>:<parameters>"
 * with type being a single char. See zune_image_spec_to_structure().
 *
 * Basically an ImageSpec can be anything which can be displayed:
 * gfx datas, drawing code, ...
 * Because of this polymorphism, I decided to play it a little OO,
 * without creating a BOOPSI class, though.
 * Probably adding a new "subclass" is much work (implementing
 * needed "methods" and modifying the im_Type-peeking functions),
 * but it shouldn't be needed, as most useful cases seems covered :)
 */

/*
 * Hu-ho, more time passes, more I feel crazy.
 * Imspec seems braindead, especially IST_LINK ones, to implement
 * indirection to variables images.
 * Imspecs are kind of object-oriented (the beginners one, not BOOPSI !),
 * with a bunch of static function ptrs for each class,
 * though some simple imspec generic functions peek at the im_Type field
 * to switch to the correct behaviour.
 * Inconsistent, but avoid the pain of having to extend existent "classes"
 * with more functions. Currently 8 methods are enough :)
 * Some methods ptrs may be NULL, when a dummy func call can be avoided.
 * All images are reference counted, to avoid gratuitous copies,
 * and to allow single change => multiple effects
 *
 * Having only reference count would have been simple. But scaled images
 * require an extra treatment because they must be scaled differently
 * for each gadget they're applied to. Thus the extra complexity.
 * I don't remember if the complexity comes from the scaling problem
 * or handling a prefs change at runtime ...
 */


/* im_Type-peeking functions (ie bad OOP, but quickly written) :
 * zune_imspec_get_path
 * zune_imspec_get_pixmap
 * zune_imspec_get_mask
 * zune_imspec_set_state
 * zune_imspec_get_width (and 3 others set/height)
 * zune_imspec_set_scaled_size
 * zune_imspec_setup
 * zune_imspec_cleanup
 */

/*
 * obfuscated C is good for your health
 * I'm happy, I've hacked prev/next pattern for xmms mikmod plugin
 * (useful for some Jogeir mods :)
 */

static GMemChunk *imMemChunk;


/* Drawing */
static ZImageDraw zune_link_fill;
extern ZImageDraw _zune_fill_pattern_rectangle;
extern ZImageDraw _zune_fill_muipen_rectangle;
extern ZImageDraw _zune_fill_rgb_rectangle;
extern ZImageDraw _zune_fill_cmap_rectangle;

extern ZImageDraw _zune_fill_bitmap_rectangle;
extern ZImageDraw _zune_fill_brush_rectangle;
extern ZImageDraw _zune_fill_vector_rectangle;


/* copy funcs */
static ZImageCopy zune_imspec_readcopy;
static ZImageCopy zune_imspec_base_copy;
static ZImageCopy zune_brush_copy;

/* free funcs */
static ZImageFinalize zune_imspec_default_finalize;
static ZImageFinalize zune_imspec_rgb_finalize;
static ZImageFinalize zune_bitmap_finalize;
static ZImageFinalize zune_vector_finalize;
static ZImageFinalize zune_brush_finalize;
static ZImageFinalize zune_link_finalize;

/* setup */
static ZImageSetup zune_vector_setup;
static ZImageSetup zune_link_setup;
static ZImageSetup zune_imspec_pen_setup;

/* cleanup */
static ZImageCleanup zune_vector_cleanup;
static ZImageCleanup zune_link_cleanup;
static ZImageCleanup zune_imspec_pen_cleanup;

/* show */
static ZImageShow zune_vector_show;
static ZImageShow zune_link_show;

/* hide */
static ZImageHide zune_vector_hide;
static ZImageHide zune_link_hide;

/* to string */
static ZImageToString zune_muipen_to_string;
static ZImageToString zune_rgb_to_string;
static ZImageToString zune_cmap_to_string;
static ZImageToString zune_pattern_to_string;
static ZImageToString zune_bitmap_to_string;
static ZImageToString zune_vector_to_string;
static ZImageToString zune_brush_to_string;
static ZImageToString zune_link_to_string;

/* ctors */
static struct MUI_ImageSpec *
zune_imspec_bitmap_new(STRPTR path);

static struct MUI_ImageSpec *
zune_imspec_rgb_new(gushort red, gushort green, gushort blue, STRPTR text);

static struct MUI_ImageSpec *
zune_imspec_cmap_new(ULONG pixel);

static struct MUI_ImageSpec *
zune_imspec_vector_new(STRPTR path);

static struct MUI_ImageSpec *
zune_imspec_brush_new(STRPTR path, gint border);


static struct MUI_ImageSpecClass zune_imspec_muipen_class = {
    _zune_fill_muipen_rectangle,
    zune_imspec_readcopy,
    zune_imspec_default_finalize,
    NULL,
    NULL,
    NULL,
    NULL,
    zune_muipen_to_string
};

static struct MUI_ImageSpecClass zune_imspec_rgb_class = {
    _zune_fill_rgb_rectangle,
    zune_imspec_readcopy,
    zune_imspec_rgb_finalize,
    zune_imspec_pen_setup,
    zune_imspec_pen_cleanup,
    NULL,
    NULL,
    zune_rgb_to_string
};

static struct MUI_ImageSpecClass zune_imspec_cmap_class = {
    _zune_fill_cmap_rectangle,
    zune_imspec_readcopy,
    zune_imspec_default_finalize,
    NULL,
    NULL,
    NULL,
    NULL,
    zune_cmap_to_string
};

static struct MUI_ImageSpecClass zune_imspec_pattern_class = {
    _zune_fill_pattern_rectangle,
    zune_imspec_readcopy,
    zune_imspec_default_finalize,
    NULL,
    NULL,
    NULL,
    NULL,
    zune_pattern_to_string
};

static struct MUI_ImageSpecClass zune_imspec_bitmap_class = {
    _zune_fill_bitmap_rectangle,
    zune_imspec_readcopy,
    zune_bitmap_finalize,
    _zune_bitmap_load,
    _zune_bitmap_unload,
    NULL,
    NULL,
    zune_bitmap_to_string
};

static struct MUI_ImageSpecClass zune_imspec_vector_class = {
    _zune_fill_vector_rectangle,
    zune_imspec_readcopy,
    zune_vector_finalize,
    zune_vector_setup,
    zune_vector_cleanup,
    zune_vector_show,
    zune_vector_hide,
    zune_vector_to_string
};

static struct MUI_ImageSpecClass zune_imspec_brush_class = {
    _zune_fill_brush_rectangle,
    zune_brush_copy,
    zune_brush_finalize,
    _zune_brush_load,
    _zune_brush_unload,
    _zune_brush_render,
    _zune_brush_unrender,
    zune_brush_to_string
};

/* handle for another image */
static struct MUI_ImageSpecClass zune_imspec_link_class = {
    zune_link_fill,
    zune_imspec_readcopy,
    zune_link_finalize,
    zune_link_setup,
    zune_link_cleanup,
    zune_link_show,
    zune_link_hide,
    zune_link_to_string
};

GdkPixmap *
_zune_imspec_get_pixmap (struct MUI_ImageSpec *img);
GdkPixmap *
_zune_imspec_get_mask (struct MUI_ImageSpec *img);


/*
 * Alloc/free resources
 */

static void
imspec_destroy (void)
{
    g_mem_chunk_destroy(imMemChunk);
}

void
__zune_imspec_init(void)
{
    imMemChunk = g_mem_chunk_create(struct MUI_ImageSpec, 20, G_ALLOC_AND_FREE);
    g_atexit(imspec_destroy);
}

/**********************/

struct MUI_ImageSpec *
zune_image_spec_to_structure (ULONG in)
{
    struct MUI_ImageSpec *out = NULL;

    if (_between(MUII_WindowBack, in, MUII_ReadListBack)) /* preset image */
    {
	out = zune_imspec_copy(__zprefs.images[in]);
    }
    else if (_between(MUII_BACKGROUND, in, MUII_MARKBACKGROUND)) /* pattern */
    {
	out = zune_imspec_copy (zune_get_pattern_spec(in));
    }
    else /* translate char * */
    {
	STRPTR s = (STRPTR)in;
	ULONG tmp;

	switch(s[0])
	{
	    case '0': /* pattern */
		sscanf(s+2, "%ld", &tmp);
		out = zune_imspec_copy (zune_get_pattern_spec(tmp));
		break;
	    case '2': /* rgb */
	    {
		gushort rgbshort[3];

		sscanf(s + 2, "%hx,%hx,%hx",
		       rgbshort, rgbshort+1, rgbshort+2);
		out = zune_imspec_rgb_new(rgbshort[0], rgbshort[1], rgbshort[2], NULL);
	    }
	    break;
	    case '3': /* ext. boopsi image */
		out = zune_imspec_vector_new(s+2);
		break;
	    case '4': /* ext. brush, border 0 */
		out = zune_imspec_brush_new(s+2, 0);
		break;
	    case '5': /* bitmap */
		out = zune_imspec_bitmap_new(s+2);
		break;
	    case '6': /* preset image */
		sscanf(s+2, "%ld", &tmp);
		out = zune_imspec_copy(__zprefs.images[tmp]);
		break;
	    case '7': /* mui pen */
		sscanf(s+2, "%ld", &tmp);
		out = zune_imspec_copy (zune_get_muipen_spec(tmp));
		break;
	    case '8': /* cmap */
		sscanf(s+2, "%ld", &tmp);
		out = zune_imspec_cmap_new(tmp);
		break;
	    case '9': /* rgb text */
	    {
		GdkColor c;

/*  	    g_print("zune_image_spec_to_structure: got text %s\n", s + 2); */
		gdk_color_parse(s + 2, &c);
/*  		g_print("parsed to: %hd, %hd, %hd\n", c.red, c.green, c.blue); */
	       	out = zune_imspec_rgb_new(c.red, c.green, c.blue, s + 2);
	    }
	    break;
	    case 'a': /* ext. brush, border 0 */
		out = zune_imspec_brush_new(s+2, 0);
		break;
	    case 'b': /* ext. brush, border 1 */
		out = zune_imspec_brush_new(s+2, 1);
		break;
	    case 'c': /* ext. brush, border 2 */
		out = zune_imspec_brush_new(s+2, 2);
		break;
	    case 'd': /* ext. brush, border 3 */
		out = zune_imspec_brush_new(s+2, 3);
		break;
	}
    }
    if (out == NULL)
	g_warning("zune_image_spec_to_structure: cannot parse '%s'\n",
		  (STRPTR)in);
    return out;
}

/*
 * (*out) must be either a valid imspec, or NULL !!!
 * so we can free it if we have to replace it.
 */
void
zune_image_spec_parse_string (STRPTR s, struct MUI_ImageSpec **out)
{
    struct MUI_ImageSpec *res;

    g_return_if_fail(out != NULL);

    if (!s)
	return;
    res = zune_image_spec_to_structure((ULONG)s);
    if (res)
    {
	if (*out)
	{
	    zune_imspec_free(*out);
	}
	*out = res;
    }
}


void
zune_draw_image (struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG flags)
{
    g_return_if_fail(img != NULL && img->im_Class != NULL);
    g_assert(img->im_Class->im_DrawF != NULL);
/*  G_BREAKPOINT(); */
    img->im_Class->im_DrawF(img, mri, left, top, width, height,
			    xoffset, yoffset, flags);
}


static struct MUI_ImageSpec *
zune_imspec_new0 ()
{
    return g_chunk_new0(struct MUI_ImageSpec, imMemChunk);
/*      return g_malloc0(sizeof(struct MUI_ImageSpec)); */
}

static void
zune_imspec_delete(struct MUI_ImageSpec *mem)
{
    g_chunk_free(mem, imMemChunk);
/*      g_free(mem); */
}

static BOOL
zune_imspec_is_shown (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    g_return_val_if_fail(spec->show_count >= 0, FALSE);
    return spec->show_count > 0;
}

static BOOL
zune_imspec_is_setuped (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    g_return_val_if_fail(spec->setup_count >= 0, FALSE);
    return spec->setup_count > 0;
}

static BOOL
zune_imspec_attempt_show (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    g_return_val_if_fail(spec->show_count >= 0, FALSE);
    ++spec->show_count;
    return (spec->show_count == 1);
}

static BOOL
zune_imspec_attempt_setup (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    g_return_val_if_fail(spec->setup_count >= 0, FALSE);
    ++spec->setup_count;
    return (spec->setup_count == 1);
}

static BOOL
zune_imspec_attempt_hide (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    --spec->show_count;
    g_return_val_if_fail(spec->show_count >= 0, FALSE);
    return (spec->show_count == 0);
}

static BOOL
zune_imspec_attempt_cleanup (struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL, FALSE);
    --spec->setup_count;
    g_return_val_if_fail(spec->setup_count >= 0, FALSE);
    return (spec->setup_count == 0);
}


void
zune_imspec_ref(struct MUI_ImageSpec *spec)
{
    g_return_if_fail(spec != NULL);
    g_return_if_fail(spec->ref_count >= 0);
    spec->ref_count += 1;
/*      g_print("ref imspec %p, new ref = %d\n", spec, spec->ref_count); */
}

void
zune_imspec_unref(struct MUI_ImageSpec *spec)
{
    g_return_if_fail(spec != NULL);
    spec->ref_count -= 1;
    g_return_if_fail(spec->ref_count >= 0);
/*      g_print("unref imspec %p, new ref=%d\n", spec, spec->ref_count); */
    if (spec->ref_count == 0)
    {
	g_assert(spec->im_Class && spec->im_Class->im_FinalizeF != NULL);
	if (zune_imspec_is_shown(spec))
	{
	    g_print("img %p (%s) wasn't hidden at finalization\n", spec,
		    zune_imspec_to_string(spec));
	    zune_imspec_hide(spec);
	}
	if (zune_imspec_is_setuped(spec))
	{
/*  	    struct MUI_ImageSpec *tricky; */
	    g_print("img %p (%s) wasn't cleanuped at finalization\n", spec,
		    zune_imspec_to_string(spec));
/*  	    tricky = spec; */
/*    	    zune_imspec_cleanup(&spec, ); */
/*  	    if (tricky != spec) */
/*  	    { */
/*  		g_print("zune_imspec_unref : spec changed from %p to %p in cleanup !!!\n", */
/*  			tricky, spec); */
/*  		if (spec->ref_count != 0) */
/*  		{ */
/*  		    g_print("zune_imspec_unref : not finalizing new spec !\n"); */
/*  		    return; */
/*  		} */
/*  		else */
/*  		    g_print("zune_imspec_unref : finalizing new spec !\n"); */
/*  	    } */
	}
/*      g_print("Finalize imspec %p (begin)\n", spec); */
	spec->im_Class->im_FinalizeF(spec);
/*  	g_print(" end finaliz %p\n", spec); */
    }
}


/*
 * copy will either ref a shared image,
 * or really copy an unshared image
 */
struct MUI_ImageSpec *
zune_imspec_copy(struct MUI_ImageSpec *spec)
{
    struct MUI_ImageSpec *res;
    g_return_val_if_fail(spec != NULL && spec->im_Class, NULL);
    g_assert(spec->im_Class->im_CopyF != NULL);
/*      g_print("begin copy %p\n", spec); */
    res = spec->im_Class->im_CopyF(spec);
/*      g_print("end copy %p, res = %p (%s)\n", spec, res, zune_imspec_to_string(res)); */
    return res;
}

static struct MUI_ImageSpec *
zune_imspec_readcopy(struct MUI_ImageSpec *spec)
{
    g_return_val_if_fail(spec != NULL && spec->im_Class, NULL);
    zune_imspec_ref(spec);
    return spec;
}

void
zune_imspec_free(struct MUI_ImageSpec *spec)
{
/*      g_print("begin free %p\n", spec); */
    zune_imspec_unref(spec);
/*      g_print("end free %p\n", spec); */
}


/* load file(s) */
/*
 * why **img ? because of brushes. They are scaled differently
 * for each gadget, so having many references to the same brush
 * leads to draw a badly scaled brush for most gadgets.
 * Instead we replace the original image link with
 * a new copy of the brush (brushes are not refcounted, but really
 * different copies). 
 */
void
zune_imspec_setup (struct MUI_ImageSpec **img, struct MUI_RenderInfo *mri)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail((*img) != NULL);
    g_return_if_fail((*img)->im_Class != NULL);

    if ((*img)->im_Type == IST_LINK
	&& (*img)->u.link->im_Type == IST_EXTERNAL
	&& (*img)->u.link->u.ext.es_Type == EST_BRUSH)
    {
	struct MUI_ImageSpec *brush_copy;

	brush_copy = zune_brush_copy((*img)->u.link);
	brush_copy->u.ext.u.bsh.link_backup = *img;
/*  g_print("zune_imspec_setup: replaced link %p with brush copy %p (%s) (orig=%p)\n", */
/*  	*img, brush_copy, zune_imspec_to_string(brush_copy), (*img)->u.link); */
	*img = brush_copy;
    }

    if (zune_imspec_attempt_setup(*img))
	if ((*img)->im_Class->im_SetupF)
	    (*img)->im_Class->im_SetupF(*img, mri);
}

void
zune_imspec_cleanup (struct MUI_ImageSpec **img, struct MUI_RenderInfo *mri)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail((*img) != NULL);
    g_return_if_fail((*img)->im_Class != NULL);

    if (zune_imspec_attempt_cleanup(*img))
	if ((*img)->im_Class->im_CleanupF)
	    (*img)->im_Class->im_CleanupF(*img, mri);

    if ((*img)->im_Type == IST_EXTERNAL
	&& (*img)->u.ext.es_Type == EST_BRUSH)
    {
	if ((*img)->u.ext.u.bsh.link_backup != NULL)
	{
	    struct MUI_ImageSpec *link;

	    link = (*img)->u.ext.u.bsh.link_backup;
/*  g_print("zune_imspec_cleanup: replacing brush %p (%s) (orig=%p) with link %p\n", */
/*  	*img, zune_imspec_to_string(*img), link->u.link, link); */
	    zune_imspec_free(*img);
	    *img = link;
	}
    }

}


/* render at given size (dimensions of obj)*/
/*
 * obj is NULL when showing preset images in window show
 */
void
zune_imspec_show (struct MUI_ImageSpec *img, Object *obj)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail(img->im_Class != NULL);

    if (zune_imspec_attempt_show(img))
	if (img->im_Class->im_ShowF)
	    img->im_Class->im_ShowF(img, obj);
}

void
zune_imspec_hide (struct MUI_ImageSpec *img)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail(img->im_Class != NULL);

    if (zune_imspec_attempt_hide(img))
	if (img->im_Class->im_HideF)
	    img->im_Class->im_HideF(img);
}

STRPTR
zune_imspec_to_string (struct MUI_ImageSpec *spec)
{
/*      if (spec->im_Short == ISST_STRING) */
/*  	return spec->st.str; */
/*      else */
	return spec->im_Class->im_ToStringF(spec);
}



/*********************************/


STRPTR
__zune_imspec_get_path (struct MUI_ImageSpec *img)
{
    switch (img->im_Type)
    {
	case IST_BITMAP:
	    if (g_path_is_absolute(img->u.bm.path))
		return img->u.bm.path;
	    else
		return __zune_file_find_image(img->u.bm.path);
	case IST_EXTERNAL:
	    if (g_path_is_absolute(img->u.ext.path))
		return img->u.ext.path;
	    else
		return __zune_file_find_image(img->u.ext.path);
	case IST_LINK:
	    return __zune_imspec_get_path (img->u.link);
	default:
    }
    return NULL;
}

GdkPixmap *
__zune_imspec_get_pixmap (struct MUI_ImageSpec *img)
{
    switch (img->im_Type)
    {
	case IST_BITMAP:
	    return img->u.bm.pixmap;
	case IST_EXTERNAL:
	    if (img->u.ext.es_Type == EST_BRUSH)
	    {
		GdkPixmap *p = img->u.ext.u.bsh.pixmap[img->u.ext.u.bsh.state];
		if (!p)
		    p = img->u.ext.u.bsh.pixmap[0];
		return p;
	    }
	    break;
	case IST_LINK:
	    return __zune_imspec_get_pixmap (img->u.link);
	default:
    }
    return NULL;
}

GdkPixmap *
__zune_imspec_get_mask (struct MUI_ImageSpec *img)
{
    switch (img->im_Type)
    {
	case IST_EXTERNAL:
	    if (img->u.ext.es_Type == EST_BRUSH)
	    {
		GdkBitmap *m = img->u.ext.u.bsh.mask[img->u.ext.u.bsh.state];
		if (!m)
		    m = img->u.ext.u.bsh.mask[0];
		return m;
	    }
	    break;
	case IST_LINK:
	    return __zune_imspec_get_mask(img->u.link);
	default:
    }
    return NULL;
}

/*
 * return TRUE if state really changed
 */
BOOL
__zune_imspec_set_state (struct MUI_ImageSpec *img, ULONG state)
{
    switch (img->im_Type)
    {
	case IST_EXTERNAL:
	    if (img->u.ext.es_Type == EST_BRUSH)
	    {
		BrushState oldstate = img->u.ext.u.bsh.state;
		switch (state)
		{
		    case IDS_NORMAL:
			img->u.ext.u.bsh.state = 0;
			break;
		    case IDS_SELECTED:
			img->u.ext.u.bsh.state = 1;
			break;
		}
		return (oldstate != img->u.ext.u.bsh.state);
	    }
	    else /* vector */
	    {
	    }
	    break;
	case IST_LINK:
	    return __zune_imspec_set_state (img->u.link, state);
	default:
    }
    return FALSE;
}

/***********************************************/
/* default */

struct MUI_ImageSpec *
zune_imspec_default_new(ImageSpecType type)
{
    struct MUI_ImageSpec *out = zune_imspec_new0();

/*      g_print("NEW imspec %p\n", out); */
    out->ref_count = 1;
    out->setup_count = 0;
    out->show_count = 0;
    out->im_Type = type;
/*      out->im_Short = ISST_NONE; */
    out->im_Width = MUI_MAXMAX;
    out->im_Height = MUI_MAXMAX;
    return out;
}

static void
zune_imspec_default_finalize(struct MUI_ImageSpec *spec)
{
/*      if (spec->im_Short == ISST_STRING) */
/*  	g_free(spec->st.str); */
    g_return_if_fail(spec->ref_count == 0);
    zune_imspec_delete(spec);
}

static struct MUI_ImageSpec *
zune_imspec_base_copy(struct MUI_ImageSpec *spec)
{
    struct MUI_ImageSpec *copy = zune_imspec_new0();
/*      g_print("NEW imspec copy %p\n", copy); */
    *copy = *spec;
    copy->ref_count = 1;
    copy->setup_count = 0;
    copy->show_count = 0;
/*      copy->im_Short = ISST_NONE; */
    return copy;
}


/*------*/
/* Bitmap */
struct MUI_ImageSpec *
zune_imspec_bitmap_new(STRPTR path)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_BITMAP);

    if (!path)
    {
	zune_imspec_default_finalize(out);
	return NULL;
    }

    out->u.bm.path = g_strdup(path);
    out->im_Class = &zune_imspec_bitmap_class;
/*  g_print("zune_imspec_bitmap_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

static void
zune_bitmap_finalize(struct MUI_ImageSpec *spec)
{
    g_free(spec->u.bm.path);
    zune_imspec_default_finalize(spec);
}

STRPTR
zune_bitmap_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[1000];

    g_snprintf(buf, 1000, "5:%s", spec->u.bm.path);
    return buf;
}

/*  static struct MUI_ImageSpec * */
/*  zune_imspec_bitmap_copy(struct MUI_ImageSpec *spec) */
/*  { */
/*      struct MUI_ImageSpec *copy = zune_imspec_default_copy(spec); */

/*      copy->u.bm.path = g_strdup(spec->u.bm.path); */
/*      return copy; */
/*  } */

/*------*/
/* Pattern */
struct MUI_ImageSpec *
zune_imspec_pattern_new(ULONG muipatt)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_PATTERN);

    out->u.pattern = muipatt;
    out->im_Class = &zune_imspec_pattern_class;
/*  g_print("zune_imspec_pattern_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

STRPTR
zune_pattern_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[16];

    g_snprintf(buf, 16, "0:%ld", spec->u.pattern);
    return buf;
}

/********************/
/* common to Pens : */

static void
zune_imspec_pen_setup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail(mri != NULL);

    zune_penspec_setup(mri, &img->u.pen);
}

static void
zune_imspec_pen_cleanup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail(mri != NULL);

    zune_penspec_cleanup(mri, &img->u.pen);
}



/*------*/
/* RGB */
struct MUI_ImageSpec *
zune_imspec_rgb_new(gushort red, gushort green, gushort blue, STRPTR text)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_COLOR);

    out->u.pen.ps_penType = PST_RGB;
    if (text)
	out->u.pen.ps_rgbText = g_strdup(text);
    out->u.pen.ps_rgbColor.red = red;
    out->u.pen.ps_rgbColor.green = green;
    out->u.pen.ps_rgbColor.blue = blue;
    out->im_Class = &zune_imspec_rgb_class;
/*  g_print("zune_imspec_rgb_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

static void
zune_imspec_rgb_finalize(struct MUI_ImageSpec *spec)
{
    g_return_if_fail(spec->ref_count == 0);
    zune_penspec_destroy_content(&spec->u.pen);
    zune_imspec_delete(spec);
}

STRPTR
zune_rgb_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[100];
    if (spec->u.pen.ps_rgbText)
	g_snprintf(buf, 100, "9:%s", spec->u.pen.ps_rgbText);
    else
	g_snprintf(buf, 100, "2:%s",
		   zune_gdkcolor_to_string(&spec->u.pen.ps_rgbColor));
    return buf;
}

/* mui */
struct MUI_ImageSpec *
zune_imspec_muipen_new(MPen muipen)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_COLOR);

    out->u.pen.ps_penType = PST_MUI;
    out->u.pen.ps_mui = muipen;
    out->im_Class = &zune_imspec_muipen_class;
/*  g_print("zune_imspec_muipen_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

STRPTR
zune_muipen_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[16];

    g_snprintf(buf, 16, "7:%d", spec->u.pen.ps_mui);
    return buf;
}

/* cmap */
struct MUI_ImageSpec *
zune_imspec_cmap_new(ULONG pixel)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_COLOR);

    out->u.pen.ps_penType = PST_CMAP;
    out->u.pen.ps_cmap = pixel;
    out->im_Class = &zune_imspec_cmap_class;
/*  g_print("zune_imspec_cmap_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

STRPTR
zune_cmap_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[16];

    g_snprintf(buf, 16, "8:%ld", spec->u.pen.ps_cmap);
    return buf;
}

/*------*/
/* Vector */

struct MUI_ImageSpec *
zune_imspec_vector_new(STRPTR path)
{
    struct MUI_ImageSpec *out = zune_imspec_default_new(IST_EXTERNAL);

    out->u.ext.es_Type = EST_VECTOR;
    out->u.ext.path = g_strdup(path);
    out->im_Class = &zune_imspec_vector_class;
/*  g_print("zune_imspec_vector_new: %s\n", zune_imspec_to_string(out)); */
    return out;
}

static void
zune_vector_finalize(struct MUI_ImageSpec *spec)
{
    g_free(spec->u.ext.path);
    zune_imspec_default_finalize(spec);
}

STRPTR
zune_vector_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[1000];

    g_snprintf(buf, 1000, "3:%s", spec->u.ext.path);
    return buf;
}

/*  static struct MUI_ImageSpec * */
/*  zune_imspec_vector_copy(struct MUI_ImageSpec *spec) */
/*  { */
/*      struct MUI_ImageSpec *copy = zune_imspec_default_copy(spec); */

/*      copy->u.ext.path = g_strdup(spec->u.ext.path); */
/*      return copy; */
/*  } */

static void
zune_vector_setup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
}

static void
zune_vector_cleanup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
}

static void
zune_vector_show (struct MUI_ImageSpec *img, Object *obj)
{
}

static void
zune_vector_hide (struct MUI_ImageSpec *img)
{
}

/*------*/
/* Brush */

struct MUI_ImageSpec *
zune_imspec_brush_new(STRPTR path, gint border)
{
    struct MUI_ImageSpec *out;

/*      g_print("zune_imspec_brush_new: %s ", path); */
    out = zune_imspec_default_new(IST_EXTERNAL);
/*      g_print("(%p)\n", out); */

    out->u.ext.es_Type = EST_BRUSH;
    out->u.ext.path = g_strdup(path);
    out->u.ext.u.bsh.border = border;
    out->im_Class = &zune_imspec_brush_class;
/*  g_print("zune_imspec_brush_new: %p (%s)\n", out, zune_imspec_to_string(out)); */
    return out;
}

STRPTR
zune_brush_to_string (struct MUI_ImageSpec *spec)
{
    static char buf[1000];

    g_snprintf(buf, 1000, "%c:%s", 'a' + spec->u.ext.u.bsh.border,
	       spec->u.ext.path);
    return buf;
}

static void
zune_brush_finalize(struct MUI_ImageSpec *spec)
{
/*  g_print("zune_imspec_brush_finalize: %p (%s)\n", spec, zune_imspec_to_string(spec)); */
    g_free(spec->u.ext.path);
    zune_imspec_default_finalize(spec);
}

static struct MUI_ImageSpec *
zune_brush_copy(struct MUI_ImageSpec *spec)
{
    struct MUI_ImageSpec *copy;

/*  g_print("gonna copy brush %p (%s)\n", spec, zune_imspec_to_string(spec)); */
/*   g_print("link_backup = %p\n", spec->u.ext.u.bsh.link_backup); */

/* So this assert trapped you. It's because you must never copy a brush
   that replaced a link. This brush is a copy. Only the original brush
   (pointed by the link) must be copied. Else at cleanup there will be
   a big mess leading to violent crashes !!! */
g_assert(spec->u.ext.u.bsh.link_backup == NULL);

    copy = zune_imspec_base_copy(spec);
    copy->u.ext.path = g_strdup(spec->u.ext.path);
/*  g_print("copied brush %p to %p (%s)\n", spec, copy, zune_imspec_to_string(spec)); */
    return copy;
}

struct MUI_ImageSpec *
zune_imspec_link_new(struct MUI_ImageSpec *linked)
{
    struct MUI_ImageSpec *out;

    g_return_val_if_fail(linked != NULL, NULL);
    out = zune_imspec_default_new(IST_LINK);
    out->u.link = zune_imspec_copy(linked);
    out->im_Width = linked->im_Width;
    out->im_Height = linked->im_Height;
    out->im_Class = &zune_imspec_link_class;
/*  g_print("zune_imspec_link_new: %p (linked %p) (%s)\n", out, linked, zune_imspec_to_string(out)); */
    return out;
}

static void
zune_link_fill(struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri,
	       LONG left, LONG top, LONG width, LONG height,
	       LONG xoffset, LONG yoffset, LONG flags)
{
    zune_draw_image(mri, img->u.link, left, top, width, height, xoffset, yoffset, flags);
}

static void
zune_link_setup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
/*  g_print("zune_link_setup: %p ask to %p\n", img, img->u.link); */
    zune_imspec_setup(&img->u.link, mri);
}

static void
zune_link_cleanup (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
    zune_imspec_cleanup(&img->u.link, mri);
}

static void
zune_link_show (struct MUI_ImageSpec *img, Object *obj)
{
/*  g_print("zune_link_show: %p ask to %p (obj=%p)\n", img, img->u.link, obj); */
    zune_imspec_show(img->u.link, obj);
}

static void
zune_link_hide (struct MUI_ImageSpec *img)
{
    zune_imspec_hide(img->u.link);
}

STRPTR
zune_link_to_string (struct MUI_ImageSpec *img)
{
/*      static char buf[1000]; */

/*      g_snprintf(buf, 1000, "[link to] %s", zune_imspec_to_string(img->u.link)); */
/*      return buf; */
    return zune_imspec_to_string(img->u.link);
}

static void
zune_link_finalize(struct MUI_ImageSpec *img)
{
/*  g_print("zune_link_finalize: %p (link=%p, %s)\n", img, img->u.link, */
/*  	zune_imspec_to_string(img->u.link)); */
    zune_imspec_free(img->u.link);
    zune_imspec_default_finalize(img);
}

/* You must have copied new_link before (for example, by getting
 * it from zune_image_spec_to_structure which does this for you)
 */
void
zune_link_rebind (struct MUI_ImageSpec *img, struct MUI_ImageSpec *new_link)
{
    if (new_link == NULL)
	return;
    
    zune_imspec_free(img->u.link);
    img->u.link = new_link;
    img->im_Width = new_link->im_Width;
    img->im_Height = new_link->im_Height;
/*  g_print("zune_link_rebind: %p got relinked to %p (%s)\n", img, new_link, */
/*  	zune_imspec_to_string(new_link)); */
}




LONG
zune_imspec_get_width (struct MUI_ImageSpec *img)
{
    g_return_val_if_fail(img != NULL, 10);
    if (img->im_Type == IST_LINK)
	return img->u.link->im_Width;
    else
	return img->im_Width;
}

LONG
zune_imspec_get_height (struct MUI_ImageSpec *img)
{
    g_return_val_if_fail(img != NULL, 10);
    if (img->im_Type == IST_LINK)
	return img->u.link->im_Height;
    else
	return img->im_Height;
}

void
zune_imspec_set_width (struct MUI_ImageSpec *img, LONG w)
{
    g_return_if_fail(img != NULL);
    if (img->im_Type == IST_LINK)
	img->u.link->im_Width = w;
    else
	img->im_Width = w;
}


void
zune_imspec_set_height (struct MUI_ImageSpec *img, LONG h)
{
    g_return_if_fail(img != NULL);
    if (img->im_Type == IST_LINK)
	img->u.link->im_Height = h;
    else
	img->im_Height = h;
}

void
zune_imspec_set_scaled_size (struct MUI_ImageSpec *img, LONG w, LONG h)
{
    g_return_if_fail(img != NULL);
    if (img->im_Type == IST_EXTERNAL && img->u.ext.es_Type == EST_BRUSH)
    {
	img->u.ext.u.bsh.swidth = w;
	img->u.ext.u.bsh.sheight = h;
    }
}

void
zune_imspec_set_renderinfo (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri)
{
    g_return_if_fail(img != NULL);
    g_return_if_fail(mri != NULL);
    
    img->tmp_mri = mri;
}
