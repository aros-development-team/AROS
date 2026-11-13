/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library - per-screen theme instantiation

    Creates screen matched copies of a theme's images (converted to the
    depth/attributes of the target screen) and element descriptors over
    those copies. Instances are self contained; a theme can be
    instantiated for any number of screens concurrently.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/decorator.h>

#include <aros/libcall.h>

#include <libraries/decortheme.h>
#include "decortheme_intern.h"

#define REMAP_SCR(id) dts->dts_Images->img_##id = \
    DCreateDecorImageContainerMatchingScreen(theme->dt_Images->img_##id, truecolor, scr)

struct DecorThemeScreen *ObtainScreenTheme(struct DecorTheme *theme, struct Screen *scr, BOOL truecolor)
{
    struct DecorThemeScreen *dts;

    if (!theme)
        return NULL;

    dts = AllocVec(sizeof(struct DecorThemeScreen), MEMF_ANY | MEMF_CLEAR);
    if (!dts)
        return NULL;

    dts->dts_Theme = theme;
    dts->dts_Screen = scr;
    dts->dts_TrueColor = truecolor;

    dts->dts_Images = NewDecorImages();
    if (!dts->dts_Images)
    {
        FreeVec(dts);
        return NULL;
    }

    REMAP_SCR(sdepth);
    REMAP_SCR(sbarlogo);
    REMAP_SCR(stitlebar);

    REMAP_SCR(size);
    REMAP_SCR(close);
    REMAP_SCR(depth);
    REMAP_SCR(zoom);
    REMAP_SCR(up);
    REMAP_SCR(down);
    REMAP_SCR(left);
    REMAP_SCR(right);
    REMAP_SCR(mui);
    REMAP_SCR(popup);
    REMAP_SCR(snapshot);
    REMAP_SCR(iconify);
    REMAP_SCR(lock);
    REMAP_SCR(winbar_normal);
    REMAP_SCR(border_normal);
    REMAP_SCR(border_deactivated);
    REMAP_SCR(verticalcontainer);
    REMAP_SCR(verticalknob);
    REMAP_SCR(horizontalcontainer);
    REMAP_SCR(horizontalknob);

    REMAP_SCR(menu);
    REMAP_SCR(amigakey);
    REMAP_SCR(menucheck);
    REMAP_SCR(submenu);

    InitThemeElements(dts->dts_Elements, theme->dt_Config, dts->dts_Images);

    return dts;
}

void ReleaseScreenTheme(struct DecorThemeScreen *dts)
{
    if (dts)
    {
        if (dts->dts_Images)
            FreeImages(dts->dts_Images);
        FreeVec(dts);
    }
}

/* ========== Library Entry Points ========== */

/*****************************************************************************
    NAME
        struct DecorThemeScreen * DTObtainScreenTheme(

    SYNOPSIS
        struct DecorTheme *theme, struct Screen *scr, BOOL truecolor)

    FUNCTION
        Create a theme instance with image copies matched to a target screen.

    INPUTS
        theme - Live theme returned by DTLoadTheme(); NULL returns NULL.
        scr - Live, non-NULL Screen with a valid RastPort.BitMap. Keep it open
            until the returned instance has been released.
        truecolor - TRUE for the true-color image path; FALSE to request
            palette-matched images. Choose according to the target screen.

    RESULT
        Owned DecorThemeScreen, or NULL for a NULL theme or failure to allocate
        its outer structure or image set. Individual images can be unusable
        even when the instance is returned.

    NOTES
        The instance owns dts_Images and contains descriptors in dts_Elements.
        Inspect the required image pointers and their ok flags before use.
        The source theme and screen are borrowed, not retained; both must
        outlive the instance. Release it with DTReleaseScreenTheme().

        Each call creates a separate instance. It does not cache, install or
        activate a theme, lock the screen, or update existing instances after
        the source configuration changes. Do not replace the theme's image
        containers with arbitrary containers lacking their source filenames.

        The palette path first tries the image filename with _LUT appended,
        then the original file remapped for the screen. The true-color path
        copies source pixel data and may create screen-compatible bitmaps.
        Relative source filenames still depend on the current directory;
        use an absolute path when loading a theme for later remapping.

        Call from a DOS process with decortheme.library open. Conversion,
        file access and picture decoding are synchronous, with no caller
        timeout. Serialize access with theme mutation and screen closure.

    BUGS
        The image conversion helper has unchecked allocations and can fail
        with invalid memory access rather than NULL. On the palette path,
        a loaded datatype object can lose its owning pointer when source
        pixel data is also present, leaking that object and its resources.

    SEE ALSO
        DTLoadTheme(), DTReleaseScreenTheme(), DTGetImages(), DTGetElement()

*****************************************************************************/
AROS_LH3(struct DecorThemeScreen *, DTObtainScreenTheme,
    AROS_LHA(struct DecorTheme *, theme, A0),
    AROS_LHA(struct Screen *, scr, A1),
    AROS_LHA(BOOL, truecolor, D0),
    struct Library *, DecorThemeBase, 13, Decortheme)
{
    AROS_LIBFUNC_INIT
    return ObtainScreenTheme(theme, scr, truecolor);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void DTReleaseScreenTheme(

    SYNOPSIS
        struct DecorThemeScreen *dts)

    FUNCTION
        Release a screen-specific theme instance and its owned image copies.

    INPUTS
        dts - Owned DecorThemeScreen, or NULL for no operation.

    RESULT
        None.

    NOTES
        Stop all rendering and other access first. The image set and every
        pointer into dts_Elements become invalid. Do not free dts_Images or
        its containers separately.

        The source theme is not freed and the screen is not closed. Release
        all instances before freeing their source theme or closing their
        screens. No reference count, synchronization or display detachment
        is performed. Call in task context with decortheme.library open.

    SEE ALSO
        DTObtainScreenTheme(), DTFreeTheme()

*****************************************************************************/
AROS_LH1(void, DTReleaseScreenTheme,
    AROS_LHA(struct DecorThemeScreen *, dts, A0),
    struct Library *, DecorThemeBase, 14, Decortheme)
{
    AROS_LIBFUNC_INIT
    ReleaseScreenTheme(dts);
    AROS_LIBFUNC_EXIT
}
