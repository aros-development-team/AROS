/*
    Copyright © 2013-2021, The AROS Development Team. All rights reserved.
    $Id$
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

/*** Instance Data **********************************************************/

struct AppearanceEditor_DATA
{
    APTR *ae_readBuff;

    Object *ae_ThemePreview;
    Object *ae_ThemeEnable;
    Object *ae_ThemeChoice;
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

    IPTR   *ae_ThemeArray;
    struct Hook ae_PreviewHook;
};

static    STRPTR THEMES_DEFAULT;
static    STRPTR THEMES_AMIGAOS = "AmigaOS3.x";
static    CONST_STRPTR THEMES_BASE = "THEMES:";
static    CONST_STRPTR THEMES_ENVPATH = "SYS/theme.var";
static    CONST_STRPTR COMPOSITE_ENVPATH = "SYS/compositor.prefs";
static    CONST_STRPTR THEMES_DEFPATH = "SYS:Prefs/Presets/theme.default";
static    CONST_STRPTR THEMES_OPTZUNEPATH = "Zune/usethemeprefs";

#define COMPOSITE_PEFSTEMPLATE  "ABOVE/S,BELOW/S,LEFT/S,RIGHT/S,ALPHA/S"

enum
{
    ARG_ABOVE = 0,
    ARG_BELOW,
    ARG_LEFT,
    ARG_RIGHT,
    ARG_ALPHA,
    NOOFARGS
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct AppearanceEditor_DATA *data = INST_DATA(CLASS, self)

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

/* N.B. This hook doesn't currently produce any preview of the selected theme.
 * It just updates gadgets to match other gadgets and marks prefs as changed */
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
    SET(data->ae_ThemeChoice, MUIA_Disabled, !(themeEnable));

    themeActive = XGET(data->ae_ThemeChoice, MUIA_Cycle_Active);
    if (data->ae_ThemeArray[themeActive])
    {
        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/global.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeZune, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/wanderer.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeWand, MUIA_Disabled, themeoptDisable);
        
        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/palette.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemePalette, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/font.prefs", (char *)data->ae_ThemeArray[themeActive]);
        if ((tmpLock = Lock(themeFileTmp, ACCESS_READ)) == BNULL)
            themeoptDisable = TRUE;
        else
            UnLock(tmpLock);
        SET(data->ae_ThemeFont, MUIA_Disabled, themeoptDisable);

        themeoptDisable = !(themeEnable);
        sprintf(themeFileTmp, "THEMES:%s/pointer.prefs", (char *)data->ae_ThemeArray[themeActive]);
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

    SET(self, MUIA_PrefsEditor_Changed, TRUE);

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
Object *AppearanceEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    UBYTE               *ExAllBuffer = NULL;
    BOOL                ExAllMore;

#if 0
    Object              *_ThemePreviewObj;
#endif
    Object              *_ThemeEnable;
    Object              *_ThemeSelectionObj;

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

    struct List         _ThemesAvailable;
    struct Node         *_ThemeEntry;
    IPTR                _ThemeArray = (IPTR)NULL;

    BPTR                _ThemeLock = BNULL;

    D(bug("[AppearanceEditor] %s()\n", __func__));

    NewList(&_ThemesAvailable);

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
                struct ExAllData *ead = (struct ExAllData*)ExAllBuffer;
                eac->eac_LastKey = 0;
                do {
                   ExAllMore = ExAll(_ThemeLock, ead, kExallBufSize, ED_COMMENT, eac);
                    if ((!ExAllMore) && (IoErr() != ERROR_NO_MORE_ENTRIES)) {
                        break;
                    }
                    if (eac->eac_Entries == 0) {
                        continue;
                    }
                    ead = (struct ExAllData *)ExAllBuffer;
                    do {
                        if (ead->ed_Type == ST_USERDIR)
                        {
                            _ThemeEntry = AllocVec(sizeof(struct Node), MEMF_CLEAR);
                            _ThemeEntry->ln_Name = StrDup(ead->ed_Name);
                            AddTail(&_ThemesAvailable, _ThemeEntry);
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

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, (IPTR)_(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) THEMES_ENVPATH,
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Theme",
        MUIA_PrefsEditor_CanTest, FALSE,
        MUIA_PrefsEditor_CanUse, FALSE,

        Child, (IPTR)VGroup,
#if 0
            Child, (IPTR)(_ThemePreviewObj = ThemePreviewObject,
            End),
#endif
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
                            MUIA_Cycle_Entries, (IPTR)_ThemeArray,
                        End),
                    End,

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
                    Child, (IPTR)HVSpace,
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
                    Child, (IPTR)HVSpace,
                End,

            End,
        End,

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

#if 0
        data->ae_ThemePreview = _ThemePreviewObj;
#endif
        data->ae_ThemeEnable = _ThemeEnable;
        data->ae_ThemeChoice = _ThemeSelectionObj;
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

        data->ae_ThemeArray = (IPTR *)_ThemeArray;

        data->ae_readBuff = AllocMem(1024, MEMF_ANY);

        data->ae_PreviewHook.h_Entry = (HOOKFUNC)AppearanceEditor__PreviewHookFunc;
        data->ae_PreviewHook.h_Data = data;

        DoMethod
        (
            data->ae_ThemeEnable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->ae_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->ae_ThemeChoice, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
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

    if (THEMES_DEFAULT != THEMES_AMIGAOS)
        FreeVec(THEMES_DEFAULT);

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
        SET(data->ae_ThemeChoice, MUIA_Disabled, FALSE);
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
        SET(data->ae_ThemeChoice, MUIA_Disabled, TRUE);
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
            char *tmpThemeFile = FilePart(importBuffer);

            D(bug("[AppearanceEditor] %s: Prefs Theme = '%s'\n", __func__, tmpThemeFile));

            while (data->ae_ThemeArray[index] != (IPTR)NULL)
            {
                if (strncmp((char *)data->ae_ThemeArray[index], tmpThemeFile, strlen((char *)data->ae_ThemeArray[index])) == 0)
                {
                    char themeFileTmp[256];
                    BPTR tmpLock;

                    NNSET(data->ae_ThemeChoice, MUIA_Cycle_Active, index);

                    sprintf(themeFileTmp, "THEMES:%s/global.prefs", (char *)data->ae_ThemeArray[index]);
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

                    sprintf(themeFileTmp, "THEMES:%s/wanderer.prefs", (char *)data->ae_ThemeArray[index]);
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

                    sprintf(themeFileTmp, "THEMES:%s/palette.prefs", (char *)data->ae_ThemeArray[index]);
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

                    sprintf(themeFileTmp, "THEMES:%s/font.prefs", (char *)data->ae_ThemeArray[index]);
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

                    sprintf(themeFileTmp, "THEMES:%s/pointer.prefs", (char *)data->ae_ThemeArray[index]);
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
                FreeArgs(rdargs);
            }
            FreeDosObject(DOS_RDARGS, rdargs);
        }
    }
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
        NameFromFH(message->fh, exportBuffer, 1024);
        backup = strstr(exportBuffer, "theme.var") == NULL;

        if (message->fh)
        {
            GET(data->ae_ThemeChoice, MUIA_Cycle_Active, &active);
            D(bug("[AppearanceEditor] %s: Selected Theme = [%02d] '%s'\n", __func__, active, data->ae_ThemeArray[active]));

            sprintf(exportBuffer, "%s%s", THEMES_BASE, (char *)data->ae_ThemeArray[active]);

            if (FPuts(message->fh, exportBuffer))
                success = FALSE;

            if (!backup)
            {
                char *sourceBuffer = AllocVec(1024, MEMF_CLEAR);
                if (sourceBuffer)
                {
                    if (!(XGET(data->ae_ThemeZune, MUIA_Disabled)) && XGET(data->ae_OptionZune, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Zune config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/global.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "SYS/theme.var"), "%s", "Zune/global.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                    if (!(XGET(data->ae_ThemeWand, MUIA_Disabled)) && XGET(data->ae_OptionWand, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Wanderer config with theme's version...\n", __func__);)
                        sprintf(sourceBuffer, "THEMES:%s/wanderer.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "Wanderer/global.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }

                    if (!(XGET(data->ae_ThemePalette, MUIA_Disabled)) && XGET(data->ae_OptionPalette, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Palette config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/palette.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "palette.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }                    
                    if (!(XGET(data->ae_ThemeFont, MUIA_Disabled)) && XGET(data->ae_OptionFont, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Font config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/font.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "palette.prefs");
                        AppearanceEditor_ReplacePrefs(sourceBuffer, exportBuffer);
                    }
                    if (!(XGET(data->ae_ThemePointer, MUIA_Disabled)) && XGET(data->ae_OptionPointer, MUIA_Selected))
                    {
                        D(bug("[AppearanceEditor] %s: replacing Pointer config with theme's version...\n", __func__);)

                        sprintf(sourceBuffer, "THEMES:%s/pointer.prefs", (char *)data->ae_ThemeArray[active]);
                        NameFromFH(message->fh, exportBuffer, 1024);
                        sprintf(strstr(exportBuffer, "theme.var"), "%s", "palette.prefs");
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
            SetVar(COMPOSITE_ENVPATH, exportBuffer, ebPos,
                GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
        }
        else
        {
            DeleteVar(COMPOSITE_ENVPATH, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
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

            SET(data->ae_ThemeChoice, MUIA_Cycle_Active, index);

            sprintf(themeFileTmp, "THEMES:%s/global.prefs", (char *)data->ae_ThemeArray[index]);
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

            sprintf(themeFileTmp, "THEMES:%s/wanderer.prefs", (char *)data->ae_ThemeArray[index]);
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

            sprintf(themeFileTmp, "THEMES:%s/palette.prefs", (char *)data->ae_ThemeArray[index]);
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

            sprintf(themeFileTmp, "THEMES:%s/font.prefs", (char *)data->ae_ThemeArray[index]);
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

            sprintf(themeFileTmp, "THEMES:%s/pointer.prefs", (char *)data->ae_ThemeArray[index]);
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

