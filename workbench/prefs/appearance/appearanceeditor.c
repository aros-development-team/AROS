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
    Object *ae_ThemePreview;
    Object *ae_ThemeEnable;
    Object *ae_ThemeChoice;
    Object *ae_OptionZune;
#if (0)
    Object *ae_OptionWand;
#endif

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
    BOOL themeEnable, compEnable;
    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    themeEnable = (BOOL)XGET(data->ae_ThemeEnable, MUIA_Selected);
    SET(data->ae_ThemeChoice, MUIA_Disabled, !(themeEnable));
    SET(data->ae_OptionZune, MUIA_Disabled, !(themeEnable));
#if (0)
    SET(data->ae_OptionWand, MUIA_Disabled, !(themeEnable));
#endif

    compEnable = (BOOL)XGET(data->ae_CompEnable, MUIA_Selected);
    SET(data->ae_CompBelow, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompLeft, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompRight, MUIA_Disabled, !(compEnable));
    SET(data->ae_CompAlpha, MUIA_Disabled, !(compEnable));

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
        MUIA_PrefsEditor_Name, (IPTR)_(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR) THEMES_ENVPATH,
        MUIA_PrefsEditor_IconTool, (IPTR) "SYS:Prefs/Theme",

        Child, (IPTR)VGroup,
	    Child, (IPTR)(_ThemePreviewObj = ThemePreviewObject,
            End),
            Child, (IPTR)RegisterGroup(tab_labels),
                MUIA_Register_Frame, TRUE,
                Child, (IPTR)VGroup,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)HVSpace,
                        Child, (IPTR)ColGroup(2),
                            MUIA_Group_SameWidth, FALSE,
                            Child, (IPTR)Label1(_(MSG_ENABLETHEMES)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_ThemeEnable = (Object *)AppearanceEditor__Checkmark(TRUE)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)HVSpace,
                            Child, (IPTR)RectangleObject,
                                MUIA_Background, MUII_FILL,
                                MUIA_FixHeight, 2,
                            End,
                            Child, (IPTR)Label1(_(MSG_SELECTEDTHEME)),
                            Child, (IPTR)(_ThemeSelectionObj = (Object *)CycleObject,
                                MUIA_CycleChain, 1,
                                MUIA_Cycle_Entries, (IPTR)_ThemeArray,
                            End),
                            Child, (IPTR)Label1(_(MSG_ENABLEZUNEPREFS)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_ThemeZunePrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
                                Child, (IPTR)HVSpace,
                            End,
#if (0)
                            Child, (IPTR)Label1(_(MSG_ENABLEWANDPREFS)),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_ThemeWandPrefsObj = (Object *)AppearanceEditor__Checkmark(TRUE)),
                                Child, (IPTR)HVSpace,
                            End,
#endif
                        End,
                        Child, (IPTR)HVSpace,
                    End,
                End,
                Child, (IPTR)VGroup,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)HVSpace,
                        Child, (IPTR)ColGroup(2),
                            MUIA_Group_SameWidth, FALSE,
                            Child, (IPTR)Label1("Enable Screen Composition"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_CompEnable = (Object *)AppearanceEditor__Checkmark(TRUE)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label1("Composite Below"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_CompBelow = (Object *)AppearanceEditor__Checkmark(FALSE)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label1("Composite Left"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_CompLeft = (Object *)AppearanceEditor__Checkmark(FALSE)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)Label1("Composite Right"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_CompRight = (Object *)AppearanceEditor__Checkmark(FALSE)),
                                Child, (IPTR)HVSpace,
                            End,
                            Child, (IPTR)HVSpace,
                            Child, (IPTR)RectangleObject,
                                MUIA_Background, MUII_FILL,
                                MUIA_FixHeight, 2,
                            End,
                            Child, (IPTR)Label1("Enable Compositing with Alpha"),
                            Child, (IPTR)HGroup,
                                Child, (IPTR)(_CompAlpha = (Object *)AppearanceEditor__Checkmark(FALSE)),
                                Child, (IPTR)HVSpace,
                            End,
                        End,
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
        data->ae_ThemeChoice = _ThemeSelectionObj;
        data->ae_OptionZune = _ThemeZunePrefsObj;
#if (0)
        data->ae_OptionWand = _ThemeWandPrefsObj;
#endif

        data->ae_CompEnable = _CompEnable;
        data->ae_CompBelow = _CompBelow;
        data->ae_CompLeft = _CompLeft;
        data->ae_CompRight = _CompRight;
        data->ae_CompAlpha = _CompAlpha;

        data->ae_ThemeArray = (IPTR *)_ThemeArray;

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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    while (data->ae_ThemeArray[index] != (IPTR)NULL)
    {
        D(bug("[AppearanceEditor] %s: Freeing %02d: %s\n", __PRETTY_FUNCTION__, index, data->ae_ThemeArray[index]));
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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    if ((fh = Open(message->filename, MODE_OLDFILE)) != BNULL)
    {
        NNSET(data->ae_ThemeEnable, MUIA_Selected, TRUE);
        SET(data->ae_ThemeChoice, MUIA_Disabled, FALSE);
        SET(data->ae_OptionZune, MUIA_Disabled, FALSE);
#if (0)
        SET(data->ae_OptionWand, MUIA_Disabled, FALSE);
#endif
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) fh);
        Close(fh);
    }
    else
    {
        NNSET(data->ae_ThemeEnable, MUIA_Selected, FALSE);
        SET(data->ae_ThemeChoice, MUIA_Disabled, TRUE);
        SET(data->ae_OptionZune, MUIA_Disabled, TRUE);
#if (0)
        SET(data->ae_OptionWand, MUIA_Disabled, TRUE);
#endif
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

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __PRETTY_FUNCTION__, message->fh));

    if (message->fh)
    {
        if (FGets(message->fh, importBuffer, 1024))
        {
            char *tmpThemeFile = FilePart(importBuffer);

            D(bug("[AppearanceEditor] %s: Prefs Theme = '%s'\n", __PRETTY_FUNCTION__, tmpThemeFile));

            while (data->ae_ThemeArray[index] != (IPTR)NULL)
            {
                if (strncmp((char *)data->ae_ThemeArray[index], tmpThemeFile, strlen((char *)data->ae_ThemeArray[index])) == 0)
                {
                    NNSET(data->ae_ThemeChoice, MUIA_Cycle_Active, index);
                    break;
                }
                index++;
            }
        }
        if (GetVar(THEMES_OPTZUNEPATH, importBuffer, 1, GVF_GLOBAL_ONLY) == -1)
            optzune = FALSE;

        NNSET(data->ae_OptionZune, MUIA_Selected, optzune);
    }

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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));    

    if (XGET(data->ae_ThemeEnable, MUIA_Selected))
    {
        success = DoSuperMethodA(CLASS, self, message);
    }
    else
    {
        DeleteVar(THEMES_ENVPATH, GVF_GLOBAL_ONLY);
        success = DoMethod(self, MUIM_PrefsEditor_ExportFH, NULL);
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
    char *exportBuffer;
    IPTR  active = 0;

    D(bug("[AppearanceEditor] %s(FH@ 0x%p)\n", __PRETTY_FUNCTION__, message->fh));

    if ((exportBuffer = AllocVec(1024, MEMF_CLEAR)) != NULL)
    {
        if (message->fh)
        {
            GET(data->ae_ThemeChoice, MUIA_Cycle_Active, &active);
            D(bug("[AppearanceEditor] %s: Selected Theme = [%02d] '%s'\n", __PRETTY_FUNCTION__, active, data->ae_ThemeArray[active]));

            sprintf(exportBuffer, "%s%s", THEMES_BASE, (char *)data->ae_ThemeArray[active]);

            if (FPuts(message->fh, exportBuffer))
                success = FALSE;

            if (XGET(data->ae_OptionZune, MUIA_Selected))
            {
                sprintf(exportBuffer, "True");
                SetVar(THEMES_OPTZUNEPATH, exportBuffer, 4,GVF_GLOBAL_ONLY);
            }
            else
            {
                DeleteVar(THEMES_OPTZUNEPATH, GVF_GLOBAL_ONLY);
            }
            // TODO: Signal Decoration to relaod the theme
        }
        if (XGET(data->ae_CompEnable, MUIA_Selected))
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
            SetVar(COMPOSITE_ENVPATH, exportBuffer, ebPos,GVF_GLOBAL_ONLY);
        }
        else
        {
            DeleteVar(COMPOSITE_ENVPATH, GVF_GLOBAL_ONLY);
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

    D(bug("[AppearanceEditor] %s()\n", __PRETTY_FUNCTION__));

    // Theme options
    while (data->ae_ThemeArray[index] != (IPTR)NULL)
    {
        if (!strcmp((char *)data->ae_ThemeArray[index], THEMES_DEFAULT))
        {
            SET(data->ae_ThemeChoice, MUIA_Cycle_Active, index);
            break;
        }
        index++;
    }
    SET(data->ae_OptionZune, MUIA_Selected, TRUE);
#if (0)
    SET(data->ae_OptionWand, MUIA_Selected, TRUE);
#endif

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

