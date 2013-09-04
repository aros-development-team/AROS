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

#include <stdlib.h>
#include <stdio.h>

#include "locale.h"
#include "appearanceeditor.h"
#include "themepreview.h"

/*** Instance Data **********************************************************/

struct AppearanceEditor_DATA
{
    Object *te_ThemePreview;
    Object *te_ThemeChoice;
    Object *te_OptionZune;
#if (0)
    Object *te_OptionWand;
#endif
    IPTR   *te_ThemeArray;
    struct Hook te_PreviewHook;
};

static    CONST_STRPTR THEMES_BASE = "THEMES:";
static    CONST_STRPTR THEMES_DEFAULT = "ice";
static    CONST_STRPTR THEMES_OPTZUNEPATH = "ENV:Zune/usethemeprefs";

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

    return ThemeArray;
}

AROS_UFH3(static void, AppearanceEditor__PreviewHookFunc,
	  AROS_UFHA(struct Hook *, h, A0),
	  AROS_UFHA(Object *, self, A2),
	  AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    struct AppearanceEditor_DATA *data = h->h_Data;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

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
    UBYTE       *ExAllBuffer = NULL;
    BOOL        ExAllMore;

    Object      *_ThemePreviewObj;
    Object      *_ThemeSelectionObj;
    Object      *_ThemeWandPrefsObj;
    Object      *_ThemeZunePrefsObj;

    struct List _ThemesAvailable;
    struct Node *_ThemeEntry;
    IPTR        _ThemeArray = NULL;

    BPTR        _ThemeLock = BNULL;

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    NewList(&_ThemesAvailable);

    // Find Available Themes ...
    if (((_ThemeLock = Lock(THEMES_BASE, SHARED_LOCK)) != BNULL) && ((ExAllBuffer = AllocVec(kExallBufSize, MEMF_CLEAR|MEMF_PUBLIC)) != NULL))
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
    }

    if (_ThemeLock)
        UnLock(_ThemeLock);

    _ThemeArray =  AppearanceEditor__ListToArray(&_ThemesAvailable);

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, _(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) "SYS/theme.var",
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Theme",

        Child, VGroup,
	    Child, (IPTR)(_ThemePreviewObj = ThemePreviewObject,
            End),
            Child, (IPTR)ColGroup(2),
                MUIA_Group_SameWidth, FALSE,
                Child, (IPTR)Label1(_(MSG_SELECTEDTHEME)),
                Child, (IPTR)(_ThemeSelectionObj = (Object *)CycleObject,
                    MUIA_CycleChain, 1,
                    MUIA_Cycle_Entries, _ThemeArray,
                End),
                Child, (IPTR)Label1(_(MSG_ENABLEZUNEPREFS)),
                Child, (IPTR)(_ThemeZunePrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
#if (0)
                Child, (IPTR)Label1(_(MSG_ENABLEWANDPREFS)),
                Child, (IPTR)(_ThemeWandPrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
#endif
            End,
        End,

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->te_ThemePreview = _ThemePreviewObj;
        data->te_ThemeChoice = _ThemeSelectionObj;
        data->te_OptionZune = _ThemeZunePrefsObj;
#if (0)
        data->te_OptionWand = _ThemeWandPrefsObj;
#endif

        data->te_ThemeArray = _ThemeArray;

        data->te_PreviewHook.h_Entry = (HOOKFUNC)AppearanceEditor__PreviewHookFunc;
	data->te_PreviewHook.h_Data = data;

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

    while (data->te_ThemeArray[index] != NULL)
    {
        D(bug("[AppearanceEditor] %s: Freeing %02d: %s\n", __PRETTY_FUNCTION__, index, data->te_ThemeArray[index]));
        FreeVec(data->te_ThemeArray[index]);
        index++;
    }
    FreeVec((APTR)data->te_ThemeArray);

    return DoSuperMethodA(CLASS, self, message);
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

            while (data->te_ThemeArray[index] != NULL)
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

        tmpThemePath = AllocVec(strlen(THEMES_BASE) + strlen(data->te_ThemeArray[active]) + 1, MEMF_CLEAR);
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

    while (data->te_ThemeArray[index] != NULL)
    {
        if (!strcmp(data->te_ThemeArray[index], THEMES_DEFAULT))
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
ZUNE_CUSTOMCLASS_5
(
    AppearanceEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    OM_DISPOSE,                   struct opSet *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);

