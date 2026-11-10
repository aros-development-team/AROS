/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
    This file is part of the SystemPrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

/****** SystemPrefsWindow.mcc/MUIC_SystemPrefsWindow ************
*
*   NAME
*       MUIC_SystemPrefsWindow
*
*   SUPERCLASS
*       PrefsWindow.mcc
*
*   DESCRIPTION
*       Connect a PrefsEditor-derived content object to the PrefsWindow buttons
*       and a preferences menu. Supply that editor in one non-NULL
*       WindowContents tag; construction returns NULL if it is missing. The
*       superclass adopts it into the content hierarchy and changes the first
*       WindowContents tag to TAG_IGNORE. Keep constructor tag storage writable.
*       The configured class version is 1.3.
*       The window title defaults to the editor Name. Menu items invoke Import,
*       Export, Export with icon, SetDefaults, Test, Revert, Save, Use, and
*       Cancel. A reusable ASL file requester starts at SYS:Prefs/Presets,
*       rejects icons, and allows patterns. Failure to allocate it does not fail
*       construction; Import and Export then return FALSE.
*       Changed and Testing notifications update button states: Test requires
*       Changed, Revert requires Testing, and Save/Use require either.
*       Capability checks additionally require CanTest for Test/Revert, CanSave
*       for Save, and CanUse for Use. Cancel is not changed by this calculation.
*       Initially Test, Revert, Save, and Use are forced disabled rather than
*       initialized from current editor state.
*
*   NOTES
*       Capability restrictions apply to Disabled tags, not method dispatch. The
*       menu items are not synchronized to the disabled buttons, and direct
*       calls are not blocked. The OM_SET implementation can change caller tag
*       values to TRUE in place when a capability is absent; it does not restore
*       them. Capability changes alone have no installed notification to
*       recompute the buttons.
*       Test and Revert delegate to the editor without quitting. Successful
*       Save, Use, and Cancel send MUIV_Application_ReturnID_Quit to the
*       Application; they do not merely set Window_Open FALSE. The application
*       decides how to handle that return ID. Failures return FALSE without an
*       implemented error requester.
*       Import/Export are synchronous requester operations that can wait for the
*       user. Their refresh hook forwards IDCMP_REFRESHWINDOW to
*       Application_CheckRefresh. Use in the normal Application/Window GUI
*       lifecycle. The class owns its requester and catalog; the superclass owns
*       the editor child.
*
*   BUGS
*       The same requester is reused. Export sets ASLFR_DoSaveMode TRUE; Import
*       does not explicitly reset it. Do not assume the requester mode is
*       independently initialized for each action.
*       Import marks the editor Changed after calling its Import method even
*       when that method reports failure. Export returns the editor result;
*       optional icon loading/writing failures do not change it. No multi-file
*       transaction or rollback is provided.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Test ************
*
*   NAME
*       MUIM_PrefsWindow_Test
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Test);
*
*   FUNCTION
*       Invoke the editor Test operation.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       Return TRUE if the editor returns nonzero, otherwise FALSE.
*
*   NOTES
*       Does not request application exit. The base PrefsEditor implementation
*       exports to ENV:, then clears Changed and sets Testing on success. This
*       is temporary application of preferences, not a dry run. No CanTest check
*       is performed by this method.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Revert ************
*
*   NAME
*       MUIM_PrefsWindow_Revert
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Revert);
*
*   FUNCTION
*       Invoke the editor Revert operation.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       Return TRUE if the editor returns nonzero, otherwise FALSE.
*
*   NOTES
*       Does not request application exit. Base PrefsEditor seeks its setup
*       backup, imports it, and exports the restored settings to ENV: before
*       clearing Changed and Testing. Partial work is not rolled back if a later
*       step fails. No capability check or error requester is provided here.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Save ************
*
*   NAME
*       MUIM_PrefsWindow_Save
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Save);
*
*   FUNCTION
*       Save preferences through the editor and request application exit on
*       success.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       TRUE after editor success and sending Quit; FALSE after editor failure.
*
*   NOTES
*       Base PrefsEditor first performs Use, writing ENV: and clearing
*       Changed/Testing, then exports to ENVARC:. A later ENVARC: failure can
*       therefore leave temporary settings applied and flags cleared even though
*       Save returns FALSE. This wrapper does not check CanSave or perform
*       rollback. Quit is an Application return ID, not proof that the
*       application has terminated.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Use ************
*
*   NAME
*       MUIM_PrefsWindow_Use
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Use);
*
*   FUNCTION
*       Apply preferences through the editor and request application exit on
*       success.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       TRUE after editor success and sending Quit; FALSE after editor failure.
*
*   NOTES
*       Base PrefsEditor exports to ENV: and clears Changed/Testing on success.
*       This does not archive to ENVARC:. This wrapper does not check CanUse,
*       close the window directly, or show an error requester.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Cancel ************
*
*   NAME
*       MUIM_PrefsWindow_Cancel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Cancel);
*
*   FUNCTION
*       Cancel through the editor and request application exit on success.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       TRUE after editor success and sending Quit; FALSE after editor failure.
*
*   NOTES
*       Base PrefsEditor calls Revert only when Testing is true; otherwise it
*       returns TRUE without restoring edited values. Do not interpret this as
*       unconditional restoration of editor contents. A failed revert prevents
*       the Quit request. Window_CloseRequest reaches this action through
*       PrefsWindow.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Import ************
*
*   NAME
*       MUIM_PrefsWindow_Import
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Import);
*
*   FUNCTION
*       Ask for a preset file and import it through the editor.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       Editor Import result, or FALSE if the requester is absent/cancelled or
*       path preparation fails.
*
*   NOTES
*       After a successful selection, allocate and compose the path, call
*       PrefsEditor_Import synchronously, then set Changed TRUE even if the
*       editor returned failure. Free the temporary path after the call; an
*       overriding editor must not retain it. Base PrefsEditor opens the file,
*       delegates to ImportFH, and closes it. No automatic Test/Use or
*       application exit follows. Reused requester save mode is not explicitly
*       reset.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_Export ************
*
*   NAME
*       MUIM_PrefsWindow_Export
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Export, withIcon);
*
*   FUNCTION
*       Ask for a destination and export preferences, optionally writing an
*       icon.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       withIcon - ULONG; zero omits icon creation, nonzero requests it.
*
*   RESULT
*       Editor Export result, or FALSE if requester/path preparation fails. Icon
*       failure does not alter this result.
*
*   NOTES
*       The requester uses save mode. The selected file can be created or
*       overwritten by the editor; base PrefsEditor uses MODE_NEWFILE and may
*       create missing directories. Failure does not guarantee preservation of
*       prior file contents.
*       With nonzero withIcon and a successful export, load ENVARC:sys/def_Pref,
*       temporarily use editor IconTool as DefaultTool and ACTION=USE as the
*       only tooltype, call PutDiskObject, restore fields, and free the icon.
*       Missing default icon and failed icon write are ignored. The temporary
*       destination path is freed after all operations. No application exit
*       follows.
*
*/
/****** SystemPrefsWindow.mcc/MUIM_PrefsWindow_SetDefaults ************
*
*   NAME
*       MUIM_PrefsWindow_SetDefaults
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_SetDefaults);
*
*   FUNCTION
*       Load defaults through the editor and mark successful changes.
*
*   INPUTS
*       obj - Live SystemPrefsWindow with a valid editor and Application/Window
*       context.
*       No payload arguments.
*
*   RESULT
*       TRUE if editor SetDefaults succeeds, otherwise FALSE.
*
*   NOTES
*       Set Changed TRUE only after success. PrefsEditor declares SetDefaults
*       abstract, so the concrete editor must implement it. This wrapper does
*       not automatically apply, archive, or quit. Failed implementations can
*       leave partial editor changes; no rollback is added.
*
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/prefseditor.h>
#include <zune/prefswindow.h>

#include "systemprefswindow.h"
#include "systemprefswindow_private.h"

#include "catalogs/catalog_version.h"
#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

#include <string.h>

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct SystemPrefsWindow_DATA *data = INST_DATA(CLASS, self)

/*** Locale functions *******************************************************/
/* Return a borrowed catalog string for a valid CatCompArray index, falling
 * back to the compiled default when no catalog is open. No range validation
 * or allocation occurs; catalog-backed storage must not outlive its
 * catalog.
 */
CONST_STRPTR MSG(struct Catalog *catalog, ULONG id)
{
    if (catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    }
    else
    {
        return CatCompArray[id].cca_Str;
    }
}

#define _(id)  MSG(catalog, (id))
#define __(id) (IPTR) MSG(catalog, (id))

/*** Utility functions ******************************************************/
/* Construct a menu item from NM_BARLABEL or a readable encoded label. If
 * byte one is NUL, use the first character as shortcut and text after byte
 * two as title; otherwise use the whole string without shortcut. Require
 * sufficient valid input bytes and return the created object or NULL.
 */
Object *MakeMenuitem(CONST_STRPTR text)
{
    CONST_STRPTR title    = NULL,
                 shortcut = NULL;
    
    if (text != NM_BARLABEL && text[1] == '\0')
    {
        title    = text + 2;
        shortcut = text;
    }
    else
    {
        title    = text;
        shortcut = NULL;
    }
    
    return (Object *)MenuitemObject,
        MUIA_Menuitem_Title,      (IPTR) title,
        shortcut != NULL       ?
        MUIA_Menuitem_Shortcut :
        TAG_IGNORE,               (IPTR) shortcut,
    End;
}

/* ASL message hook: on IDCMP_REFRESHWINDOW, ask the Application stored in
 * requester UserData to check refresh. Ignore other message classes.
 * Require valid requester, message, and Application while the synchronous
 * requester is active; no preference operation or ownership change occurs.
 */
AROS_UFH3(static void, IMsgHook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct FileRequester *, req, A2),
    AROS_UFHA(struct IntuiMessage *, imsg, A1))
{
    AROS_USERFUNC_INIT

    if (imsg->Class == IDCMP_REFRESHWINDOW)
        DoMethod(req->fr_UserData, MUIM_Application_CheckRefresh);

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
/* Require a non-NULL first WindowContents editor, open the optional
 * catalog, and build the preferences menu through PrefsWindow. Allocate the
 * reusable requester without failing construction if allocation fails.
 * Store the borrowed child reference, apply tags, force four action buttons
 * disabled, and install state/menu notifications. Return object or NULL.
 */
Object *SystemPrefsWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct SystemPrefsWindow_DATA *data = NULL;
    struct TagItem *tag        = NULL;
    struct Catalog *catalog    = NULL;
    Object         *editor     = NULL;
    Object         *importMI, *exportMI, *exportIconMI, *defaultsMI,
                   *testMI, *revertMI, *saveMI, *useMI, *cancelMI;
    
    tag = FindTagItem(WindowContents, message->ops_AttrList);
    if (tag != NULL) editor = (Object *) tag->ti_Data;
    if (editor == NULL) return NULL;
    
    /*--- Initialize locale ------------------------------------------------*/
    catalog = OpenCatalog
    (
        NULL,
        "System/Classes/Zune/SystemPrefsWindow.catalog",
        OC_Version, CATALOG_VERSION,
        TAG_DONE
    );
    
    /*--- Create object ----------------------------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Title, XGET(editor, MUIA_PrefsEditor_Name),
        
        MUIA_Window_Menustrip, (IPTR) MenustripObject,
            Child, (IPTR) MenuObject,
                MUIA_Menu_Title, __(MSG_MENU_PREFS),
                Child, (IPTR)(importMI = MakeMenuitem(_(MSG_MENU_PREFS_IMPORT))),
                Child, (IPTR)(exportMI = MakeMenuitem(_(MSG_MENU_PREFS_EXPORT))),
                Child, (IPTR)(exportIconMI = MakeMenuitem(_(MSG_MENU_PREFS_EXPORT_WITH_ICON))),
                Child, (IPTR)(defaultsMI = MakeMenuitem(_(MSG_MENU_RESET_TO_DEFAULTS))),
                Child, (IPTR)MakeMenuitem(NM_BARLABEL),
                Child, (IPTR)(testMI   = MakeMenuitem(_(MSG_MENU_PREFS_TEST))),
                Child, (IPTR)(revertMI = MakeMenuitem(_(MSG_MENU_PREFS_REVERT))),
                Child, (IPTR)MakeMenuitem(NM_BARLABEL),
                Child, (IPTR)(saveMI   = MakeMenuitem(_(MSG_MENU_PREFS_SAVE))),
                Child, (IPTR)(useMI    = MakeMenuitem(_(MSG_MENU_PREFS_USE))),
                Child, (IPTR)(cancelMI = MakeMenuitem(_(MSG_MENU_PREFS_CANCEL))),
            End,
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    
    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        data->spwd_Catalog = catalog;
        data->spwd_Editor  = editor;
        data->spwd_IMsgHook.h_Entry = HookEntry;
        data->spwd_IMsgHook.h_SubEntry = (HOOKFUNC)IMsgHook;

        data->spwd_FileRequester = MUI_AllocAslRequestTags
        (
            ASL_FileRequest,
            ASLFR_RejectIcons,   TRUE,
            ASLFR_InitialDrawer, "SYS:Prefs/Presets",
            ASLFR_DoPatterns,    TRUE,
            ASLFR_IntuiMsgFunc,  (IPTR)&data->spwd_IMsgHook,
            TAG_DONE
        );

        /*-- Handle initial attribute values -------------------------------*/
        SetAttrsA(self, message->ops_AttrList);

        /*-- Setup initial button states -----------------------------------*/
        SET(self, MUIA_PrefsWindow_Test_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Revert_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Save_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Use_Disabled, TRUE);
        
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            editor, MUIM_Notify, MUIA_PrefsEditor_Changed, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_SystemPrefsWindow_UpdateButtons
        );
        DoMethod
        (
            editor, MUIM_Notify, MUIA_PrefsEditor_Testing, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_SystemPrefsWindow_UpdateButtons
        );

        DoMethod
        (
            testMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Test
        );
        DoMethod
        (
            revertMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Revert
        );
        DoMethod
        (
            saveMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Save
        );
        DoMethod
        (
            useMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Use
        );
        DoMethod
        (
            cancelMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Cancel
        );
        DoMethod
        (
            importMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Import
        );
        DoMethod
        (
            exportMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 2, MUIM_PrefsWindow_Export, FALSE
        );
        DoMethod
        (
            exportIconMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 2, MUIM_PrefsWindow_Export, TRUE
        );
        DoMethod
        (
            defaultsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_SetDefaults
        );
    }
    else
    {
        if (catalog != NULL) CloseCatalog(catalog);
    }
    
    return self;
}

/* Close the catalog and free the reusable ASL requester, then delegate
 * disposal through PrefsWindow. The editor is owned by the inherited
 * content hierarchy, not independently disposed here. Return superclass
 * disposal result.
 */
IPTR SystemPrefsWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    if (data->spwd_Catalog != NULL) CloseCatalog(data->spwd_Catalog);
    FreeAslRequest(data->spwd_FileRequester);

    return DoSuperMethodA(CLASS, self, message);
}

/* Private notification target: read editor Changed and Testing; disable
 * Test unless changed, Revert unless testing, and Save/Use unless either.
 * Send attributes through this class so capabilities can force further
 * disabling. Do not change Cancel or menu items. Return zero.
 */
IPTR SystemPrefsWindow__MUIM_SystemPrefsWindow_UpdateButtons
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    BOOL changed = XGET(data->spwd_Editor, MUIA_PrefsEditor_Changed);
    BOOL testing = XGET(data->spwd_Editor, MUIA_PrefsEditor_Testing);
    
    SET(self, MUIA_PrefsWindow_Test_Disabled, !changed);
    SET(self, MUIA_PrefsWindow_Revert_Disabled, !testing);

    SET(self, MUIA_PrefsWindow_Save_Disabled, !(changed || testing));
    SET(self, MUIA_PrefsWindow_Use_Disabled, !(changed || testing));

    return 0;
}

/* Inspect action Disabled tags and overwrite their values with TRUE if
 * editor capabilities prohibit the corresponding button. CanTest controls
 * Test/Revert, CanSave Save, CanUse Use. Caller tag values are not
 * restored. Delegate to PrefsWindow and return its result; this is not a
 * method-dispatch guard.
 */
IPTR SystemPrefsWindow__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList;
    struct TagItem *tag;
                                  
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PrefsWindow_Save_Disabled:
                if (!XGET(data->spwd_Editor, MUIA_PrefsEditor_CanSave))
                {
                    tag->ti_Data = TRUE;
                }
                break;
                
            case MUIA_PrefsWindow_Test_Disabled:
            case MUIA_PrefsWindow_Revert_Disabled:
                if (!XGET(data->spwd_Editor, MUIA_PrefsEditor_CanTest))
                {
                    tag->ti_Data = TRUE;
                }
                break;

            case MUIA_PrefsWindow_Use_Disabled:
                if (!XGET(data->spwd_Editor, MUIA_PrefsEditor_CanUse))
                {
                    tag->ti_Data = TRUE;
                }
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

/* Invoke the editor Test operation. Return TRUE if the editor returns
 * nonzero, otherwise FALSE. Does not request application exit. The base
 * PrefsEditor implementation exports to ENV:, then clears Changed and sets
 * Testing on success. This is temporary application of preferences, not a
 * dry run. No CanTest check is performed by this method.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Test
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Test))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    return TRUE;
}

/* Invoke the editor Revert operation. Return TRUE if the editor returns
 * nonzero, otherwise FALSE. Does not request application exit. Base
 * PrefsEditor seeks its setup backup, imports it, and exports the restored
 * settings to ENV: before clearing Changed and Testing. Partial work is not
 * rolled back if a later step fails. No capability check or error requester
 * is provided here.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Revert))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    return TRUE;
}

/* Save preferences through the editor and request application exit on
 * success. TRUE after editor success and sending Quit; FALSE after editor
 * failure. Base PrefsEditor first performs Use, writing ENV: and clearing
 * Changed/Testing, then exports to ENVARC:. A later ENVARC: failure can
 * therefore leave temporary settings applied and flags cleared even though
 * Save returns FALSE. This wrapper does not check CanSave or perform
 * rollback. Quit is an Application return ID, not proof that the
 * application has terminated.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Save
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Save))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    return TRUE;
}

/* Apply preferences through the editor and request application exit on
 * success. TRUE after editor success and sending Quit; FALSE after editor
 * failure. Base PrefsEditor exports to ENV: and clears Changed/Testing on
 * success. This does not archive to ENVARC:. This wrapper does not check
 * CanUse, close the window directly, or show an error requester.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Use))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    return TRUE;
}

/* Cancel through the editor and request application exit on success. TRUE
 * after editor success and sending Quit; FALSE after editor failure. Base
 * PrefsEditor calls Revert only when Testing is true; otherwise it returns
 * TRUE without restoring edited values. Do not interpret this as
 * unconditional restoration of editor contents. A failed revert prevents
 * the Quit request. Window_CloseRequest reaches this action through
 * PrefsWindow.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Cancel
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Cancel))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );

    return TRUE;
}

/* Ask for a preset file and import it through the editor. Editor Import
 * result, or FALSE if the requester is absent/cancelled or path preparation
 * fails. After a successful selection, allocate and compose the path, call
 * PrefsEditor_Import synchronously, then set Changed TRUE even if the
 * editor returned failure. Free the temporary path after the call; an
 * overriding editor must not retain it. Base PrefsEditor opens the file,
 * delegates to ImportFH, and closes it. No automatic Test/Use or
 * application exit follows. Reused requester save mode is not explicitly
 * reset.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Import
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    IPTR result = FALSE;
    struct Catalog *catalog = data->spwd_Catalog;

    if (data->spwd_FileRequester)
    {
        BOOL reqOK = MUI_AslRequestTags
        (
            data->spwd_FileRequester,
            ASLFR_TitleText, _(MSG_FILEREQ_IMPORT_TITLE),
            ASLFR_Window, XGET(self, MUIA_Window_Window),
            ASLFR_UserData, XGET(self, MUIA_ApplicationObject),
            TAG_DONE
        );
        if (reqOK)
        {
            LONG buflen = strlen(data->spwd_FileRequester->rf_Dir) +
                strlen(data->spwd_FileRequester->rf_File) + 5;
            STRPTR prefname = AllocVec(buflen, MEMF_ANY);
            if (prefname)
            {
                strcpy(prefname, data->spwd_FileRequester->rf_Dir);
                if (AddPart(prefname, data->spwd_FileRequester->rf_File, buflen))
                {
                    result = DoMethod
                    (
                        data->spwd_Editor,
                        MUIM_PrefsEditor_Import,
                        prefname
                    );
                    SET(data->spwd_Editor, MUIA_PrefsEditor_Changed, TRUE);
                }
                FreeVec(prefname);
            }
        }
        // FIXME: error reporting
    }

    return result;
}

/* Ask for a destination and export preferences, optionally writing an icon.
 * Editor Export result, or FALSE if requester/path preparation fails. Icon
 * failure does not alter this result. The requester uses save mode. The
 * selected file can be created or overwritten by the editor; base
 * PrefsEditor uses MODE_NEWFILE and may create missing directories. Failure
 * does not guarantee preservation of prior file contents. With nonzero
 * withIcon and a successful export, load ENVARC:sys/def_Pref, temporarily
 * use editor IconTool as DefaultTool and ACTION=USE as the only tooltype,
 * call PutDiskObject, restore fields, and free the icon. Missing default
 * icon and failed icon write are ignored. The temporary destination path is
 * freed after all operations. No application exit follows.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_Export
(
    Class *CLASS, Object *self, struct MUIP_PrefsWindow_Export *message
)
{
    SETUP_INST_DATA;

    IPTR result = FALSE;
    STRPTR prefname = NULL;
    struct Catalog *catalog = data->spwd_Catalog;

    if (data->spwd_FileRequester)
    {
        BOOL reqOK = MUI_AslRequestTags
        (
            data->spwd_FileRequester,
            ASLFR_TitleText, _(MSG_FILEREQ_EXPORT_TITLE),
            ASLFR_DoSaveMode, TRUE,
            ASLFR_Window, XGET(self, MUIA_Window_Window),
            ASLFR_UserData, XGET(self, MUIA_ApplicationObject),
            TAG_DONE
        );
        if (reqOK)
        {
            LONG buflen = strlen(data->spwd_FileRequester->rf_Dir) +
                strlen(data->spwd_FileRequester->rf_File) + 5;
            prefname = AllocVec(buflen, MEMF_ANY);
            if (prefname)
            {
                strcpy(prefname, data->spwd_FileRequester->rf_Dir);
                if (AddPart(prefname, data->spwd_FileRequester->rf_File, buflen))
                {
                    result = DoMethod
                    (
                        data->spwd_Editor,
                        MUIM_PrefsEditor_Export,
                        prefname
                    );
                }
            }
        }
        // FIXME: error reporting
    }

    if (result && message->withIcon)
    {
        struct DiskObject *dobj = GetDiskObject("ENVARC:sys/def_Pref");
        if (dobj)
        {
            STRPTR oldDefTool = dobj->do_DefaultTool;
            STRPTR *oldToolType = dobj->do_ToolTypes;
            STRPTR newToolType[] = {"ACTION=USE", NULL};
            dobj->do_DefaultTool = (STRPTR)XGET(data->spwd_Editor, MUIA_PrefsEditor_IconTool);
            dobj->do_ToolTypes = newToolType;
            PutDiskObject(prefname, dobj);
            dobj->do_DefaultTool = oldDefTool;
            dobj->do_ToolTypes = oldToolType;
            FreeDiskObject(dobj);
        }
    }

    FreeVec(prefname);

    return result;
}

/* Load defaults through the editor and mark successful changes. TRUE if
 * editor SetDefaults succeeds, otherwise FALSE. Set Changed TRUE only after
 * success. PrefsEditor declares SetDefaults abstract, so the concrete
 * editor must implement it. This wrapper does not automatically apply,
 * archive, or quit. Failed implementations can leave partial editor
 * changes; no rollback is added.
 */
IPTR SystemPrefsWindow__MUIM_PrefsWindow_SetDefaults
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_SetDefaults))
    {
        // FIXME: error reporting
        return FALSE;
    }
    SET(data->spwd_Editor, MUIA_PrefsEditor_Changed, TRUE);
    
    return TRUE;
}
