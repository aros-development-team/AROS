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

static struct DecorTheme *LoadTheme(STRPTR path, ULONG size)
{
    struct DecorTheme *theme;

    /* themes written for another layout cannot be used */
    if (GetDecorVers(path) != DECORTHEME_THEMEVERSION)
    {
        D(bug("[decortheme] %s: '%s' is not a version %d theme\n", __func__, path, DECORTHEME_THEMEVERSION));
        return NULL;
    }

    theme = AllocVec(sizeof(struct DecorTheme), MEMF_ANY | MEMF_CLEAR);
    if (theme)
    {
        theme->dt_Config = LoadConfig(path, size);
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
}

/*****************************************************************************
    NAME
        struct DecorTheme * DTLoadTheme(

    SYNOPSIS
        STRPTR path)

    FUNCTION
        Load an independent decoration theme from a directory.

    INPUTS
        path - Non-NULL, NUL-terminated theme-directory path. The string is
            copied; it need remain valid only until return.

    RESULT
        Owned DecorTheme, or NULL if the theme is not written for the theme
        layout this library implements (see DTCheckTheme()) or allocation of
        the theme or image-set structure fails. A non-NULL result does not
        establish that all files were present or that every image is usable.

    NOTES
        The loader reads Menu/Config and System/Config beneath path, then
        loads the screen, window and menu images named by the theme layout.
        Missing configuration files leave defaults in place; unknown settings
        are ignored. Failed image loads leave NULL entries in dt_Images.
        Check each required image and its ok flag before relying on it.

        Images are looked for in the theme's image sets first, which follow
        the conventions of MUI image sets:

            Images/<size>pt/<Name>.mf0 .. .mfN  one file for each state
            Images/<size>pt/<Name>.mim          every state in one image
            Images/<size>pt/<Name>.mbr          a single state

        The set used is the one of the size the user selected for the
        theme in use (the SYS/themesize.var environment variable), or the
        theme's default size when none is selected or the theme does not
        have it. Use DTLoadThemeSize() to load a particular size. A state
        without a file of its own uses the image of the state it is derived
        from, so a theme can provide just the normal and selected images
        (.mf0 and .mf1), or one image for every state.

        The returned theme contains configuration, source images and element
        descriptors. It is not installed as the system theme. Independent
        loads create independent instances. Free it with DTFreeTheme() after
        all screen instances and users of borrowed pointers have finished.

        Call from a DOS process with decortheme.library open. File access and
        picture decoding are synchronous and have no caller timeout. Relative
        paths use the process's current directory. Use a valid, nonzero current
        directory lock; configuration loading temporarily changes that state.
        Coordinate calls with any other code sharing the same process state.

        Configuration values and image dimensions are not comprehensively
        validated. Use trusted theme files with valid geometry and image grids.

    BUGS
        Some configuration and image-loader allocations are unchecked;
        out-of-memory conditions can cause invalid memory access instead
        of returning NULL. If the original current-directory lock is zero,
        the loader can leave the process using a lock it has released.

    SEE ALSO
        DTFreeTheme(), DTGetConfig(), DTGetImages(), DTGetElement(), DTObtainScreenTheme()

*****************************************************************************/
AROS_LH1(struct DecorTheme *, DTLoadTheme,
    AROS_LHA(STRPTR, path, A0),
    struct Library *, DecorThemeBase, 6, Decortheme)
{
    AROS_LIBFUNC_INIT

    D(bug("[decortheme] %s('%s')\n", __func__, path));

    return LoadTheme(path, 0);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct DecorTheme * DTLoadThemeSize(

    SYNOPSIS
        STRPTR path, ULONG size)

    FUNCTION
        Load an independent decoration theme for one of its image sizes.

    INPUTS
        path - Non-NULL, NUL-terminated theme-directory path, as for
            DTLoadTheme().
        size - Size of the image set to use (the number in the name of an
            Images/<size>pt directory of the theme), or 0 to have it chosen
            as DTLoadTheme() does.

    RESULT
        Owned DecorTheme, or NULL under the conditions given for
        DTLoadTheme().

    NOTES
        A size the theme does not provide selects the theme's default size.
        The size in use is found in the ImageSize field of the configuration.
        Images the selected set lacks are taken from the theme's other sets,
        closest size first. Settings found in Images/<size>pt/Config replace
        those of System/Config and Menu/Config for that size.

        Use this to show a theme in a size other than the one selected for
        the theme in use, for instance when previewing a choice.

    SEE ALSO
        DTLoadTheme(), DTGetThemeSizes(), DTFreeTheme()

*****************************************************************************/
AROS_LH2(struct DecorTheme *, DTLoadThemeSize,
    AROS_LHA(STRPTR, path, A0),
    AROS_LHA(ULONG, size, D0),
    struct Library *, DecorThemeBase, 15, Decortheme)
{
    AROS_LIBFUNC_INIT

    return LoadTheme(path, size);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG DTCheckTheme(

    SYNOPSIS
        STRPTR path)

    FUNCTION
        Tell whether a directory holds a theme this library can load.

    INPUTS
        path - Non-NULL, NUL-terminated directory path.

    RESULT
        The version of the theme layout the theme is written for when the
        library supports it, otherwise 0.

    NOTES
        A theme states its layout version with "DecorVers" in its
        System/Config. A directory without that file, a theme stating no
        version and a theme written for a version other than
        DECORTHEME_THEMEVERSION all give 0, and are refused by DTLoadTheme().
        Use this to decide whether to offer a theme for selection.

        Only the configuration file is read; no images are loaded. Call from
        a DOS process.

    SEE ALSO
        DTLoadTheme(), DTGetThemeSizes()

*****************************************************************************/
AROS_LH1(ULONG, DTCheckTheme,
    AROS_LHA(STRPTR, path, A0),
    struct Library *, DecorThemeBase, 16, Decortheme)
{
    AROS_LIBFUNC_INIT

    ULONG version = GetDecorVers(path);

    return (version == DECORTHEME_THEMEVERSION) ? version : 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        ULONG DTGetThemeSizes(

    SYNOPSIS
        STRPTR path, ULONG *sizes, ULONG maxsizes, ULONG *defsize)

    FUNCTION
        List the image sizes a theme provides.

    INPUTS
        path - Non-NULL, NUL-terminated theme-directory path.
        sizes - Array receiving the sizes, smallest first, or NULL.
        maxsizes - Number of entries in sizes.
        defsize - Receives the theme's default size, or NULL.

    RESULT
        The number of sizes stored; 0 if the theme has no image sets.

    NOTES
        The sizes are the numbers in the names of the theme's
        Images/<size>pt directories. The default size is the one stated
        with "DefaultSize" in System/Config, or DECORTHEME_DEFSIZE, replaced
        by the closest size the theme provides. At most
        DECORTHEME_MAXSIZES sizes are considered.

    SEE ALSO
        DTLoadThemeSize(), DTCheckTheme()

*****************************************************************************/
AROS_LH4(ULONG, DTGetThemeSizes,
    AROS_LHA(STRPTR, path, A0),
    AROS_LHA(ULONG *, sizes, A1),
    AROS_LHA(ULONG, maxsizes, D0),
    AROS_LHA(ULONG *, defsize, A2),
    struct Library *, DecorThemeBase, 17, Decortheme)
{
    AROS_LIBFUNC_INIT

    ULONG all[DECORTHEME_MAXSIZES];
    ULONG count = FindThemeSizes(path, all, DECORTHEME_MAXSIZES);
    ULONG i;

    if (defsize)
    {
        struct DecorConfig *dc = LoadConfig(path, 0);

        *defsize = 0;
        if (dc)
        {
            *defsize = dc->DefaultSize;
            FreeConfig(dc);
        }
    }

    if (!sizes)
        return 0;
    if (count > maxsizes)
        count = maxsizes;
    for (i = 0; i < count; i++)
        sizes[i] = all[i];

    return count;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void DTFreeTheme(

    SYNOPSIS
        struct DecorTheme *theme)

    FUNCTION
        Release a loaded theme and its owned configuration and source images.

    INPUTS
        theme - Owned theme returned by DTLoadTheme(), or NULL for no operation.

    RESULT
        None.

    NOTES
        Release every screen instance with DTReleaseScreenTheme() and stop
        all rendering and other access before freeing the source theme.
        Pointers returned by DTGetConfig(), DTGetImages() and DTGetElement()
        become invalid. There is no reference count or synchronization.

        Do not separately free the theme's image set or configuration. Call
        once for each successful load, in task context with decortheme.library
        open. The function does not close a screen or change the system theme.

    SEE ALSO
        DTLoadTheme(), DTReleaseScreenTheme()

*****************************************************************************/
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

/*****************************************************************************
    NAME
        struct DecorConfig * DTGetConfig(

    SYNOPSIS
        struct DecorTheme *theme)

    FUNCTION
        Return the configuration owned by a loaded theme.

    INPUTS
        theme - Live DecorTheme, or NULL.

    RESULT
        Borrowed DecorConfig pointer, or NULL when theme is NULL.

    NOTES
        No copy or reference is acquired. Do not free the configuration or its
        ThemePath string; both remain owned by the theme. Keep the theme alive
        and serialize access with any mutation or destruction.

        Changing configuration fields does not rebuild existing images or
        element descriptors. The pointer describes the loaded configuration,
        not necessarily the currently installed system decoration.

    SEE ALSO
        DTLoadTheme(), DTFreeTheme(), DTGetElement()

*****************************************************************************/
AROS_LH1(struct DecorConfig *, DTGetConfig,
    AROS_LHA(struct DecorTheme *, theme, A0),
    struct Library *, DecorThemeBase, 8, Decortheme)
{
    AROS_LIBFUNC_INIT

    return theme ? theme->dt_Config : NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct DecorImages * DTGetImages(

    SYNOPSIS
        struct DecorTheme *theme)

    FUNCTION
        Return the source-image set owned by a loaded theme.

    INPUTS
        theme - Live DecorTheme, or NULL.

    RESULT
        Borrowed DecorImages pointer, or NULL when theme is NULL.

    NOTES
        The set contains source images, not screen-matched copies. Individual
        image pointers may be NULL; a non-NULL image also has an ok flag.
        Neither this call nor the pointer retains the theme.

        Do not pass this set to DTFreeImages() or dispose its images separately.
        Keep the theme alive while using it and serialize access with changes.
        Use DTObtainScreenTheme() for images matched to a target screen.

    SEE ALSO
        DTLoadTheme(), DTFreeTheme(), DTObtainScreenTheme()

*****************************************************************************/
AROS_LH1(struct DecorImages *, DTGetImages,
    AROS_LHA(struct DecorTheme *, theme, A0),
    struct Library *, DecorThemeBase, 9, Decortheme)
{
    AROS_LIBFUNC_INIT

    return theme ? theme->dt_Images : NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct DecoratorElement * DTGetElement(

    SYNOPSIS
        struct DecorTheme *theme, ULONG elemid)

    FUNCTION
        Return a source-theme element descriptor by identifier.

    INPUTS
        theme - Live DecorTheme, or NULL.
        elemid - DECOR_ELEM_* identifier from <libraries/decortheme.h>;
            valid values are below DECOR_NUM_ELEMENTS.

    RESULT
        Borrowed DecoratorElement pointer, or NULL when theme is NULL or the
        identifier is out of range. A valid descriptor may have no usable image.

    NOTES
        The descriptor is stored inside the theme and refers to its source
        images. Do not free it. Keep the theme alive and prevent concurrent
        mutation or destruction while accessing it.

        For screen-matched rendering, use the corresponding dts_Elements entry
        of a live DecorThemeScreen instead. Descriptor lookup does not allocate,
        render, validate image geometry or acquire a reference.

    SEE ALSO
        DTGetImages(), DTObtainScreenTheme(), DTFreeTheme()

*****************************************************************************/
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
