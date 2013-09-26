/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
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
#include <proto/muimaster.h>

#include <proto/alib.h>

#include <libraries/mui.h>

#include <stdlib.h>
#include <stdio.h>

#include "locale.h"
#include "appearanceeditor.h"
#include "themepreview.h"

/*** Instance Data **********************************************************/

struct AppearanceEditor_DATA
{
    Object *te_ThemePreview;
    Object *te_ThemeEnable;
    Object *te_ThemeChoice;
    Object *te_OptionZune;
#if (0)
    Object *te_OptionWand;
#endif
    IPTR   *te_ThemeArray;
    struct Hook te_PreviewHook;
};

static    STRPTR THEMES_DEFAULT;
static    STRPTR THEMES_AMIGAOS = "AmigaOS3.x";
static    CONST_STRPTR THEMES_BASE = "THEMES:";
static    CONST_STRPTR THEMES_ENVPATH = "SYS/theme.var";
static    CONST_STRPTR THEMES_DEFPATH = "SYS:Prefs/Presets/theme.default";
static    CONST_STRPTR THEMES_OPTZUNEPATH = "Zune/usethemeprefs";

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct AppearanceEditor_DATA *data = INST_DATA(CLASS, self)

static IPTR AppearanceEditor__ListToArray(struct List *ThemeList)
{
    int         ThemeCount = 0, i = 0;
    IPTR        *ThemeArray = NULL;
    struct Node *ThemeEntry, *TmpNode;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    ForeachNode(ThemeList, ThemeEntry)
    {
        ThemeCount++;
    }

    ThemeArray = AllocVec(sizeof(IPTR) * (ThemeCount + 1), MEMF_CLEAR);
    D(bug("[AppearanceEditor] %s: Array for %d Entries @ 0x%p\n", __PRETTY_FUNCTION__, ThemeCount, ThemeArray));

    ForeachNodeSafe(ThemeList, ThemeEntry, TmpNode)
    {
        Remove(ThemeEntry);
        D(bug("[AppearanceEditor] %s:   %02d: %s\n", __PRETTY_FUNCTION__, i, ThemeEntry->ln_Name));
        ThemeArray[i++] = (IPTR)ThemeEntry->ln_Name;
        FreeVec(ThemeEntry);
    }

    return (IPTR)ThemeArray;
}

AROS_UFH3(static void, AppearanceEditor__PreviewHookFunc,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(Object *, self, A2),
	  AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    struct AppearanceEditor_DATA *data = h->h_Data;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    if (XGET(data->te_ThemeEnable, MUIA_Selected))
    {
        SET(data->te_ThemeChoice, MUIA_Disabled, FALSE);
        SET(data->te_OptionZune, MUIA_Disabled, FALSE);
#if (0)
        SET(data->te_OptionWand, MUIA_Disabled, FALSE);
#endif
    }
    else
    {
        SET(data->te_ThemeChoice, MUIA_Disabled, TRUE);
        SET(data->te_OptionZune, MUIA_Disabled, TRUE);
#if (0)
        SET(data->te_OptionWand, MUIA_Disabled, TRUE);
#endif
    }
    SET(self, MUIA_PrefsEditor_Changed, TRUE);

    AROS_USERFUNC_EXIT
}

Object *AppearanceEditor__Checkmark(BOOL selected)
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
    CONST_STRPTR        tab_labels[3];
    UBYTE               *ExAllBuffer = NULL;
    BOOL                ExAllMore;

    Object              *_ThemePreviewObj;
    Object              *_ThemeEnable;
    Object              *_ThemeSelectionObj;
#if (0)
    Object              *_ThemeWandPrefsObj;
#endif
    Object              *_ThemeZunePrefsObj;
    Object              *_CompEnable;
    Object              *_CompBelow;
    Object              *_CompLeft;
    Object              *_CompRight;
    Object              *_CompAlpha;

    struct List         _ThemesAvailable;
    struct Node         *_ThemeEntry;
    IPTR                _ThemeArray = (IPTR)NULL;

    BPTR                _ThemeLock = BNULL;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    NewList(&_ThemesAvailable);

    tab_labels[0] = "Theme'ing";
    tab_labels[1] = "Compositing";
    tab_labels[2] = NULL;

    // Find Available Themes ...
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
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) THEMES_ENVPATH,
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Theme",

        Child, VGroup,
	    Child, (IPTR)(_ThemePreviewObj = ThemePreviewObject,
            End),
            Child, RegisterGroup(tab_labels),
                MUIA_Register_Frame, TRUE,
                Child, VGroup,
                    Child, (IPTR)ColGroup(2),
                        MUIA_Group_SameWidth, FALSE,
                        Child, (IPTR)Label1(_(MSG_ENABLETHEMES)),
                        Child, HGroup,
                            Child, (IPTR)(_ThemeEnable = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                        Child, HVSpace,
                        Child, RectangleObject,
                            MUIA_Background, MUII_FILL,
                            MUIA_FixHeight, 2,
                        End,
                        Child, (IPTR)Label1(_(MSG_SELECTEDTHEME)),
                        Child, (IPTR)(_ThemeSelectionObj = (Object *)CycleObject,
                            MUIA_CycleChain, 1,
                            MUIA_Cycle_Entries, _ThemeArray,
                        End),
                        Child, (IPTR)Label1(_(MSG_ENABLEZUNEPREFS)),
                        Child, HGroup,
                            Child, (IPTR)(_ThemeZunePrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
#if (0)
                        Child, (IPTR)Label1(_(MSG_ENABLEWANDPREFS)),
                        Child, HGroup,
                            Child, (IPTR)(_ThemeWandPrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
#endif
                    End,
                End,
                Child, VGroup,
                    Child, (IPTR)ColGroup(2),
                        MUIA_Group_SameWidth, FALSE,
                        Child, (IPTR)Label1("Enable Screen Compositing"),
                        Child, HGroup,
                            Child, (IPTR)(_CompEnable = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                        Child, (IPTR)Label1("Composite Below"),
                        Child, HGroup,
                            Child, (IPTR)(_CompBelow = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                        Child, (IPTR)Label1("Composite Left"),
                        Child, HGroup,
                            Child, (IPTR)(_CompLeft = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                        Child, (IPTR)Label1("Composite Right"),
                        Child, HGroup,
                            Child, (IPTR)(_CompRight = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                        Child, HVSpace,
                        Child, RectangleObject,
                            MUIA_Background, MUII_FILL,
                            MUIA_FixHeight, 2,
                        End,
                        Child, (IPTR)Label1("Enable Compositing with Alpha"),
                        Child, HGroup,
                            Child, (IPTR)(_CompAlpha = (Object *)AppearanceEditor__Checkmark(TRUE)),
                            Child, HVSpace,
                        End,
                    End,
                    Child, HVSpace,
                End,
            End,
        End,

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->te_ThemePreview = _ThemePreviewObj;
        data->te_ThemeEnable = _ThemeEnable;
        data->te_ThemeChoice = _ThemeSelectionObj;
        data->te_OptionZune = _ThemeZunePrefsObj;
#if (0)
        data->te_OptionWand = _ThemeWandPrefsObj;
#endif

        data->te_ThemeArray = (IPTR *)_ThemeArray;

        data->te_PreviewHook.h_Entry = (HOOKFUNC)AppearanceEditor__PreviewHookFunc;
	data->te_PreviewHook.h_Data = data;

        DoMethod
        (
            data->te_ThemeEnable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->te_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->te_ThemeChoice, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->te_PreviewHook, (IPTR)NULL
        );

        DoMethod
        (
            data->te_OptionZune, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->te_PreviewHook, (IPTR)NULL
        );
#if (0)
        DoMethod
        (
            data->te_OptionWand, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) self, 3, MUIM_CallHook, (IPTR)&data->te_PreviewHook, (IPTR)NULL
        );
#endif
    }

    return self;
}

IPTR AppearanceEditor__OM_DISPOSE(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    int index = 0;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    while (data->te_ThemeArray[index] != (IPTR)NULL)
    {
        D(bug("[AppearanceEditor] %s: Freeing %02d: %s\n", __PRETTY_FUNCTION__, index, data->te_ThemeArray[index]));
        FreeVec((APTR)data->te_ThemeArray[index]);
        index++;
    }
    FreeVec((APTR)data->te_ThemeArray);

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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    if ((fh = Open(message->filename, MODE_OLDFILE)) != BNULL)
    {
        NNSET(data->te_ThemeEnable, MUIA_Selected, TRUE);
        SET(data->te_ThemeChoice, MUIA_Disabled, FALSE);
        SET(data->te_OptionZune, MUIA_Disabled, FALSE);
#if (0)
        SET(data->te_OptionWand, MUIA_Disabled, FALSE);
#endif
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) fh);
        Close(fh);
    }
    else
    {
        NNSET(data->te_ThemeEnable, MUIA_Selected, FALSE);
        SET(data->te_ThemeChoice, MUIA_Disabled, TRUE);
        SET(data->te_OptionZune, MUIA_Disabled, TRUE);
#if (0)
        SET(data->te_OptionWand, MUIA_Disabled, TRUE);
#endif
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
    TEXT tmpThemePath[1024];
    int index = 0;

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __PRETTY_FUNCTION__, message->fh));

    if (message->fh)
    {
        if (FGets(message->fh, tmpThemePath, 1024))
        {
            char *tmpThemeFile = FilePart(tmpThemePath);

            D(bug("[AppearanceEditor] %s: Prefs Theme = '%s'\n", __PRETTY_FUNCTION__, tmpThemeFile));

            while (data->te_ThemeArray[index] != (IPTR)NULL)
            {
                if (strncmp((char *)data->te_ThemeArray[index], tmpThemeFile, strlen((char *)data->te_ThemeArray[index])) == 0)
                {
                    NNSET(data->te_ThemeChoice, MUIA_Cycle_Active, index);
                    break;
                }
                index++;
            }
        }
    }

    if (GetVar(THEMES_OPTZUNEPATH, tmpThemePath, 1, GVF_GLOBAL_ONLY) == -1)
        optzune = FALSE;

    NNSET(data->te_OptionZune, MUIA_Selected, optzune);
    
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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));    

    if (XGET(data->te_ThemeEnable, MUIA_Selected))
    {
        success = DoSuperMethodA(CLASS, self, message);
    }
    else
    {
        DeleteVar(THEMES_ENVPATH, GVF_GLOBAL_ONLY);
    }

    return success;
}

IPTR AppearanceEditor__MUIM_PrefsEditor_ExportFH
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportFH *message
)
{
    SETUP_INST_DATA;
    BOOL success = TRUE;
    IPTR  active = 0;

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __PRETTY_FUNCTION__, message->fh));

    if (message->fh)
    {
        char *tmpThemePath;

        GET(data->te_ThemeChoice, MUIA_Cycle_Active, &active);
        D(bug("[AppearanceEditor] %s: Selected Theme = [%02d] '%s'\n", __PRETTY_FUNCTION__, active, data->te_ThemeArray[active]));

        tmpThemePath = AllocVec(strlen(THEMES_BASE) + strlen((char *)data->te_ThemeArray[active]) + 1, MEMF_CLEAR);
        sprintf(tmpThemePath, "%s%s", THEMES_BASE, (char *)data->te_ThemeArray[active]);

        if (FPuts(message->fh, tmpThemePath))
            success = FALSE;

        if (XGET(data->te_OptionZune, MUIA_Selected))
        {
            sprintf(tmpThemePath, "True");
            SetVar(THEMES_OPTZUNEPATH, tmpThemePath, 4,GVF_GLOBAL_ONLY);
        }
        else
        {
            DeleteVar(THEMES_OPTZUNEPATH, GVF_GLOBAL_ONLY);
        }
        
        FreeVec(tmpThemePath);
        
        // TODO: Signal Decoration to relaod the theme
    }
    
    return success;
}

IPTR AppearanceEditor__MUIM_PrefsEditor_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    int index = 0;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    while (data->te_ThemeArray[index] != (IPTR)NULL)
    {
        if (!strcmp((char *)data->te_ThemeArray[index], THEMES_DEFAULT))
        {
            SET(data->te_ThemeChoice, MUIA_Cycle_Active, index);
            break;
        }
        index++;
    }
    SET(data->te_OptionZune, MUIA_Selected, TRUE);
#if (0)
    SET(data->te_OptionWand, MUIA_Selected, TRUE);
#endif
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

