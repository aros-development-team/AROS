/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>
#include <proto/muimaster.h>

#include <proto/alib.h>

#include <libraries/mui.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "locale.h"
#include "appearanceeditor.h"
#include "themepreview.h"

#include <dos/var.h>
#include <libraries/decortheme.h>
#include <proto/decortheme.h>

/* most image sizes of a theme offered for selection */
#define THEMESIZES_MAX  DECORTHEME_MAXSIZES

/*** Instance Data **********************************************************/

struct AppearanceEditor_DATA
{
    APTR *ae_readBuff;

    Object *ae_ThemePreview;
    Object *ae_ThemeEnable;
    Object *ae_ThemeCycle;      /* the theme ... */
    Object *ae_StyleGrp;
    Object *ae_StyleCycle;      /* ... its style, for themes that have styles ... */
    Object *ae_SizeGrp;
    Object *ae_SizeCycle;       /* ... and the size of its images */
    Object *ae_ThemePalette;
    Object *ae_OptionPalette;
    Object *ae_ThemeFont;
    Object *ae_OptionFont;
    Object *ae_ThemePointer;
    Object *ae_OptionPointer;
    Object *ae_ThemeZune;
    Object *ae_OptionZune;
    Object *ae_ThemeWand;
    Object *ae_OptionWand;

    Object *ae_ThemeDescGrp;
    Object *ae_ThemeDescA;
    Object *ae_ThemeDescB;
    
    Object *ae_CompEnable;
    Object *ae_CompBelow;
    Object *ae_CompLeft;
    Object *ae_CompRight;
    Object *ae_CompAlpha;
    Object *ae_CompScale;
    Object *ae_CompNative;

    IPTR   *ae_ThemeArray;      /* every selectable theme, as its path in THEMES: */
    IPTR    ae_ThemeActive;     /* the one selected */
    STRPTR *ae_ThemeNames;      /* themes offered by the theme cycle */
    STRPTR *ae_StyleLabels;     /* styles of the selected theme */
    STRPTR  ae_SizeLabels[THEMESIZES_MAX + 1];
    TEXT    ae_SizeText[THEMESIZES_MAX][12];
    ULONG   ae_Sizes[THEMESIZES_MAX];
    ULONG   ae_SizeCount;
    struct Hook ae_PreviewHook;
    struct Hook ae_SelectHook;
};

static    STRPTR THEMES_DEFAULT;
static    STRPTR THEMES_AMIGAOS = "AmigaOS3.x";
static    CONST_STRPTR THEMES_BASE = "THEMES:";
static    CONST_STRPTR THEMES_ENVPATH = "SYS/theme.var";
/* file holding the image size selected for the theme, beside the above */
static    CONST_STRPTR THEMES_SIZEFILE = "themesize.var";
static    CONST_STRPTR COMPOSITE_ENVPATH = "SYS/compositor.prefs";
static    CONST_STRPTR THEMES_DEFPATH = "SYS:Prefs/Presets/theme.default";
static    CONST_STRPTR THEMES_OPTZUNEPATH = "Zune/usethemeprefs";

/* NOPLANES is parsed so that a hand-edited variable still imports; the editor has no control for it */
#define COMPOSITE_PEFSTEMPLATE  "ABOVE/S,BELOW/S,LEFT/S,RIGHT/S,ALPHA/S,SCALE/S,NATIVEMODE/S,NOPLANES/S"

enum
{
    ARG_ABOVE = 0,
    ARG_BELOW,
    ARG_LEFT,
    ARG_RIGHT,
    ARG_ALPHA,
    ARG_SCALE,
    ARG_NATIVEMODE,
    ARG_NOPLANES,
    NOOFARGS
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct AppearanceEditor_DATA *data = INST_DATA(CLASS, self)

/*** Theme, style and size selection ****************************************/

/*
    Every selectable theme is an entry of ae_ThemeArray, named by its path
    in THEMES:. A theme that comes in styles has an entry for each style
    ("<theme>/<style>"), all next to each other. The theme cycle offers
    the themes and the style cycle the styles of the selected one; together
    they give the selected entry. The size cycle offers the image sizes of
    that entry.
*/

static ULONG AppearanceEditor__NameLen(CONST_STRPTR theme)
{
    CONST_STRPTR sep = strchr(theme, '/');

    return sep ? (ULONG)(sep - theme) : strlen(theme);
}

static BOOL AppearanceEditor__SameTheme(CONST_STRPTR a, CONST_STRPTR b)
{
    ULONG len = AppearanceEditor__NameLen(a);

    return (len == AppearanceEditor__NameLen(b)) && !strncmp(a, b, len);
}

/* First entry of the theme at position 'theme' of the theme cycle */
static IPTR AppearanceEditor__FirstOfTheme(struct AppearanceEditor_DATA *data, IPTR theme)
{
    IPTR index = 0, count = 0;

    while (data->ae_ThemeArray[index])
    {
        if ((index == 0) || !AppearanceEditor__SameTheme((STRPTR)data->ae_ThemeArray[index],
                (STRPTR)data->ae_ThemeArray[index - 1]))
        {
            if (count++ == theme)
                return index;
        }
        index++;
    }

    return 0;
}

/* Position in the theme cycle of the theme the entry 'index' belongs to */
static IPTR AppearanceEditor__ThemeOf(struct AppearanceEditor_DATA *data, IPTR index)
{
    IPTR i, theme = 0;

    for (i = 1; (i <= index) && data->ae_ThemeArray[i]; i++)
    {
        if (!AppearanceEditor__SameTheme((STRPTR)data->ae_ThemeArray[i],
                (STRPTR)data->ae_ThemeArray[i - 1]))
            theme++;
    }

    return theme;
}

/* The themes offered by the theme cycle: each theme once */
static STRPTR *AppearanceEditor__MakeThemeNames(IPTR *themes)
{
    STRPTR *names;
    IPTR index, count = 0;

    for (index = 0; themes[index]; index++)
        ;
    names = AllocVec(sizeof(STRPTR) * (index + 1), MEMF_CLEAR);
    if (!names)
        return NULL;

    for (index = 0; themes[index]; index++)
    {
        STRPTR theme = (STRPTR)themes[index];
        ULONG len = AppearanceEditor__NameLen(theme);

        if ((index > 0) && AppearanceEditor__SameTheme(theme, (STRPTR)themes[index - 1]))
            continue;

        if ((names[count] = AllocVec(len + 1, MEMF_CLEAR)) == NULL)
            break;
        CopyMem(theme, names[count], len);
        count++;
    }

    return names;
}

/* The image size the user selected for the theme in use; 0 if none */
static ULONG AppearanceEditor__SavedSize(void)
{
    TEXT buffer[16];
    ULONG size = 0, i;

    if (GetVar(DECORTHEME_SIZEVAR, buffer, sizeof(buffer), GVF_GLOBAL_ONLY) <= 0)
        return 0;
    for (i = 0; (buffer[i] >= '0') && (buffer[i] <= '9') && (size < 1024); i++)
        size = (size * 10) + (buffer[i] - '0');

    return size;
}

static ULONG AppearanceEditor__SelectedSize(struct AppearanceEditor_DATA *data)
{
    IPTR active = XGET(data->ae_SizeCycle, MUIA_Cycle_Active);

    return (active < data->ae_SizeCount) ? data->ae_Sizes[active] : 0;
}

/* Offers the styles of the selected theme */
static void AppearanceEditor__UpdateStyles(struct AppearanceEditor_DATA *data)
{
    static CONST_STRPTR nostyles[] = { "", NULL };
    IPTR first = AppearanceEditor__FirstOfTheme(data,
        AppearanceEditor__ThemeOf(data, data->ae_ThemeActive));
    IPTR count = 0, i;
    STRPTR *labels = NULL;

    while (data->ae_ThemeArray[first + count] &&
           AppearanceEditor__SameTheme((STRPTR)data->ae_ThemeArray[first],
                (STRPTR)data->ae_ThemeArray[first + count]))
        count++;

    /* only a theme that is listed with styles has any */
    if (strchr((STRPTR)data->ae_ThemeArray[first], '/'))
        labels = AllocVec(sizeof(STRPTR) * (count + 1), MEMF_CLEAR);

    if (labels)
    {
        for (i = 0; i < count; i++)
        {
            STRPTR theme = (STRPTR)data->ae_ThemeArray[first + i];

            labels[i] = theme + AppearanceEditor__NameLen(theme) + 1;
        }
        SET(data->ae_StyleCycle, MUIA_Cycle_Entries, (IPTR)labels);
        NNSET(data->ae_StyleCycle, MUIA_Cycle_Active, data->ae_ThemeActive - first);
    }
    else
        SET(data->ae_StyleCycle, MUIA_Cycle_Entries, (IPTR)nostyles);

    FreeVec(data->ae_StyleLabels);
    data->ae_StyleLabels = labels;

    /* a selector that is not shown must not take the keyboard focus */
    SET(data->ae_StyleCycle, MUIA_CycleChain, labels != NULL);
    SET(data->ae_StyleGrp, MUIA_ShowMe, labels != NULL);
}

/* Offers the image sizes of the selected theme, selecting 'wanted' if
   the theme has it and the theme's default size otherwise */
static void AppearanceEditor__UpdateSizes(struct AppearanceEditor_DATA *data, ULONG wanted)
{
    TEXT path[256];
    ULONG defsize = 0, i;
    IPTR active = 0;
    BOOL found = FALSE;

    data->ae_SizeCount = 0;
    if (DecorThemeBase && data->ae_ThemeArray[data->ae_ThemeActive])
    {
        snprintf(path, sizeof(path), "%s%s", THEMES_BASE, (char *)data->ae_ThemeArray[data->ae_ThemeActive]);
        data->ae_SizeCount = DTGetThemeSizes(path, data->ae_Sizes, THEMESIZES_MAX, &defsize);
    }

    for (i = 0; i < data->ae_SizeCount; i++)
    {
        snprintf(data->ae_SizeText[i], sizeof(data->ae_SizeText[i]), "%lupt", (unsigned long)data->ae_Sizes[i]);
        data->ae_SizeLabels[i] = data->ae_SizeText[i];
        if (data->ae_Sizes[i] == wanted)
        {
            active = i;
            found = TRUE;
        }
        else if (!found && (data->ae_Sizes[i] == defsize))
            active = i;
    }
    if (data->ae_SizeCount == 0)
    {
        data->ae_SizeText[0][0] = '\0';
        data->ae_SizeLabels[0] = data->ae_SizeText[0];
        i = 1;
    }
    data->ae_SizeLabels[i] = NULL;

    SET(data->ae_SizeCycle, MUIA_Cycle_Entries, (IPTR)data->ae_SizeLabels);
    NNSET(data->ae_SizeCycle, MUIA_Cycle_Active, active);

    /* nothing to choose from unless there are several sizes */
    SET(data->ae_SizeCycle, MUIA_CycleChain, data->ae_SizeCount > 1);
    SET(data->ae_SizeGrp, MUIA_ShowMe, data->ae_SizeCount > 1);
}

/* Makes the entry 'index' the selected theme, in the size 'size' if it
   has that size (0 selects its default size) */
static void AppearanceEditor__SelectTheme(struct AppearanceEditor_DATA *data, IPTR index, ULONG size)
{
    data->ae_ThemeActive = index;
    NNSET(data->ae_ThemeCycle, MUIA_Cycle_Active, AppearanceEditor__ThemeOf(data, index));
    AppearanceEditor__UpdateStyles(data);
    AppearanceEditor__UpdateSizes(data, size);
}

static void AppearanceEditor__DisableSelectors(struct AppearanceEditor_DATA *data, BOOL disable)
{
    SET(data->ae_ThemeCycle, MUIA_Disabled, disable);
    SET(data->ae_StyleCycle, MUIA_Disabled, disable);
    SET(data->ae_SizeCycle, MUIA_Disabled, disable);
}

/* Shows the selected theme in the preview, or nothing */
static void AppearanceEditor__ShowPreview(struct AppearanceEditor_DATA *data, BOOL show)
{
    TEXT path[256];

    if (show && data->ae_ThemeArray[data->ae_ThemeActive])
    {
        snprintf(path, sizeof(path), "%s%s", THEMES_BASE, (char *)data->ae_ThemeArray[data->ae_ThemeActive]);
        SetAttrs(data->ae_ThemePreview,
            MUIA_ThemePreview_Size, AppearanceEditor__SelectedSize(data),
            MUIA_ThemePreview_Theme, (IPTR)path,
            TAG_DONE);
    }
    else
        SET(data->ae_ThemePreview, MUIA_ThemePreview_Theme, (IPTR)NULL);
}

static IPTR AppearanceEditor__ListToArray(struct List *ThemeList)
{
    int         ThemeCount = 0, i = 0;
    IPTR        *ThemeArray = NULL;
    struct Node *ThemeEntry, *TmpNode;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    ForeachNode(ThemeList, ThemeEntry)
    {
        ThemeCount++;
    }

    ThemeArray = AllocVec(sizeof(IPTR) * (ThemeCount + 1), MEMF_CLEAR);
    D(bug("[AppearanceEditor] %s: Array for %d Entries @ 0x%p\n", __func__, ThemeCount, ThemeArray));

    ForeachNodeSafe(ThemeList, ThemeEntry, TmpNode)
    {
        Remove(ThemeEntry);
        D(bug("[AppearanceEditor] %s:   %02d: %s\n", __func__, i, ThemeEntry->ln_Name));
        ThemeArray[i++] = (IPTR)ThemeEntry->ln_Name;
        FreeVec(ThemeEntry);
    }

    return (IPTR)ThemeArray;
}

/* Updates gadgets to match other gadgets, pushes the selected theme to
 * the preview object and marks prefs as changed */
AROS_UFH3(static void, AppearanceEditor__PreviewHookFunc,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(Object *, self, A2),
          AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    struct AppearanceEditor_DATA *data = h->h_Data;
    char themeFileTmp[256];
    BOOL themeEnable, themeoptDisable, compEnable;
    IPTR themeActive, showDesc = FALSE;
    BPTR tmpLock;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    themeEnable = (BOOL)XGET(data->ae_ThemeEnable, MUIA_Selected);
    AppearanceEditor__DisableSelectors(data, !(themeEnable));

    themeActive = data->ae_ThemeActive;
    if (data->ae_ThemeArray[themeActive])
    {
        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/Env-Archive/Zune/global.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeZune, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/Wanderer/global.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeWand, MUIA_Disabled, themeoptDisable);
        
        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/palette.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemePalette, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/font.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeFont, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/pointer.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemePointer, MUIA_Disabled, themeoptDisable);
        
        sprintf(themeFileTmp, "THEMES:%s/Description", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Open(themeFileTmp, MODE_OLDFILE)) != BNULL)
        {
            LONG readLen = Read(tmpLock, data->ae_readBuff, 1024);
            Close(tmpLock);
            if (readLen > 0)
            {
                char *line2 = strstr((const char *)data->ae_readBuff, "\n");
                if (line2)
                    line2[0] = '\0';
                ((char *)data->ae_readBuff)[readLen] = '\0';

                showDesc = TRUE;

                SET(data->ae_ThemeDescA, MUIA_Text_Contents, data->ae_readBuff);
                SET(data->ae_ThemeDescB, MUIA_Text_Contents, &line2[1]);
            }
        }
    }
    else
    {
        SET(data->ae_ThemeZune, MUIA_Disabled, TRUE);
        SET(data->ae_ThemeWand, MUIA_Disabled, TRUE);
        
        SET(data->ae_ThemePalette, MUIA_Disabled, TRUE);
        SET(data->ae_ThemeFont, MUIA_Disabled, TRUE);
        SET(data->ae_ThemePointer, MUIA_Disabled, TRUE);
    }
    SET(data->ae_ThemeDescGrp, MUIA_ShowMe, showDesc);

    compEnable = (BOOL)XGET(data->ae_CompEnable, MUIA_Selected);
    SET(data->ae_CompBelow, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompLeft, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompRight, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompAlpha, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompScale, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompNative, MUIA_Disabled, !(compEnable) || !XGET(data->ae_CompScale, MUIA_Selected));

    AppearanceEditor__ShowPreview(data, themeEnable);

    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

/* A theme or a style was chosen with the cycles */
AROS_UFH3(static void, AppearanceEditor__SelectHookFunc,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(Object *, self, A2),
          AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    struct AppearanceEditor_DATA *data = h->h_Data;
    IPTR theme = XGET(data->ae_ThemeCycle, MUIA_Cycle_Active);

    if (theme != AppearanceEditor__ThemeOf(data, data->ae_ThemeActive))
    {
        /* another theme: its first style, in its default size */
        AppearanceEditor__SelectTheme(data, AppearanceEditor__FirstOfTheme(data, theme), 0);
    }
    else
    {
        IPTR index = AppearanceEditor__FirstOfTheme(data, theme);

        if (data->ae_StyleLabels)
            index += XGET(data->ae_StyleCycle, MUIA_Cycle_Active);
        if (index != data->ae_ThemeActive)
        {
            /* another style: keep the size if this one has it too */
            data->ae_ThemeActive = index;
            AppearanceEditor__UpdateSizes(data, AppearanceEditor__SelectedSize(data));
        }
    }

    CallHookPkt(&data->ae_PreviewHook, self, NULL);

    AROS_USERFUNC_EXIT
}

static Object *MakeCheckmark(BOOL selected)
{
    Object *obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL);

    if (obj)
    {
        SET(obj, MUIA_CycleChain, 1);
        SET(obj, MUIA_Selected, selected);
    }
    return obj;
}

#define kExallBufSize          (4096)

/*** Methods ****************************************************************/
/*
    Theme discovery

    A directory in THEMES: is a theme if decortheme.library can load it;
    directories holding themes written for another theme layout, or
    something else, are not offered. A theme that comes in several styles
    keeps every style in a sub-directory of its own, each being a
    complete theme:

        THEMES:<theme>/<style>/...

    Such styles are listed, and selected, as "<theme>/<style>".
*/

static BOOL AppearanceEditor__Exists(CONST_STRPTR theme, CONST_STRPTR part)
{
    TEXT path[256];
    BPTR lock;

    snprintf(path, sizeof(path), "THEMES:%s/%s", theme, part);
    if ((lock = Lock(path, SHARED_LOCK)) == BNULL)
        return FALSE;
    UnLock(lock);

    return TRUE;
}

/* A theme is offered when it is one the decoration can load, which is
   for decortheme.library to tell */
static BOOL AppearanceEditor__IsTheme(CONST_STRPTR theme)
{
    TEXT path[256];

    if (!DecorThemeBase)
        return AppearanceEditor__Exists(theme, "System/Config");

    snprintf(path, sizeof(path), "THEMES:%s", theme);
    return DTCheckTheme(path) != 0;
}

static void AppearanceEditor__AddTheme(struct List *themes, CONST_STRPTR theme)
{
    struct Node *entry = AllocVec(sizeof(struct Node), MEMF_CLEAR);

    if (entry)
    {
        if ((entry->ln_Name = StrDup(theme)) != NULL)
            AddTail(themes, entry);
        else
            FreeVec(entry);
    }
}

/* Adds the styles found in a theme directory; returns how many there are */
static ULONG AppearanceEditor__AddStyles(struct List *themes, CONST_STRPTR theme)
{
    struct FileInfoBlock *fib;
    TEXT path[256];
    ULONG count = 0;
    BPTR lock;

    snprintf(path, sizeof(path), "THEMES:%s", theme);
    if ((lock = Lock(path, SHARED_LOCK)) == BNULL)
        return 0;

    if ((fib = AllocDosObject(DOS_FIB, NULL)) != NULL)
    {
        if (Examine(lock, fib))
        {
            while (ExNext(lock, fib))
            {
                if (fib->fib_DirEntryType <= 0)
                    continue;

                snprintf(path, sizeof(path), "%s/%s", theme, fib->fib_FileName);
                if (AppearanceEditor__IsTheme(path))
                {
                    AppearanceEditor__AddTheme(themes, path);
                    count++;
                }
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);

    return count;
}

Object *AppearanceEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    UBYTE               *ExAllBuffer = NULL;
    BOOL                ExAllMore;

    Object              *_ThemePreviewObj;
    Object              *_ThemeEnable;
    Object              *_ThemeSelectionObj;
    Object              *_ThemeStyleGrp, *_ThemeStyleObj;
    Object              *_ThemeSizeGrp, *_ThemeSizeObj;
    STRPTR              *_ThemeNames;
    static CONST_STRPTR  _NoEntries[] = { "", NULL };

    Object              *_ThemePalettePrefsGrp, *_ThemePalettePrefsObj,
                        *_ThemeZunePrefsGrp, *_ThemeZunePrefsObj,
                        *_ThemeWandPrefsGrp, *_ThemeWandPrefsObj,
                        *_ThemeFontPrefsGrp, *_ThemeFontPrefsObj,
                        *_ThemePointerPrefsGrp, *_ThemePointerPrefsObj,
                        *_ThemeDescGrp,
                        *_ThemeDescA,
                        *_ThemeDescB;

    Object              *_CompEnable;
    Object              *_CompBelow;
    Object              *_CompLeft;
    Object              *_CompRight;
    Object              *_CompAlpha;
    Object              *_CompScale;
    Object              *_CompNative;

    struct List         _ThemesAvailable;
    struct Node         *_ThemeEntry;
    IPTR                _ThemeArray = (IPTR)NULL;

    BPTR                _ThemeLock = BNULL;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    NewList(&_ThemesAvailable);

    /* decortheme.library tells which directories are usable themes */
    ThemePreview_OpenLibs();

    // Find Available Themes
    if ((ExAllBuffer = AllocVec(kExallBufSize, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
        if (GetVar(THEMES_DEFPATH, ExAllBuffer, kExallBufSize, GVF_GLOBAL_ONLY) == -1)
            THEMES_DEFAULT = THEMES_AMIGAOS;
        else
        {
            THEMES_DEFAULT = AllocVec(strlen(ExAllBuffer) + 1, MEMF_CLEAR);
            CopyMem(ExAllBuffer, (APTR)THEMES_DEFAULT, strlen(ExAllBuffer));
        }
        
        if ((_ThemeLock = Lock(THEMES_BASE, SHARED_LOCK)) != BNULL)
        {
            struct ExAllControl  *eac;
            if ((eac = AllocDosObject(DOS_EXALLCONTROL,NULL)) != NULL)
            {
                eac->eac_LastKey = 0;
                do {
                    struct ExAllData *ead = (struct ExAllData*)ExAllBuffer;
                    ExAllMore = ExAll(_ThemeLock, ead, kExallBufSize, ED_COMMENT, eac);
                    if ((!ExAllMore) && (IoErr() != ERROR_NO_MORE_ENTRIES)) {
                        break;
                    }
                    if (eac->eac_Entries == 0) {
                        continue;
                    }
                    do {
                        if (ead->ed_Type == ST_USERDIR)
                        {
                            /* A directory is either a theme, or holds the
                               styles of a theme (e.g. "Light" and "Dark")
                               as themes of their own. */
                            if (AppearanceEditor__IsTheme(ead->ed_Name))
                                AppearanceEditor__AddTheme(&_ThemesAvailable, ead->ed_Name);
                            else
                                AppearanceEditor__AddStyles(&_ThemesAvailable, ead->ed_Name);
                        }
                        ead = ead->ed_Next;
                    } while (ead);
                } while (ExAllMore);
                FreeDosObject(DOS_EXALLCONTROL, eac);
            }
            FreeVec(ExAllBuffer);
            UnLock(_ThemeLock);
        }
        else
        {
            //TODO: Display a warning about missing themes: assign
        }
    }
    else
    {
        // Catastrophic failure - not enough memory to allocate exall buffer =S
        return (Object *)NULL;
    }

    _ThemeArray =  AppearanceEditor__ListToArray(&_ThemesAvailable);
    _ThemeNames = AppearanceEditor__MakeThemeNames((IPTR *)_ThemeArray);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, (IPTR)_(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) THEMES_ENVPATH,
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Theme",
        MUIA_PrefsEditor_CanTest, FALSE,
        MUIA_PrefsEditor_CanUse, FALSE,

        Child, (IPTR)VGroup,
            Child, (IPTR)(_ThemePreviewObj = ThemePreviewObject,
            End),
            Child, (IPTR)HGroup,

                Child, (IPTR)VGroup,
                    GroupFrameT(_(MSG_SEC_THEMING)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_ThemeEnable = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLETHEMES)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)Label1(_(MSG_SELECTEDTHEME)),
                        Child, (IPTR)(_ThemeSelectionObj = CycleObject,
                            MUIA_CycleChain, 1,
                            MUIA_Cycle_Entries, (IPTR)_ThemeNames,
                        End),
                    End,
                    Child, (IPTR)(_ThemeStyleGrp = HGroup,
                        MUIA_ShowMe, FALSE,
                        Child, (IPTR)Label1(_(MSG_SELECTEDSTYLE)),
                        Child, (IPTR)(_ThemeStyleObj = CycleObject,
                            MUIA_CycleChain, 0,
                            MUIA_Cycle_Entries, (IPTR)_NoEntries,
                        End),
                    End),
                    Child, (IPTR)(_ThemeSizeGrp = HGroup,
                        MUIA_ShowMe, FALSE,
                        Child, (IPTR)Label1(_(MSG_SELECTEDSIZE)),
                        Child, (IPTR)(_ThemeSizeObj = CycleObject,
                            MUIA_CycleChain, 0,
                            MUIA_Cycle_Entries, (IPTR)_NoEntries,
                        End),
                    End),


                    Child, (IPTR)(_ThemeDescGrp = VGroup,
                        MUIA_ShowMe, FALSE,
                        TextFrame,
                        Child, (IPTR)(_ThemeDescA = TextObject,
                            NoFrame,
                            MUIA_Text_Contents, (IPTR)"",
                            MUIA_Text_PreParse, (IPTR)"\33c",
                            MUIA_Text_SetMin, FALSE,
                        End),
                        Child, (IPTR)(_ThemeDescB = TextObject,
                            NoFrame,
                            MUIA_Text_Contents, (IPTR)"",
                            MUIA_Text_PreParse, (IPTR)"\33c\33i",
                            MUIA_Text_SetMin, FALSE,
                        End),
                    End),

                    Child, (IPTR)(RectangleObject,
                        MUIA_Rectangle_HBar, TRUE,
                        MUIA_FixHeight,      2,
                    End),

                    Child, (IPTR)(TextObject,
                        MUIA_Text_Contents, (IPTR)_(MSG_USETHEMECONFIGFOR),
                    End),
                    Child, (IPTR)(_ThemeZunePrefsGrp = HGroup,
                        Child, (IPTR)(_ThemeZunePrefsObj = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLEZUNEPREFS)),
                        Child, (IPTR)HVSpace,
                    End),
                    Child, (IPTR)(_ThemeWandPrefsGrp = HGroup,
                        Child, (IPTR)(_ThemeWandPrefsObj = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLEWANDPREFS)),
                        Child, (IPTR)HVSpace,
                    End),
                    Child, (IPTR)(_ThemePalettePrefsGrp = HGroup,
                        Child, (IPTR)(_ThemePalettePrefsObj = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLEPALETTEPREFS)),
                        Child, (IPTR)HVSpace,
                    End),
                    Child, (IPTR)(_ThemeFontPrefsGrp = HGroup,
                        Child, (IPTR)(_ThemeFontPrefsObj = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLEFONTPREFS)),
                        Child, (IPTR)HVSpace,
                    End),
                    Child, (IPTR)(_ThemePointerPrefsGrp = HGroup,
                        Child, (IPTR)(_ThemePointerPrefsObj = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(_(MSG_ENABLEPOINTERPREFS)),
                        Child, (IPTR)HVSpace,
                    End),
                    Child, (IPTR)HVSpace,
                End,

                Child, (IPTR)VGroup,
                    GroupFrameT(_(MSG_SEC_COMPOSITING)),
                    MUIA_Group_SameWidth, FALSE,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompEnable = MakeCheckmark(TRUE)),
                        Child, (IPTR)Label1(__(MSG_ENABLE_SCREEN_COMPOSITION)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompBelow = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_COMPOSITE_BELOW)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompLeft = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_COMPOSITE_LEFT)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompRight = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_COMPOSITE_RIGHT)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompAlpha = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_ENABLE_COMPOSITE_WITH_ALPHA)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompScale = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_COMPOSITE_SCALE)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(_CompNative = MakeCheckmark(FALSE)),
                        Child, (IPTR)Label1(__(MSG_COMPOSITE_NATIVEMODE)),
                        Child, (IPTR)HVSpace,
                    End,
                    Child, (IPTR)HVSpace,
                End,

            End,
        End,

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->ae_ThemePreview = _ThemePreviewObj;
        data->ae_ThemeEnable = _ThemeEnable;
        data->ae_ThemeCycle = _ThemeSelectionObj;
        data->ae_StyleGrp = _ThemeStyleGrp;
        data->ae_StyleCycle = _ThemeStyleObj;
        data->ae_SizeGrp = _ThemeSizeGrp;
        data->ae_SizeCycle = _ThemeSizeObj;
        data->ae_ThemePalette = _ThemePalettePrefsGrp;
        data->ae_OptionPalette = _ThemePalettePrefsObj;
        data->ae_ThemeFont = _ThemeFontPrefsGrp;
        data->ae_OptionFont = _ThemeFontPrefsObj;
        data->ae_ThemePointer = _ThemePointerPrefsGrp;
        data->ae_OptionPointer = _ThemePointerPrefsObj;
        data->ae_ThemeZune = _ThemeZunePrefsGrp;
        data->ae_OptionZune = _ThemeZunePrefsObj;
        data->ae_ThemeWand = _ThemeWandPrefsGrp;
        data->ae_OptionWand = _ThemeWandPrefsObj;

        data->ae_ThemeDescGrp = _ThemeDescGrp;
        data->ae_ThemeDescA = _ThemeDescA;
        data->ae_ThemeDescB = _ThemeDescB;
        
        data->ae_CompEnable = _CompEnable;
        data->ae_CompBelow = _CompBelow;
        data->ae_CompLeft = _CompLeft;
        data->ae_CompRight = _CompRight;
        data->ae_CompAlpha = _CompAlpha;
        data->ae_CompScale = _CompScale;
        data->ae_CompNative = _CompNative;

        data->ae_ThemeArray = (IPTR *)_ThemeArray;
        data->ae_ThemeNames = _ThemeNames;
        data->ae_ThemeActive = 0;

        data->ae_readBuff = AllocMem(1024, MEMF_ANY);

        data->ae_PreviewHook.h_Entry = (HOOKFUNC)AppearanceEditor__PreviewHookFunc;
        data->ae_PreviewHook.h_Data = data;

        DoMethod
        (
            data->ae_ThemeEnable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        data->ae_SelectHook.h_Entry = (HOOKFUNC)AppearanceEditor__SelectHookFunc;
        data->ae_SelectHook.h_Data = data;

        /* start out with the first theme, until the settings are read */
        if (data->ae_ThemeArray[0])
            AppearanceEditor__SelectTheme(data, 0, 0);

        DoMethod
        (
            data->ae_ThemeCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_SelectHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_StyleCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_SelectHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_SizeCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_OptionZune, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );
#if (0)
        DoMethod
        (
            data->ae_OptionWand, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );
#endif

        DoMethod
        (
            data->ae_CompEnable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_CompBelow, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_CompLeft, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_CompRight, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_CompAlpha, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );
        DoMethod
        (
            data->ae_CompScale, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );
        DoMethod
        (
            data->ae_CompNative, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );
    }

    return self;
}

IPTR AppearanceEditor__OM_DISPOSE(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    int index = 0;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    if (data->ae_readBuff)
        FreeMem(data->ae_readBuff, 1024);

    while (data->ae_ThemeArray[index] != (IPTR)NULL)
    {
        D(bug("[AppearanceEditor] %s: Freeing %02d: %s\n", __func__, index, data->ae_ThemeArray[index]));
        FreeVec((APTR)data->ae_ThemeArray[index]);
        index++;
    }
    FreeVec((APTR)data->ae_ThemeArray);

    if (data->ae_ThemeNames)
    {
        for (index = 0; data->ae_ThemeNames[index]; index++)
            FreeVec(data->ae_ThemeNames[index]);
        FreeVec(data->ae_ThemeNames);
    }
    FreeVec(data->ae_StyleLabels);

    if (THEMES_DEFAULT != THEMES_AMIGAOS)
        FreeVec(THEMES_DEFAULT);

    ThemePreview_CloseLibs();

    return DoSuperMethodA(CLASS, self, message);
}

IPTR AppearanceEditor__MUIM_PrefsEditor_Import
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_Import *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;
    BPTR fh;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    if ((fh = Open(message->filename, MODE_OLDFILE)) != BNULL)
    {
        NNSET(data->ae_ThemeEnable, MUIA_Selected, TRUE);
        AppearanceEditor__DisableSelectors(data, FALSE);
        SET(data->ae_ThemeZune, MUIA_Disabled, FALSE);
        SET(data->ae_ThemeWand, MUIA_Disabled, FALSE);
        SET(data->ae_ThemePalette, MUIA_Disabled, FALSE);
        SET(data->ae_ThemeFont, MUIA_Disabled, FALSE);
        SET(data->ae_ThemePointer, MUIA_Disabled, FALSE);
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) fh);
        Close(fh);
    }
    else
    {
        NNSET(data->ae_ThemeEnable, MUIA_Selected, FALSE);
        AppearanceEditor__DisableSelectors(data, TRUE);
        SET(data->ae_ThemeZune, MUIA_Disabled, TRUE);
        SET(data->ae_ThemeWand, MUIA_Disabled, TRUE);
        SET(data->ae_ThemePalette, MUIA_Disabled, TRUE);
        SET(data->ae_ThemeFont, MUIA_Disabled, TRUE);
        SET(data->ae_ThemePointer, MUIA_Disabled, TRUE);
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, NULL);
    }

    return success;
}

IPTR AppearanceEditor__MUIM_PrefsEditor_ImportFH (
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE, optzune = TRUE;
    TEXT importBuffer[1024];
    int index = 0;
    BOOL showDesc = FALSE;

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __func__, message->fh));

    if (message->fh)
    {
        if (FGets(message->fh, importBuffer, 1024))
        {
            char *tmpThemeFile;

            /* The theme is named relative to THEMES:, which for a style
               of a theme includes the theme ("<theme>/<style>"). */
            if (Strnicmp(importBuffer, THEMES_BASE, strlen(THEMES_BASE)) == 0)
                tmpThemeFile = importBuffer + strlen(THEMES_BASE);
            else
                tmpThemeFile = FilePart(importBuffer);

            D(bug("[AppearanceEditor] %s: Prefs Theme = '%s'\n", __func__, tmpThemeFile));

            while (data->ae_ThemeArray[index] != (IPTR)NULL)
            {
                size_t themeLen = strlen((char *)data->ae_ThemeArray[index]);

                /* the whole name has to match, not just its beginning */
                if ((strncmp((char *)data->ae_ThemeArray[index], tmpThemeFile, themeLen) == 0) &&
                    ((tmpThemeFile[themeLen] == '\0') || (tmpThemeFile[themeLen] == '\n') ||
                     (tmpThemeFile[themeLen] == '\r')))
                {
                    char themeFileTmp[256];
                    BPTR tmpLock;

                    AppearanceEditor__SelectTheme(data, index, AppearanceEditor__SavedSize());

                    sprintf(themeFileTmp, "THEMES:%s/Env-Archive/Zune/global.prefs", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
                    {
                        NNSET(data->ae_OptionZune, MUIA_Selected, FALSE);
                        SET(data->ae_ThemeZune, MUIA_Disabled, TRUE);
                    }
                    else
                    {
                        UnLock(tmpLock);
                        NNSET(data->ae_OptionZune, MUIA_Selected, TRUE);
                        SET(data->ae_ThemeZune, MUIA_Disabled, FALSE);
                    }

                    sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/Wanderer/global.prefs", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
                    {
                        NNSET(data->ae_OptionWand, MUIA_Selected, FALSE);
                        SET(data->ae_ThemeWand, MUIA_Disabled, TRUE);
                    }
                    else
                    {
                        UnLock(tmpLock);
                        NNSET(data->ae_OptionWand, MUIA_Selected, TRUE);
                        SET(data->ae_ThemeWand, MUIA_Disabled, FALSE);
                    }

                    sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/palette.prefs", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
                    {
                        NNSET(data->ae_OptionPalette, MUIA_Selected, FALSE);
                        SET(data->ae_ThemePalette, MUIA_Disabled, TRUE);
                    }
                    else
                    {
                        UnLock(tmpLock);
                        NNSET(data->ae_OptionPalette, MUIA_Selected, TRUE);
                        SET(data->ae_ThemePalette, MUIA_Disabled, FALSE);
                    }

                    sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/font.prefs", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
                    {
                        NNSET(data->ae_OptionFont, MUIA_Selected, FALSE);
                        SET(data->ae_ThemeFont, MUIA_Disabled, TRUE);
                    }
                    else
                    {
                        UnLock(tmpLock);
                        NNSET(data->ae_OptionFont, MUIA_Selected, TRUE);
                        SET(data->ae_ThemeFont, MUIA_Disabled, FALSE);
                    }

                    sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/pointer.prefs", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
                    {
                        NNSET(data->ae_OptionPointer, MUIA_Selected, FALSE);
                        SET(data->ae_ThemePointer, MUIA_Disabled, TRUE);
                    }
                    else
                    {
                        UnLock(tmpLock);
                        NNSET(data->ae_OptionPointer, MUIA_Selected, TRUE);
                        SET(data->ae_ThemePointer, MUIA_Disabled, FALSE);
                    }

                    sprintf(themeFileTmp, "THEMES:%s/Description", (char *)data->ae_ThemeArray[index]);
                    if ((tmpLock = Open(themeFileTmp, MODE_OLDFILE)) != BNULL)
                    {
                        LONG readLen = Read(tmpLock, data->ae_readBuff, 1024);
                        Close(tmpLock);
                        if (readLen > 0)
                        {
                            char *line2 = strstr((const char *)data->ae_readBuff, "\n");
                            if (line2)
                                line2[0] = '\0';
                            ((char *)data->ae_readBuff)[readLen] = '\0';

                            showDesc = TRUE;

                            SET(data->ae_ThemeDescA, MUIA_Text_Contents, data->ae_readBuff);
                            SET(data->ae_ThemeDescB, MUIA_Text_Contents, &line2[1]);
                        }
                    }
                    break;
                }
                index++;
            }
        }
    }
    SET(data->ae_ThemeDescGrp, MUIA_ShowMe, showDesc);

    if (GetVar(COMPOSITE_ENVPATH, importBuffer, 1024, GVF_GLOBAL_ONLY) != -1)
    {
        struct RDArgs *rdargs;
        IPTR CompArgs[NOOFARGS] = { 0 };

        if ((rdargs = AllocDosObjectTags(DOS_RDARGS, TAG_END)) != NULL)
        {
            rdargs->RDA_Source.CS_Buffer = importBuffer;
            rdargs->RDA_Source.CS_Length = strlen(rdargs->RDA_Source.CS_Buffer);
            rdargs->RDA_DAList = (IPTR)NULL;
            rdargs->RDA_Buffer = NULL;
            rdargs->RDA_BufSiz = 0;
            rdargs->RDA_ExtHelp = NULL;
            rdargs->RDA_Flags = 0;

            if (ReadArgs(COMPOSITE_PEFSTEMPLATE, CompArgs, rdargs) != NULL)
            {
                NNSET(data->ae_CompEnable, MUIA_Selected, (BOOL)CompArgs[ARG_ABOVE]);
                NNSET(data->ae_CompBelow, MUIA_Selected, (BOOL)CompArgs[ARG_BELOW]);
                NNSET(data->ae_CompLeft, MUIA_Selected, (BOOL)CompArgs[ARG_LEFT]);
                NNSET(data->ae_CompRight, MUIA_Selected, (BOOL)CompArgs[ARG_RIGHT]);
                NNSET(data->ae_CompAlpha, MUIA_Selected, (BOOL)CompArgs[ARG_ALPHA]);
                NNSET(data->ae_CompScale, MUIA_Selected, (BOOL)CompArgs[ARG_SCALE]);
                NNSET(data->ae_CompNative, MUIA_Selected, (BOOL)CompArgs[ARG_NATIVEMODE]);
                FreeArgs(rdargs);
            }
            FreeDosObject(DOS_RDARGS, rdargs);
        }
    }

    AppearanceEditor__ShowPreview(data, (BOOL)XGET(data->ae_ThemeEnable, MUIA_Selected));

    return success;
}

IPTR AppearanceEditor__MUIM_PrefsEditor_Export
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_Export *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    if (XGET(data->ae_ThemeEnable, MUIA_Selected))
    {
        success = DoSuperMethodA(CLASS, self, message);
    }
    else
    {
        DeleteVar(THEMES_ENVPATH, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
    }

    return success;
}

static BOOL AppearanceEditor_TimeStamp(char *buffer)
{
    struct MsgPort *mp = CreateMsgPort();
    if (mp)
    {
        struct IORequest *tr = CreateIORequest(mp, sizeof(struct IORequest));
        if (tr)
        {
            if (!OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0))
            {
                struct Library *TimerBase = (struct Library *)tr->io_Device;
                struct ClockData clockdata;
                struct timeval tv;
                char yearstr[5];

                GetSysTime(&tv);
                Amiga2Date(tv.tv_secs, &clockdata);
                sprintf(yearstr, "%u", clockdata.year);
                sprintf(buffer, "%02u%02u%s%02u%02u", clockdata.mday, clockdata.month, &yearstr[2], clockdata.hour, clockdata.min);

                D(bug("[AppearanceEditor] %s: stamp - '%s'\n", __func__, buffer);)
                
                DeleteIORequest(tr);
                DeleteMsgPort(mp);

                return TRUE;
            }
            DeleteIORequest(tr);
        }
        DeleteMsgPort(mp);
    }
    return FALSE;
}

static void AppearanceEditor_ReplacePrefs(char *sourcepath, char *destpath)
{
    BPTR srcLock, tgtLock;
    APTR copyBuff;

    D(
        bug("[AppearanceEditor] %s()\n", __func__);
        bug("[AppearanceEditor] %s: source - '%s'\n", __func__, sourcepath);
        bug("[AppearanceEditor] %s: target - '%s'\n", __func__, destpath);
    )

    /* If there is an existing file, back it up first.. */
    if ((tgtLock = Lock(destpath, ACCESS_READ)) != BNULL)
    {
        char backuppath[1024];
        char stamp[16];
        char suffix[256] = { 0 };
        char *tmpstr1, *tmpstr2 = NULL;

        NameFromLock(tgtLock, backuppath, 1024);
        UnLock(tgtLock);
        tmpstr1 = &backuppath[-1];

        D(bug("[AppearanceEditor] %s: finding suffix in 0x%p '%s'\n", __func__, &tmpstr1[1], &tmpstr1[1]);)
        while((tmpstr1 = strstr(&tmpstr1[1], ".")) != NULL)
        {
            tmpstr2 = &tmpstr1[1];
            D(bug("[AppearanceEditor] %s: -> 0x%p\n", __func__, tmpstr2);)
        }

        if (tmpstr2)
        {
            if ((tmpstr2 - backuppath) < strlen(backuppath))
            {
                sprintf(suffix, "%s", tmpstr2);
            }
            tmpstr1 = tmpstr2;
            tmpstr2 = suffix;
        }
        else
        {
            tmpstr1 = backuppath + strlen(backuppath);
            tmpstr2 = "";
        }

        if (AppearanceEditor_TimeStamp(stamp))
        {
            sprintf(tmpstr1, "%s.%s", stamp, tmpstr2);
            D(
                bug("[AppearanceEditor] %s: backing up existing file...\n", __func__);
                bug("[AppearanceEditor] %s: > '%s'\n", __func__, backuppath);
            )
            Rename(destpath, backuppath);
        }
    }
    if ((tgtLock = Open(destpath, MODE_NEWFILE)) != BNULL)
    {
        if ((srcLock = Open(sourcepath, MODE_OLDFILE)) != BNULL)
        {
            copyBuff = AllocMem(4096, MEMF_ANY);
            if (copyBuff)
            {
                LONG readlen;

                D(bug("[AppearanceEditor] %s: copying ...\n", __func__);)

                while ((readlen = Read(srcLock, copyBuff, 4096)) > 0)
                {
                    Write(tgtLock, copyBuff, readlen);
                }
                
                FreeMem(copyBuff, 4096);
            }
            Close(srcLock);
        }
        Close(tgtLock);
    }
}

IPTR AppearanceEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE, backup;
    char *exportBuffer;
    IPTR  active = 0;

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __func__, message->fh));

    if ((exportBuffer = AllocVec(1024, MEMF_CLEAR)) != NULL)
    {
        /* Check if the generic prefs editor class is just making a backup */
        TEXT sizeFile[1024];
        char *varName;

        NameFromFH(message->fh, exportBuffer, 1024);
        backup = strstr(exportBuffer, "theme.var") == NULL;

        /* The image size selected for the theme is kept in a file of
           its own, beside the one naming the theme. */
        sizeFile[0] = '\0';
        if (!backup)
        {
            strlcpy(sizeFile, exportBuffer, sizeof(sizeFile));
            varName = strstr(sizeFile, "theme.var");
            *varName = '\0';
            strlcat(sizeFile, THEMES_SIZEFILE, sizeof(sizeFile));
        }

        if (message->fh)
        {
            active = data->ae_ThemeActive;
            D(bug("[AppearanceEditor] %s: Selected Theme = [%02d] '%s'\n", __func__, active, data->ae_ThemeArray[active]));

            sprintf(exportBuffer, "%s%s", THEMES_BASE, (char *)data->ae_ThemeArray[active]);

            if (FPuts(message->fh, exportBuffer))
                success = FALSE;

            if (sizeFile[0] && AppearanceEditor__SelectedSize(data))
            {
                BPTR sizeFH = Open(sizeFile, MODE_NEWFILE);

                if (sizeFH)
                {
                    FPrintf(sizeFH, "%lu", (IPTR)AppearanceEditor__SelectedSize(data));
                    Close(sizeFH);
                }
            }

            if (!backup)
            {
                char *sourceBuffer = AllocVec(1024, MEMF_CLEAR);
                if (sourceBuffer)
                {
                    if (!(XGET(data->ae_ThemeZune, MUIA_Disabled)) && XGET(data->ae_OptionZune, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Zune config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/Env-Archive/Zune/global.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "SYS/theme.var"), "%s", "Zune/global.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                    if (!(XGET(data->ae_ThemeWand, MUIA_Disabled)) && XGET(data->ae_OptionWand, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Wanderer config with theme's version...\n", __func__);)
                        sprintf(sourceBuffer, "THEMES:%s/Env-Archive/SYS/Wanderer/global.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "Wanderer/global.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }

                    if (!(XGET(data->ae_ThemePalette, MUIA_Disabled)) && XGET(data->ae_OptionPalette, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Palette config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/Env-Archive/SYS/palette.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "palette.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                    if (!(XGET(data->ae_ThemeFont, MUIA_Disabled)) && XGET(data->ae_OptionFont, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Font config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/Env-Archive/SYS/font.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "font.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                    if (!(XGET(data->ae_ThemePointer, MUIA_Disabled)) && XGET(data->ae_OptionPointer, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Pointer config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/Env-Archive/SYS/pointer.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "pointer.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                }
            }
            // TODO: Signal Decoration to reload the theme
        }
        if (XGET(data->ae_CompEnable, MUIA_Selected) && !backup)
        {
            int ebPos;

            sprintf(exportBuffer, "ABOVE");
            ebPos = strlen(exportBuffer);
            if (XGET(data->ae_CompBelow, MUIA_Selected))
            {
                sprintf(exportBuffer + ebPos, " BELOW");
                ebPos = strlen(exportBuffer);
            }
            if (XGET(data->ae_CompLeft, MUIA_Selected))
            {
                sprintf(exportBuffer + ebPos, " LEFT");
                ebPos = strlen(exportBuffer);
            }
            if (XGET(data->ae_CompRight, MUIA_Selected))
            {
                sprintf(exportBuffer + ebPos, " RIGHT");
                ebPos = strlen(exportBuffer);
            }
            if (XGET(data->ae_CompAlpha, MUIA_Selected))
            {
                sprintf(exportBuffer + ebPos, " ALPHA");
                ebPos = strlen(exportBuffer);
            }
            if (XGET(data->ae_CompScale, MUIA_Selected))
            {
                sprintf(exportBuffer + ebPos, " SCALE");
                ebPos = strlen(exportBuffer);
                if (XGET(data->ae_CompNative, MUIA_Selected))
                {
                    sprintf(exportBuffer + ebPos, " NATIVEMODE");
                    ebPos = strlen(exportBuffer);
                }
            }
            SetVar(COMPOSITE_ENVPATH, exportBuffer, ebPos,
                GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
        }
        else if (!backup)
        {
            /*
             * Composition off: say so explicitly. A missing variable means
             * "use the AmigaOS-like default" (screens drag down) to the
             * display's compositor, an empty one enables nothing.
             */
            SetVar(COMPOSITE_ENVPATH, " ", 1, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
        }
    }
    FreeVec(exportBuffer);
    return success;
}

IPTR AppearanceEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    int index = 0;
    BOOL showDesc = FALSE;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    // Theme options
    while (data->ae_ThemeArray[index] != (IPTR)NULL)
    {
        if (!strcmp((char *)data->ae_ThemeArray[index], THEMES_DEFAULT))
        {
            char themeFileTmp[256];
            BPTR tmpLock;

            AppearanceEditor__SelectTheme(data, index, 0);
            CallHookPkt(&data->ae_PreviewHook, self, NULL);

            sprintf(themeFileTmp, "THEMES:%s/Env-Archive/Zune/global.prefs", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            {
                SET(data->ae_OptionZune, MUIA_Selected, FALSE);
                SET(data->ae_ThemeZune, MUIA_Disabled, TRUE);
            }
            else
            {
                UnLock(tmpLock);
                SET(data->ae_OptionZune, MUIA_Selected, TRUE);
                SET(data->ae_ThemeZune, MUIA_Disabled, FALSE);
            }

            sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/Wanderer/global.prefs", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            {
                SET(data->ae_OptionWand, MUIA_Selected, FALSE);
                SET(data->ae_ThemeWand, MUIA_Disabled, TRUE);
            }
            else
            {
                UnLock(tmpLock);
                SET(data->ae_OptionWand, MUIA_Selected, TRUE);
                SET(data->ae_ThemeWand, MUIA_Disabled, FALSE);
            }

            sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/palette.prefs", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            {
                SET(data->ae_OptionPalette, MUIA_Selected, FALSE);
                SET(data->ae_ThemePalette, MUIA_Disabled, TRUE);
            }
            else
            {
                UnLock(tmpLock);
                SET(data->ae_OptionPalette, MUIA_Selected, TRUE);
                SET(data->ae_ThemePalette, MUIA_Disabled, FALSE);
            }

            sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/font.prefs", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            {
                SET(data->ae_OptionFont, MUIA_Selected, FALSE);
                SET(data->ae_ThemeFont, MUIA_Disabled, TRUE);
            }
            else
            {
                UnLock(tmpLock);
                SET(data->ae_OptionFont, MUIA_Selected, TRUE);
                SET(data->ae_ThemeFont, MUIA_Disabled, FALSE);
            }

            sprintf(themeFileTmp, "THEMES:%s/Env-Archive/SYS/pointer.prefs", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            {
                SET(data->ae_OptionPointer, MUIA_Selected, FALSE);
                SET(data->ae_ThemePointer, MUIA_Disabled, TRUE);
            }
            else
            {
                UnLock(tmpLock);
                SET(data->ae_OptionPointer, MUIA_Selected, TRUE);
                SET(data->ae_ThemePointer, MUIA_Disabled, FALSE);
            }

            sprintf(themeFileTmp, "THEMES:%s/Description", (char *)data->ae_ThemeArray[index]);
            if ((tmpLock = Open(themeFileTmp, MODE_OLDFILE)) != BNULL)
            {
                LONG readLen = Read(tmpLock, data->ae_readBuff, 1024);
                Close(tmpLock);
                if (readLen > 0)
                {
                    char *line2 = strstr((const char *)data->ae_readBuff, "\n");
                    if (line2)
                        line2[0] = '\0';
                    ((char *)data->ae_readBuff)[readLen] = '\0';

                    showDesc = TRUE;

                    SET(data->ae_ThemeDescA, MUIA_Text_Contents, data->ae_readBuff);
                    SET(data->ae_ThemeDescB, MUIA_Text_Contents, &line2[1]);
                }
            }

            break;
        }
        index++;
    }
    SET(data->ae_ThemeDescGrp, MUIA_ShowMe, showDesc);

    // Compositor options
    SET(data->ae_CompEnable, MUIA_Selected, TRUE);
    SET(data->ae_CompBelow, MUIA_Selected, FALSE);
    SET(data->ae_CompLeft, MUIA_Selected, FALSE);
    SET(data->ae_CompRight, MUIA_Selected, FALSE);
    SET(data->ae_CompAlpha, MUIA_Selected, TRUE);
    SET(data->ae_CompScale, MUIA_Selected, FALSE);
    SET(data->ae_CompNative, MUIA_Selected, FALSE);

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    AppearanceEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   struct opSet *,
    MUIM_PrefsEditor_Import,      struct MUIP_PrefsEditor_Import *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_Export,      struct MUIP_PrefsEditor_Export *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

