/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __IMSPEC_H__
#define __IMSPEC_H__

#include <imagespec.h>
#include <pen.h>

/*
 * Include this only if you really *need* to peek in a MUI_ImageSpec
 * If you don't know, you'd better include zuneimage.h and access
 * MUI_ImageSpec via the given functions. Beware ! Danger ahead ! :)
 */

/* ImageSpec are using a simpler OO scheme, this is not BOOPSI.
 */

typedef enum {
    IST_PATTERN,
    IST_COLOR,
    IST_BITMAP,
    IST_VECTOR,
    IST_EXTERNAL,
    IST_LINK,
} ImageSpecType;

/* system image */
typedef enum {
    BI_CHECKMARK,
    BI_RADIOBUTTON,
    BI_CYCLE,
    BI_POPUP,
    BI_POPFILE,
    BI_POPDRAWER,
    BI_ARROWUP,
    BI_ARROWDOWN,
    BI_ARROWLEFT,
    BI_ARROWRIGHT,
    BI_PROPKNOB,
    BI_DRAWER,
    BI_HARDDISK,
    BI_DISK,
    BI_CHIP,
    BI_VOLUME,
    BI_NETWORK,
    BI_ASSIGN,
    BI_TAPEPLAY,
    BI_TAPEPAUSE,
    BI_TAPESTOP,
    BI_TAPERECORD,
    BI_TAPEUP,
    BI_TAPEDOWN,
} BoopsiImage;

typedef enum {
    EST_VECTOR,
    EST_BRUSH,
} ExternalSpecType;

typedef enum {
    BS_NORMAL,
    BS_SELECTED,
    BS_COUNT,
} BrushState;


/* Brushes are scaled differently for each gadget, hence a different
 * brush instance for each gadget. Thus no ref counting, and a tricky
 * behaviour of temporarily replacing an IST_LINK with a new instance
 * between Setup/Cleanup of an image.
 */
struct MUI_BrushSpec {
    GdkPixmap *pixmap[BS_COUNT];
    GdkBitmap *mask[BS_COUNT];
    APTR       image[BS_COUNT]; /* low level stuff (eg. for Imlib) */
    BrushState state; /* 0: normal, 1:selected */
    gint       border; /* border size (for imlib resize) */
    LONG       swidth; /* scaled size */
    LONG       sheight;
    struct MUI_ImageSpec *link_backup; /* coz brushes "replace" links */
};

struct MUI_VectorSpec {
};

struct MUI_ExternalSpec {
    ExternalSpecType es_Type;
    STRPTR path;
    union {
	struct MUI_BrushSpec bsh;
	struct MUI_VectorSpec vec;
    } u;
};

struct MUI_BitmapSpec {
    STRPTR path;
    GdkPixmap *pixmap;
    APTR       image;
};

typedef void
ZImageDraw (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri,
	       LONG left, LONG top, LONG width, LONG height,
	       LONG xoffset, LONG yoffset, LONG flags);

typedef struct MUI_ImageSpec *
ZImageCopy (struct MUI_ImageSpec *imspec);

typedef void
ZImageFinalize (struct MUI_ImageSpec *imspec);

typedef void
ZImageSetup (struct MUI_ImageSpec *imspec, struct MUI_RenderInfo *mri);

typedef void
ZImageCleanup (struct MUI_ImageSpec *imspec, struct MUI_RenderInfo *mri);

typedef void
ZImageShow (struct MUI_ImageSpec *imspec, Object *obj);

typedef void
ZImageHide (struct MUI_ImageSpec *imspec);

typedef STRPTR
ZImageToString (struct MUI_ImageSpec *imspec);


struct MUI_ImageSpecClass
{
    ZImageDraw     *im_DrawF;
    ZImageCopy     *im_CopyF;
    ZImageFinalize *im_FinalizeF;
    ZImageSetup    *im_SetupF;
    ZImageCleanup  *im_CleanupF;
    ZImageShow     *im_ShowF;
    ZImageHide     *im_HideF;
    ZImageToString *im_ToStringF;
};


struct MUI_ImageSpec
{
    ULONG ref_count;
    ULONG setup_count;
    ULONG show_count;

    ImageSpecType im_Type; /* image infos */
    union {
	LONG pattern;
	BoopsiImage vect;
        struct MUI_PenSpec pen;
        struct MUI_ExternalSpec ext;
	struct MUI_BitmapSpec bm;
	struct MUI_ImageSpec *link;  /* indirection to a "real" imspec */
    } u;

    LONG im_Width; /* MUI_MAXMAX for pens and patterns */
    LONG im_Height;

    struct MUI_RenderInfo *tmp_mri; /* to pass mri */

    struct MUI_ImageSpecClass *im_Class;
};

GdkPixmap *
__zune_imspec_get_pixmap (struct MUI_ImageSpec *img);

GdkPixmap *
__zune_imspec_get_mask (struct MUI_ImageSpec *img);

STRPTR
__zune_imspec_get_path (struct MUI_ImageSpec *img);

struct MUI_ImageSpec *
zune_imspec_pattern_new(ULONG muipatt);

struct MUI_ImageSpec *
zune_imspec_muipen_new(MPen muipen);



#endif

