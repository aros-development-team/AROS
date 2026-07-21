/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library - DecorTheme instance management

    Every loaded theme is a self contained instance; the library keeps
    no global theme state so any number of themes can coexist, loaded
    by any number of clients (the system decorator, preference preview
    renderers, etc).
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

#include <aros/libcall.h>

#include <libraries/decortheme.h>
#include "decortheme_intern.h"

AROS_LH1(struct DecorTheme *, DTLoadTheme,
    AROS_LHA(STRPTR, path, A0),
    struct Library *, DecorThemeBase, 6, Decortheme)
{
    AROS_LIBFUNC_INIT

    struct DecorTheme *theme = AllocVec(sizeof(struct DecorTheme), MEMF_ANY | MEMF_CLEAR);

    D(bug("[decortheme] %s('%s')\n", __func__, path));

    if (theme)
    {
        theme->dt_Config = LoadConfig(path);
        if (theme->dt_Config)
        {
            theme->dt_Images = LoadImages(theme->dt_Config);
            if (theme->dt_Images)
            {
                InitThemeElements(theme->dt_Elements, theme->dt_Config, theme->dt_Images);
                return theme;
            }
            FreeConfig(theme->dt_Config);
        }
        FreeVec(theme);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, DTFreeTheme,
    AROS_LHA(struct DecorTheme *, theme, A0),
    struct Library *, DecorThemeBase, 7, Decortheme)
{
    AROS_LIBFUNC_INIT

    if (theme)
    {
        if (theme->dt_Images)
            FreeImages(theme->dt_Images);
        if (theme->dt_Config)
            FreeConfig(theme->dt_Config);
        FreeVec(theme);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct DecorConfig *, DTGetConfig,
    AROS_LHA(struct DecorTheme *, theme, A0),
    struct Library *, DecorThemeBase, 8, Decortheme)
{
    AROS_LIBFUNC_INIT

    return theme ? theme->dt_Config : NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(struct DecorImages *, DTGetImages,
    AROS_LHA(struct DecorTheme *, theme, A0),
    struct Library *, DecorThemeBase, 9, Decortheme)
{
    AROS_LIBFUNC_INIT

    return theme ? theme->dt_Images : NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct DecoratorElement *, DTGetElement,
    AROS_LHA(struct DecorTheme *, theme, A0),
    AROS_LHA(ULONG, elemid, D0),
    struct Library *, DecorThemeBase, 10, Decortheme)
{
    AROS_LIBFUNC_INIT

    if (!theme || elemid >= DECOR_NUM_ELEMENTS)
        return NULL;

    return &theme->dt_Elements[elemid];

    AROS_LIBFUNC_EXIT
}
