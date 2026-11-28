/*
    Copyright (C) 2011-2026, The AROS Development Team.

    Desc: decortheme.library - theme image set loading/freeing

    Image containers are created and destroyed through the public
    decorator.library API so their ownership stays with the render
    engine.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/decorator.h>

#include <dos/dos.h>

#include <stdio.h>
#include <string.h>

#include <aros/libcall.h>

#include <libraries/decortheme.h>
#include "decortheme_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/*
    Theme image lookup
    ==================

    Theme images live in size specific directories beneath the theme,
    using the naming conventions of MUI image sets:

        <theme>/Images/<size>pt/<Name>.mf0 .. .mfN   one file per state
        <theme>/Images/<size>pt/<Name>.mb0 .. .mbN   one file per state
        <theme>/Images/<size>pt/<Name>.mim           all states in one image
        <theme>/Images/<size>pt/<Name>.mbr           a single state
        <theme>/Images/<size>pt/<Name>.mi1           a single state

    State files are numbered in the order the states are stored in the
    equivalent combined image (left to right, then top to bottom), so
    for a window gadget .mf0 is the normal image, .mf1 the selected one,
    .mf2 the inactive one and .mf3 the inactive selected one. A missing
    state falls back to the closest available one, which lets a theme
    provide just the .mf0/.mf1 pair used by MUI, or a single .mbr.

    An image is taken from the set of the size the configuration was
    loaded for. If that set does not have it, the other sets are tried,
    closest size first.
*/

#define THEMEIMAGE_DIR          DECORTHEME_IMAGEDIR
#define THEMEIMAGE_MAXSIZES     DECORTHEME_MAXSIZES
#define THEMEIMAGE_MAXSTATES    8
#define THEMEIMAGE_NAMELEN      108

struct ThemeImageSizes
{
    ULONG count;
    ULONG size[THEMEIMAGE_MAXSIZES];
};

struct DecorImages * NewDecorImages(void)
{
    return AllocVec(sizeof(struct DecorImages), MEMF_ANY | MEMF_CLEAR);
}

static ULONG SizeDistance(ULONG size, ULONG wanted)
{
    return (size > wanted) ? (size - wanted) : (wanted - size);
}

/* Collects the image sizes a theme provides, best match for 'wanted' first */
static void FindThemeImageSizes(STRPTR path, ULONG wanted, struct ThemeImageSizes *tis)
{
    ULONG i, j;

    tis->count = FindThemeSizes(path, tis->size, THEMEIMAGE_MAXSIZES);

    /* closest to the wanted size first; a larger image wins a tie */
    for (i = 1; i < tis->count; i++)
    {
        ULONG cur = tis->size[i];

        for (j = i; j > 0; j--)
        {
            ULONG dprev = SizeDistance(tis->size[j - 1], wanted);
            ULONG dcur = SizeDistance(cur, wanted);

            if ((dprev < dcur) || ((dprev == dcur) && (tis->size[j - 1] > cur)))
                break;
            tis->size[j] = tis->size[j - 1];
        }
        tis->size[j] = cur;
    }
}

static BOOL ThemeFileExists(STRPTR path, CONST_STRPTR name)
{
    STRPTR  buffer;
    BPTR    lock;
    LONG    len = strlen(path) + strlen(name) + 2;

    buffer = AllocVec(len, MEMF_ANY | MEMF_CLEAR);
    if (!buffer)
        return FALSE;
    strcpy(buffer, path);
    AddPart(buffer, name, len);

    lock = Lock(buffer, ACCESS_READ);
    FreeVec(buffer);
    if (!lock)
        return FALSE;
    UnLock(lock);

    return TRUE;
}

static struct DecorImage *LoadThemeFile(STRPTR path, CONST_STRPTR name, ULONG cols, ULONG rows)
{
    if (!ThemeFileExists(path, name))
        return NULL;
    return DGetImageFromFile(path, (STRPTR)name, cols, rows);
}

/* The state used when a theme does not provide one */
static ULONG FallbackState(ULONG state, struct DecorImage **st)
{
    /* inactive selected -> selected */
    if ((state == 3) && (st[1] != NULL))
        return 1;
    return 0;
}

/* Builds a cols x rows image from individual state images. Entries
   that are NULL are filled using the fallback state. st[0] must be
   valid; none of the state images are freed. */
static struct DecorImage *ComposeStates(struct DecorImage **st, ULONG cols, ULONG rows)
{
    struct DecorImage  *out;
    ULONG               states = cols * rows, k;
    UWORD               w = st[0]->w, h = st[0]->h;

    out = DNewDecorImageContainer(w * cols, h * rows);
    if (!out)
        return NULL;

    for (k = 0; k < states; k++)
    {
        struct DecorImage *src = st[k] ? st[k] : st[FallbackState(k, st)];

        DDrawPartToImage(src, out, 0, 0, w, h, (k % cols) * w, (k / cols) * h);
    }
    out->subimagescols = cols;
    out->subimagesrows = rows;
    out->ok = TRUE;

    return out;
}

/* <dir>/<name>.<ext>0 .. <ext>N */
static struct DecorImage *LoadStateFiles(STRPTR path, CONST_STRPTR dir, CONST_STRPTR name,
    CONST_STRPTR ext, ULONG cols, ULONG rows)
{
    struct DecorImage  *st[THEMEIMAGE_MAXSTATES];
    struct DecorImage  *out;
    TEXT                file[THEMEIMAGE_NAMELEN];
    ULONG               states = cols * rows, k;

    if (states > THEMEIMAGE_MAXSTATES)
        return NULL;

    snprintf(file, sizeof(file), "%s/%s.%s0", dir, name, ext);
    st[0] = LoadThemeFile(path, file, 1, 1);
    if (!st[0])
        return NULL;
    if (states == 1)
        return st[0];

    for (k = 1; k < states; k++)
    {
        snprintf(file, sizeof(file), "%s/%s.%s%lu", dir, name, ext, (unsigned long)k);
        st[k] = LoadThemeFile(path, file, 1, 1);
        /* every state has to cover the same area */
        if (st[k] && ((st[k]->w != st[0]->w) || (st[k]->h != st[0]->h)))
        {
            DDisposeImageContainer(st[k]);
            st[k] = NULL;
        }
    }

    out = ComposeStates(st, cols, rows);

    for (k = 0; k < states; k++)
        DDisposeImageContainer(st[k]);

    return out;
}

/* <dir>/<name>.<ext>, holding a single state */
static struct DecorImage *LoadSingleFile(STRPTR path, CONST_STRPTR dir, CONST_STRPTR name,
    CONST_STRPTR ext, ULONG cols, ULONG rows)
{
    struct DecorImage  *st[THEMEIMAGE_MAXSTATES];
    struct DecorImage  *out;
    TEXT                file[THEMEIMAGE_NAMELEN];
    ULONG               states = cols * rows, k;

    if (states > THEMEIMAGE_MAXSTATES)
        return NULL;

    snprintf(file, sizeof(file), "%s/%s.%s", dir, name, ext);
    st[0] = LoadThemeFile(path, file, 1, 1);
    if (!st[0] || (states == 1))
        return st[0];

    for (k = 1; k < states; k++)
        st[k] = NULL;

    out = ComposeStates(st, cols, rows);
    DDisposeImageContainer(st[0]);

    return out;
}

/*
    Loads the image 'name', which holds cols x rows states, trying the
    sizes the theme provides in order of preference.
*/
static struct DecorImage *LoadThemeImage(STRPTR path, struct ThemeImageSizes *tis,
    CONST_STRPTR name, ULONG cols, ULONG rows)
{
    struct DecorImage  *di = NULL;
    TEXT                dir[32];
    TEXT                file[THEMEIMAGE_NAMELEN];
    ULONG               i;

    for (i = 0; (i < tis->count) && !di; i++)
    {
        snprintf(dir, sizeof(dir), THEMEIMAGE_DIR "/%lupt", (unsigned long)tis->size[i]);

        if ((di = LoadStateFiles(path, dir, name, "mf", cols, rows)) != NULL)
            break;
        if ((di = LoadStateFiles(path, dir, name, "mb", cols, rows)) != NULL)
            break;
        snprintf(file, sizeof(file), "%s/%s.mim", dir, name);
        if ((di = LoadThemeFile(path, file, cols, rows)) != NULL)
            break;
        if ((di = LoadSingleFile(path, dir, name, "mbr", cols, rows)) != NULL)
            break;
        di = LoadSingleFile(path, dir, name, "mi1", cols, rows);
    }

    return di;
}

/*
    Loads one state of the image 'name' as an image of its own. Used
    where the states of an image are rendered independently.
*/
static struct DecorImage *LoadThemeImageState(STRPTR path, struct ThemeImageSizes *tis,
    CONST_STRPTR name, ULONG state)
{
    struct DecorImage  *di = NULL;
    TEXT                file[THEMEIMAGE_NAMELEN];
    ULONG               i;

    for (i = 0; (i < tis->count) && !di; i++)
    {
        snprintf(file, sizeof(file), THEMEIMAGE_DIR "/%lupt/%s.mf%lu",
            (unsigned long)tis->size[i], name, (unsigned long)state);
        if ((di = LoadThemeFile(path, file, 1, 1)) != NULL)
            break;
        snprintf(file, sizeof(file), THEMEIMAGE_DIR "/%lupt/%s.mb%lu",
            (unsigned long)tis->size[i], name, (unsigned long)state);
        di = LoadThemeFile(path, file, 1, 1);
    }

    return di;
}

struct DecorImages * LoadImages(struct DecorConfig * dc)
{
    ULONG   wgsubimagecols = 4; /* Default value of subimage cols in window gadget */
    STRPTR  path = dc->ThemePath;
    struct ThemeImageSizes tis;

    struct DecorImages * di = NewDecorImages();

    if (!di)
        return NULL;

    if (dc->GadgetsThreeState) wgsubimagecols = 3;

    FindThemeImageSizes(path, dc->ImageSize, &tis);

    di->img_sdepth = LoadThemeImage(path, &tis, "ScrDepth", 2, 1);
    di->img_stitlebar = LoadThemeImage(path, &tis, "ScrBar", 1, 1);
    di->img_sbarlogo = LoadThemeImage(path, &tis, "ScrBarLogo", 1, 1);

    di->img_size = LoadThemeImage(path, &tis, "WinSize", wgsubimagecols, 1);
    di->img_close = LoadThemeImage(path, &tis, "WinClose", wgsubimagecols, 1);
    di->img_depth = LoadThemeImage(path, &tis, "WinDepth", wgsubimagecols, 1);
    di->img_zoom = LoadThemeImage(path, &tis, "WinZoom", wgsubimagecols, 1);
    di->img_mui = LoadThemeImage(path, &tis, "WinMUI", wgsubimagecols, 1);
    di->img_popup = LoadThemeImage(path, &tis, "WinPopUp", wgsubimagecols, 1);
    di->img_snapshot = LoadThemeImage(path, &tis, "WinSnapshot", wgsubimagecols, 1);
    di->img_iconify = LoadThemeImage(path, &tis, "WinIconify", wgsubimagecols, 1);
    di->img_lock = LoadThemeImage(path, &tis, "WinLock", wgsubimagecols, 1);
    di->img_up = LoadThemeImage(path, &tis, "ArrowUp", wgsubimagecols, 1);
    di->img_down = LoadThemeImage(path, &tis, "ArrowDown", wgsubimagecols, 1);
    di->img_left = LoadThemeImage(path, &tis, "ArrowLeft", wgsubimagecols, 1);
    di->img_right = LoadThemeImage(path, &tis, "ArrowRight", wgsubimagecols, 1);
    di->img_winbar_normal = LoadThemeImage(path, &tis, "WinBar", 1, 2);
    di->img_border_normal = LoadThemeImageState(path, &tis, "WinBorder", 0);
    di->img_border_deactivated = LoadThemeImageState(path, &tis, "WinBorder", 1);
    di->img_verticalcontainer = LoadThemeImage(path, &tis, "PropBackVert", 2, 1);
    di->img_verticalknob = LoadThemeImage(path, &tis, "PropKnobVert", 3, 1);
    di->img_horizontalcontainer = LoadThemeImage(path, &tis, "PropBackHoriz", 1, 2);
    di->img_horizontalknob = LoadThemeImage(path, &tis, "PropKnobHoriz", 1, 3);

    di->img_menu = LoadThemeImage(path, &tis, "MenuBack", 1, 1);
    di->img_amigakey = LoadThemeImage(path, &tis, "MenuAmigaKey", 1, 1);
    di->img_menucheck = LoadThemeImage(path, &tis, "MenuCheck", 1, 1);
    di->img_submenu = LoadThemeImage(path, &tis, "MenuSubMenu", 1, 1);

    return di;
}

void FreeImages(struct DecorImages * di)
{
    DDisposeImageContainer(di->img_sdepth);
    DDisposeImageContainer(di->img_sbarlogo);
    DDisposeImageContainer(di->img_stitlebar);

    DDisposeImageContainer(di->img_size);
    DDisposeImageContainer(di->img_close);
    DDisposeImageContainer(di->img_depth);
    DDisposeImageContainer(di->img_zoom);
    DDisposeImageContainer(di->img_up);
    DDisposeImageContainer(di->img_down);
    DDisposeImageContainer(di->img_left);
    DDisposeImageContainer(di->img_right);
    DDisposeImageContainer(di->img_mui);
    DDisposeImageContainer(di->img_popup);
    DDisposeImageContainer(di->img_snapshot);
    DDisposeImageContainer(di->img_iconify);
    DDisposeImageContainer(di->img_lock);
    DDisposeImageContainer(di->img_winbar_normal);
    DDisposeImageContainer(di->img_border_normal);
    DDisposeImageContainer(di->img_border_deactivated);
    DDisposeImageContainer(di->img_verticalcontainer);
    DDisposeImageContainer(di->img_verticalknob);
    DDisposeImageContainer(di->img_horizontalcontainer);
    DDisposeImageContainer(di->img_horizontalknob);

    DDisposeImageContainer(di->img_menu);
    DDisposeImageContainer(di->img_amigakey);
    DDisposeImageContainer(di->img_menucheck);
    DDisposeImageContainer(di->img_submenu);

    FreeVec(di);
}

/* ========== Library Entry Points ========== */

/*****************************************************************************
    NAME
        struct DecorImages * DTNewDecorImages(

    SYNOPSIS
        )

    FUNCTION
        Allocate an empty, independently owned decoration-image set.

    INPUTS
        None.

    RESULT
        Owned DecorImages with all image pointers NULL, or NULL on allocation failure.

    NOTES
        No image data is loaded or allocated. Populate image fields only with
        containers owned exclusively by this set and suitable for disposal
        through decorator.library/DDisposeImageContainer(). Each non-NULL
        container must appear in at most one field and have no other owner.

        Free the set with DTFreeImages() when no renderer or element descriptor
        uses it. Do not insert borrowed images from a theme or screen instance.
        Call in task context with decortheme.library open.

    SEE ALSO
        DTFreeImages(), DTGetImages()

*****************************************************************************/
AROS_LH0(struct DecorImages *, DTNewDecorImages,
    struct Library *, DecorThemeBase, 11, Decortheme)
{
    AROS_LIBFUNC_INIT
    return NewDecorImages();
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void DTFreeImages(

    SYNOPSIS
        struct DecorImages *di)

    FUNCTION
        Dispose the images in an independently owned set and free the set.

    INPUTS
        di - Non-NULL, owned DecorImages returned by DTNewDecorImages().
            Each image field must be NULL or own a distinct disposable container.

    RESULT
        None.

    NOTES
        Every image field is disposed through decorator.library, then the set
        is freed. NULL image fields are accepted; a NULL set pointer is not.
        Stop all rendering and release users of its images before calling.
        There is no reference count, alias detection or synchronization.

        Do not pass the borrowed result of DTGetImages() or a screen instance's
        dts_Images. Their owning theme or screen instance releases those sets.
        Call once, in task context with decortheme.library open.

    SEE ALSO
        DTNewDecorImages(), DTFreeTheme(), DTReleaseScreenTheme()

*****************************************************************************/
AROS_LH1(void, DTFreeImages,
    AROS_LHA(struct DecorImages *, di, A0),
    struct Library *, DecorThemeBase, 12, Decortheme)
{
    AROS_LIBFUNC_INIT
    FreeImages(di);
    AROS_LIBFUNC_EXIT
}
