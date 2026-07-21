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

AROS_LH1(void, DTReleaseScreenTheme,
    AROS_LHA(struct DecorThemeScreen *, dts, A0),
    struct Library *, DecorThemeBase, 14, Decortheme)
{
    AROS_LIBFUNC_INIT
    ReleaseScreenTheme(dts);
    AROS_LIBFUNC_EXIT
}
