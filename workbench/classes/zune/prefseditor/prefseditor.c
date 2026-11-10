/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

/****** PrefsEditor.mcc/MUIC_PrefsEditor ************
*
*   NAME
*       MUIC_PrefsEditor
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Provide the common persistence and state operations for a preferences
*       editor. Subclass this class to implement ImportFH, ExportFH, and
*       SetDefaults and to build the controls for a particular preference
*       format. The configured class version is 1.2. The base class defines no
*       file format and implements none of those three abstract methods.
*       Provide Path for automatic loading and saving. During Setup, after
*       successful superclass setup, try Path relative to ENV:, then ENVARC:. If
*       both imports fail, call SetDefaults and set Changed TRUE only if
*       defaults succeed. Next, export the initial editor state to a temporary
*       backup whose name starts with T:PT. That backup remains open until
*       Cleanup and supplies Revert; it is not refreshed by Test, Use, or Save.
*       Setup still returns TRUE if importing, defaults, or backup creation
*       fails. An unavailable backup sets CanTest FALSE. An unavailable ENVARC:
*       lock or a reported write-protected disk sets CanSave FALSE. These
*       capability changes directly update instance fields without attribute
*       notifications. A failed Info query leaves CanSave unchanged.
*
*   NOTES
*       All preference operations are synchronous and can perform filesystem
*       I/O. Calls that use ENV: or ENVARC: temporarily change the calling
*       process current directory, invoke Import or Export, restore the previous
*       directory, and release the lock. Overrides must respect that lifetime
*       and must not retain the borrowed filename or file handle. Use the normal
*       Group setup/cleanup lifecycle and serialize access to editor state.
*       CanSave, CanTest, and CanUse describe capabilities for a surrounding
*       user interface; the action methods do not check them. This class does
*       not close a window, quit an application, or show an error requester.
*       SystemPrefsWindow supplies the window-level policy.
*       The public header marks Path and IconTool as protected attributes. They
*       are documented here for subclass authors. Header access comments do not
*       fully describe this implementation: OM_SET accepts Name, Path, IconTool,
*       CanTest, and CanUse as well as Changed and Testing; it does not accept
*       CanSave.
*
*   BUGS
*       Cleanup closes and deletes the backup but does not clear its stored
*       handle. Revert requires the valid backup from a successful setup phase;
*       do not call it after cleanup or when backup creation failed. Disposal
*       frees Name and Path but does not free the final IconTool copy.
*       Operations are not transactions. Import can partially change editor
*       state; Export can truncate a file before failure; Save can apply ENV:
*       settings before an ENVARC: failure. No rollback or close-error reporting
*       is provided.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIC_SystemPrefsWindow
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_Changed ************
*
*   NAME
*       MUIA_PrefsEditor_Changed
*
*   FUNCTION
*       Record whether the editor has unapplied changes.
*
*   INPUTS
*       BOOL; initialize, set, get in this implementation. Initially FALSE.
*
*   NOTES
*       Subclasses should set TRUE when controls change. Successful Test, Use,
*       and Revert set FALSE; Save first calls Use. Import and Export do not
*       update this attribute. Setup sets TRUE only when fallback SetDefaults
*       succeeds. Setting the flag alone performs no I/O. Boolean setters assign
*       the supplied value to the BOOL field. Recognized getters return TRUE and
*       write the stored value.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_Testing ************
*
*   NAME
*       MUIA_PrefsEditor_Testing
*
*   FUNCTION
*       Record whether test settings are active.
*
*   INPUTS
*       BOOL; initialize, set, get in this implementation. Initially FALSE.
*
*   NOTES
*       Test sets TRUE after exporting to ENV:. Successful Use and Revert set
*       FALSE. Cancel checks this flag and calls Revert only when it is nonzero.
*       Setting the flag alone neither applies nor restores preferences. Boolean
*       setters assign the supplied value to the BOOL field. Recognized getters
*       return TRUE and write the stored value.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_CanSave ************
*
*   NAME
*       MUIA_PrefsEditor_CanSave
*
*   FUNCTION
*       Report the locally recorded save capability.
*
*   INPUTS
*       BOOL; get. Initially TRUE. The header labels it I-G, but no local
*       initializer or setter handles it.
*
*   NOTES
*       Setup sets FALSE if ENVARC: cannot be locked or Info reports a
*       write-protected disk. Failure of Info leaves it unchanged. The field is
*       not automatically restored to TRUE on later setups, and the direct setup
*       assignment does not notify. Save does not check this flag; TRUE does not
*       guarantee that writing will succeed. Recognized getters return TRUE
*       and write the stored value.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_CanTest ************
*
*   NAME
*       MUIA_PrefsEditor_CanTest
*
*   FUNCTION
*       Record whether the interface should offer Test and Revert.
*
*   INPUTS
*       BOOL; initialize, set, get in this implementation. Initially TRUE.
*
*   NOTES
*       Setup sets FALSE directly if a backup cannot be created or exported.
*       This assignment does not notify and later successful setup does not
*       restore TRUE automatically. Setting TRUE does not create a backup.
*       Neither Test nor Revert checks this flag; callers must ensure a valid
*       setup backup before Revert. Boolean setters assign the supplied value to
*       the BOOL field. Recognized getters return TRUE and write the stored
*       value.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_CanUse ************
*
*   NAME
*       MUIA_PrefsEditor_CanUse
*
*   FUNCTION
*       Record whether the interface should offer Use.
*
*   INPUTS
*       BOOL; initialize, set, get in this implementation. Initially TRUE.
*
*   NOTES
*       The base implementation stores the supplied value without additional
*       checks. Use does not consult this flag; changing it neither writes
*       settings nor verifies that ENV: is writable. Boolean setters assign the
*       supplied value to the BOOL field. Recognized getters return TRUE and
*       write the stored value.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_Name ************
*
*   NAME
*       MUIA_PrefsEditor_Name
*
*   FUNCTION
*       Set the editor name used by a surrounding preferences window.
*
*   INPUTS
*       CONST_STRPTR; initialize, set, get in this implementation. Initially
*       NULL.
*
*   NOTES
*       SystemPrefsWindow uses this value for its initial window title.
*       Set frees the previous copy before calling StrDup. NULL clears the
*       value; allocation failure also leaves NULL and loses the previous value.
*       The setter does not report copy failure separately. Get returns a
*       borrowed pointer: do not modify or free it, retain it across replacement
*       or disposal, or pass it directly back to the same setter. Use an
*       independent copy for reassignment.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_Path ************
*
*   NAME
*       MUIA_PrefsEditor_Path
*
*   FUNCTION
*       Set the preference filename used for automatic directory-based
*       operations.
*
*   INPUTS
*       CONST_STRPTR; initialize, set, get in this implementation. Initially
*       NULL.
*
*   NOTES
*       Protected attribute for subclasses. Supply a valid path, normally
*       relative to ENV: and ENVARC:. DOS path semantics apply; an absolute path
*       is not made relative by changing the current directory. No path
*       validation occurs.
*       Set frees the previous copy before calling StrDup. NULL clears the
*       value; allocation failure also leaves NULL and loses the previous value.
*       The setter does not report copy failure separately. Get returns a
*       borrowed pointer: do not modify or free it, retain it across replacement
*       or disposal, or pass it directly back to the same setter. Use an
*       independent copy for reassignment.
*
*/
/****** PrefsEditor.mcc/MUIA_PrefsEditor_IconTool ************
*
*   NAME
*       MUIA_PrefsEditor_IconTool
*
*   FUNCTION
*       Set the default tool for icons exported by SystemPrefsWindow.
*
*   INPUTS
*       CONST_STRPTR; initialize, set, get in this implementation. Initially
*       NULL.
*
*   NOTES
*       Protected attribute for subclasses. It does not affect preference
*       serialization. SystemPrefsWindow uses it only for its optional icon
*       export. The final copy is not freed by this class at disposal.
*       Set frees the previous copy before calling StrDup. NULL clears the
*       value; allocation failure also leaves NULL and loses the previous value.
*       The setter does not report copy failure separately. Get returns a
*       borrowed pointer: do not modify or free it, retain it across replacement
*       or disposal, or pass it directly back to the same setter. Use an
*       independent copy for reassignment.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Import ************
*
*   NAME
*       MUIM_PrefsEditor_Import
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Import, (IPTR)filename);
*
*   FUNCTION
*       Import preferences from a file.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       filename - Non-NULL readable, NUL-terminated DOS filename.
*
*   RESULT
*       FALSE if Open fails; otherwise the ImportFH result converted to BOOL.
*
*   NOTES
*       Open with MODE_OLDFILE, call ImportFH with the handle, then close it.
*       The handle is borrowed for the duration of the call. The base method
*       does not set Changed or Testing. It ignores the Close result and
*       provides no rollback of partial editor changes.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Export ************
*
*   NAME
*       MUIM_PrefsEditor_Export
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Export, (IPTR)filename);
*
*   FUNCTION
*       Export preferences to a file.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       filename - Non-NULL readable, NUL-terminated DOS filename.
*
*   RESULT
*       FALSE if the file cannot be opened; otherwise the ExportFH result
*       converted to BOOL.
*
*   NOTES
*       Open with MODE_NEWFILE, creating or truncating the destination before
*       ExportFH runs. If Open reports ERROR_OBJECT_NOT_FOUND, duplicate the
*       filename, attempt to create slash-delimited parent directories, and
*       retry Open. The directory-helper result is ignored; earlier directories
*       can remain after a later failure. Close the handle after ExportFH,
*       ignoring Close errors. No atomic replacement or rollback is provided.
*       Changed and Testing are not updated.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Test ************
*
*   NAME
*       MUIM_PrefsEditor_Test
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Test);
*
*   FUNCTION
*       Apply the current editor settings temporarily.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       No payload arguments.
*
*   RESULT
*       TRUE after successful export to ENV:; FALSE otherwise.
*
*   NOTES
*       Export Path relative to ENV:. On success set Changed FALSE, then Testing
*       TRUE. This writes settings, not a dry run. It does not update the setup
*       backup or check CanTest. On failure the flags are not changed by this
*       base method, but a partial file can remain.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Revert ************
*
*   NAME
*       MUIM_PrefsEditor_Revert
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Revert);
*
*   FUNCTION
*       Restore the settings captured during Setup.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       No payload arguments; a valid open setup backup is required.
*
*   RESULT
*       TRUE if seeking, importing the backup, and exporting to ENV: all
*       succeed; FALSE otherwise.
*
*   NOTES
*       Seek the backup to offset zero, call ImportFH, then export Path relative
*       to ENV:. On complete success set Changed FALSE, then Testing FALSE. Stop
*       at the first failure. If the final export fails, the editor may already
*       contain the backup settings while flags retain their old values. There
*       is no CanTest or valid-handle check before Seek. The backup is the setup
*       snapshot, not the most recent Test or Save.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Save ************
*
*   NAME
*       MUIM_PrefsEditor_Save
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Save);
*
*   FUNCTION
*       Apply settings and archive them.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       No payload arguments.
*
*   RESULT
*       TRUE only if Use and the subsequent ENVARC: export both succeed; FALSE
*       otherwise.
*
*   NOTES
*       Call Use first, then export Path relative to ENVARC:. If archiving fails
*       after Use succeeds, ENV: has already been written and Changed/Testing
*       cleared. Neither those effects nor a partial archive are rolled back. No
*       CanSave check, application exit, or error requester is provided.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Use ************
*
*   NAME
*       MUIM_PrefsEditor_Use
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Use);
*
*   FUNCTION
*       Apply settings without archiving them.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       No payload arguments.
*
*   RESULT
*       TRUE after successful export to ENV:; FALSE otherwise.
*
*   NOTES
*       Export Path relative to ENV:. On success set Changed FALSE, then Testing
*       FALSE. Do not update the setup backup or write ENVARC:. CanUse is not
*       checked. Export failure does not cause a base-class flag update but can
*       leave a partial destination file.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_Cancel ************
*
*   NAME
*       MUIM_PrefsEditor_Cancel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_Cancel);
*
*   FUNCTION
*       Revert active test settings when requested by the Testing flag.
*
*   INPUTS
*       obj - Live PrefsEditor-derived object implementing the abstract
*       operations.
*       No payload arguments.
*
*   RESULT
*       The Revert result if Testing is nonzero; TRUE otherwise.
*
*   NOTES
*       If Testing is false, return immediately without restoring edited
*       controls, changing flags, or writing files. If Testing is true, Revert
*       requires the valid setup backup. Cancel does not itself close a window
*       or quit an application.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_ImportFH ************
*
*   NAME
*       MUIM_PrefsEditor_ImportFH
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_ImportFH, (IPTR)fh);
*
*   FUNCTION
*       Implement reading preferences from an open file handle.
*
*   INPUTS
*       obj - Concrete subclass implementing this abstract method.
*       fh - Valid DOS BPTR, borrowed for this synchronous call.
*
*   RESULT
*       The base class defines no result because it has no handler.
*       Implementations must return nonzero for success and zero for failure to
*       satisfy the base callers.
*
*   NOTES
*       Read at the supplied handle position and update the editor. Import
*       supplies a newly opened file; Revert seeks the backup to zero first. Do
*       not close or retain the handle. Define format validation and
*       partial-read behavior in the subclass. The file format and supported
*       preference values belong to the concrete subclass.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_ExportFH ************
*
*   NAME
*       MUIM_PrefsEditor_ExportFH
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_ExportFH, (IPTR)fh);
*
*   FUNCTION
*       Implement writing preferences to an open file handle.
*
*   INPUTS
*       obj - Concrete subclass implementing this abstract method.
*       fh - Valid DOS BPTR, borrowed for this synchronous call.
*
*   RESULT
*       The base class defines no result because it has no handler.
*       Implementations must return nonzero for success and zero for failure to
*       satisfy the base callers.
*
*   NOTES
*       Write the editor state at the supplied handle position. Export supplies
*       a newly opened or truncated file; Setup supplies the temporary backup.
*       Do not close or retain the handle. Report serialization/write failures
*       to the caller; the base caller does not verify content or Close errors.
*       The file format and supported preference values belong to the concrete
*       subclass.
*
*/
/****** PrefsEditor.mcc/MUIM_PrefsEditor_SetDefaults ************
*
*   NAME
*       MUIM_PrefsEditor_SetDefaults
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsEditor_SetDefaults);
*
*   FUNCTION
*       Implement restoring default values in the editor.
*
*   INPUTS
*       obj - Concrete subclass implementing this abstract method.
*       No payload arguments.
*
*   RESULT
*       The base class defines no result because it has no handler.
*       Implementations must return nonzero for success and zero for failure to
*       satisfy the base callers.
*
*   NOTES
*       Initialize the concrete editor controls and state. Setup calls this only
*       after both ENV: and ENVARC: imports fail and marks Changed TRUE only
*       after success. The abstract declaration alone performs no
*       initialization. Direct callers determine any subsequent flag changes or
*       persistence. The file format and supported preference values belong to
*       the concrete subclass.
*
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include "support.h"
#include "prefseditor.h"
#include "prefseditor_private.h"

#include <aros/debug.h>

/*** Macros and defines *****************************************************/
#define SETUP_INST_DATA struct PrefsEditor_DATA *data = INST_DATA(CLASS, self)

#define ENV    ((IPTR) "ENV:")
#define ENVARC ((IPTR) "ENVARC:")

/*** Methods ****************************************************************/
/* Delegate Group construction, initialize CanSave/CanTest/CanUse TRUE, and
 * replay caller tags through OM_SET. Other fields start cleared with
 * instance allocation. Return the object or NULL after superclass failure.
 * Abstract serialization/defaults are supplied by subclasses.
 */
Object *PrefsEditor__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg)message);

    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        /*-- Set defaults --------------------------------------------------*/
        data->ped_CanSave = TRUE;
        data->ped_CanTest = TRUE;
        data->ped_CanUse = TRUE;

        /*-- Handle initial attribute values -------------------------------*/
        SetAttrsA(self, message->ops_AttrList);
        
    }
    
    return self;
}

/* Require normal Group setup. After superclass success, try ENV then ENVARC
 * import, then defaults; mark Changed only for successful defaults. Export
 * a setup backup to T:PT plus a unique ID and keep it open. Remove
 * incomplete backup and disable CanTest on failure. Disable CanSave for
 * failed ENVARC lock or reported write protection. Return TRUE despite
 * these local failures; direct capability assignments do not notify.
 */
IPTR PrefsEditor__MUIM_Setup
(
    Class *CLASS, Object *self, struct MUIP_Setup *message
)
{
    SETUP_INST_DATA;
    BPTR lock;
    
    if (!DoSuperMethodA(CLASS, self, (Msg) message)) return FALSE;
    
    /*-- Load preferences --------------------------------------------------*/
    if (!DoMethod(self, MUIM_PrefsEditor_ImportFromDirectory, ENV))
    {
        if (!DoMethod(self, MUIM_PrefsEditor_ImportFromDirectory, ENVARC))
        {
            if (DoMethod(self, MUIM_PrefsEditor_SetDefaults))
            {
                SET(self, MUIA_PrefsEditor_Changed, TRUE);
            }
        }
    }
    
    /*-- Store backup of initial preferences -------------------------------*/
    data->ped_BackupFH = CreateTemporary(data->ped_BackupPath, BACKUP_PREFIX);
    if (data->ped_BackupFH != BNULL)
    {
        if
        (
            !DoMethod
            (
                self, MUIM_PrefsEditor_ExportFH, (IPTR) data->ped_BackupFH
            )
        )
        {
            /* Remove the incomplete backup file */
            Close(data->ped_BackupFH);
            DeleteFile(data->ped_BackupPath);
            data->ped_BackupFH = BNULL;
        }
    }
    
    if (data->ped_BackupFH == BNULL)
    {
        data->ped_CanTest = FALSE;
    }
    
    /*-- Completely disable save if ENVARC: is write-protected -------------*/
    lock = Lock("ENVARC:", SHARED_LOCK);
    if (lock != BNULL)
    {
        struct InfoData id;
        
        if (Info(lock, &id))
        {
            if (id.id_DiskState == ID_WRITE_PROTECTED)
            {
                data->ped_CanSave = FALSE;
            }
        }
        
        UnLock(lock);
    }
    else
    {
        data->ped_CanSave = FALSE;
    }

    return TRUE;
}

/* Close and delete the setup backup when its stored handle is nonzero, then
 * return superclass Cleanup result. Do not clear the stored handle. Require
 * a matched lifecycle; callers must not use the backup after cleanup.
 * Close/DeleteFile errors are ignored.
 */
IPTR PrefsEditor__MUIM_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    SETUP_INST_DATA;
    
    if (data->ped_BackupFH != BNULL)
    {
        Close(data->ped_BackupFH);
        DeleteFile(data->ped_BackupPath);
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

/* Free owned Name and Path copies and delegate Group disposal. The final
 * IconTool copy is not freed here. Backup closure belongs to Cleanup, not
 * this handler. Return superclass result.
 */
IPTR PrefsEditor__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    if (data->ped_Name != NULL) FreeVec(data->ped_Name);
    if (data->ped_Path != NULL) FreeVec(data->ped_Path);
    
    return DoSuperMethodA(CLASS, self, message);
}

/* Store Changed, Testing, CanTest and CanUse; replace Name/Path/IconTool by
 * freeing then duplicating. Require readable independent string inputs or
 * NULL; copy failure loses old value. CanSave is not handled. Prepend
 * Group_Forward FALSE to a temporary message whose TAG_MORE points at
 * caller tags, then delegate and return superclass result. This does not
 * set NoNotify, and a later caller Group_Forward tag can override the
 * prepended setting.
 */
IPTR PrefsEditor__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList;
    struct TagItem *tag,
                    noforward_attrs[] =
                    {
                        { MUIA_Group_Forward, FALSE        },
                        { TAG_MORE,           (IPTR)tstate }
                    };
    struct opSet noforward_message = *message;
    noforward_message.ops_AttrList = noforward_attrs;
    
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PrefsEditor_Changed:
                data->ped_Changed = tag->ti_Data;
                break;
                
            case MUIA_PrefsEditor_Testing:
                data->ped_Testing = tag->ti_Data;
                break;
                
            case MUIA_PrefsEditor_CanTest:
                data->ped_CanTest = tag->ti_Data;
                break;
                
            case MUIA_PrefsEditor_Name:
                if (data->ped_Name != NULL) FreeVec(data->ped_Name);
                data->ped_Name = StrDup((CONST_STRPTR) tag->ti_Data);
                break;
                
            case MUIA_PrefsEditor_Path:
                if (data->ped_Path != NULL) FreeVec(data->ped_Path);
                data->ped_Path = StrDup((CONST_STRPTR) tag->ti_Data);
                break;

            case MUIA_PrefsEditor_IconTool:
                if (data->ped_IconTool != NULL) FreeVec(data->ped_IconTool);
                data->ped_IconTool = StrDup((CONST_STRPTR) tag->ti_Data);
                break;

            case MUIA_PrefsEditor_CanUse:
                data->ped_CanUse = tag->ti_Data;
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) &noforward_message);
}

/* For all eight local attributes write the stored value and return TRUE.
 * String pointers are borrowed until replacement or disposal. Require valid
 * writable opg_Storage. Delegate unknown attributes to Group and return its
 * result.
 */
IPTR PrefsEditor__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_PrefsEditor_Changed:
            *store = data->ped_Changed;
            break;
            
        case MUIA_PrefsEditor_Testing:
            *store = data->ped_Testing;
            break;
        
        case MUIA_PrefsEditor_CanSave:
            *store = data->ped_CanSave;
            break;
            
        case MUIA_PrefsEditor_CanTest:
            *store = data->ped_CanTest;
            break;
        
        case MUIA_PrefsEditor_Name:
            *store = (IPTR) data->ped_Name;
            break;
            
        case MUIA_PrefsEditor_Path:
            *store = (IPTR) data->ped_Path;
            break;

        case MUIA_PrefsEditor_IconTool:
            *store = (IPTR) data->ped_IconTool;
            break;
            
        case MUIA_PrefsEditor_CanUse:
            *store = data->ped_CanUse;
            break;

        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

/* Private helper: lock the supplied directory for reading, temporarily make
 * it the calling process current directory, and invoke Export with the
 * stored Path. Restore the prior current directory and unlock after the
 * synchronous call. Return the method result converted to BOOL, or FALSE if
 * Lock fails. Require valid directory and Path; overrides must not retain
 * them or disrupt current-directory ownership.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_ExportToDirectory
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportToDirectory *message
)
{
    SETUP_INST_DATA;
    BOOL success = FALSE;
    BPTR lock    = Lock(message->directory, ACCESS_READ);
    
    if (lock != BNULL)
    {
        BPTR oldcd = CurrentDir(lock);
        
        success = DoMethod
        (
            self, MUIM_PrefsEditor_Export, (IPTR) data->ped_Path
        );
        
        CurrentDir(oldcd);
        
        UnLock(lock);
    }
    
    return success;
}

/* Private helper: lock the supplied directory for reading, temporarily make
 * it the calling process current directory, and invoke Import with the
 * stored Path. Restore the prior current directory and unlock after the
 * synchronous call. Return the method result converted to BOOL, or FALSE if
 * Lock fails. Require valid directory and Path; overrides must not retain
 * them or disrupt current-directory ownership.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_ImportFromDirectory
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFromDirectory *message
)
{
    SETUP_INST_DATA;
    BOOL success = FALSE;
    BPTR lock    = Lock(message->directory, ACCESS_READ);
    
    if (lock != BNULL)
    {
        BPTR oldcd = CurrentDir(lock);
        
        success = DoMethod
        (
            self, MUIM_PrefsEditor_Import, (IPTR) data->ped_Path
        );
        
        CurrentDir(oldcd);

        UnLock(lock);

    }
    
    return success;
}

/* Import preferences from a file. FALSE if Open fails; otherwise the
 * ImportFH result converted to BOOL. Open with MODE_OLDFILE, call ImportFH
 * with the handle, then close it. The handle is borrowed for the duration
 * of the call. The base method does not set Changed or Testing. It ignores
 * the Close result and provides no rollback of partial editor changes.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Import
(
    Class *CLASS, Object *self, struct MUIP_PrefsEditor_Import *message
)
{
    BOOL success = FALSE;
    BPTR fh;
    
    if ((fh = Open(message->filename, MODE_OLDFILE)) != BNULL)
    {
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) fh);
        Close(fh);
    }
    
    return success;
}

/* Export preferences to a file. FALSE if the file cannot be opened;
 * otherwise the ExportFH result converted to BOOL. Open with MODE_NEWFILE,
 * creating or truncating the destination before ExportFH runs. If Open
 * reports ERROR_OBJECT_NOT_FOUND, duplicate the filename, attempt to create
 * slash-delimited parent directories, and retry Open. The directory-helper
 * result is ignored; earlier directories can remain after a later failure.
 * Close the handle after ExportFH, ignoring Close errors. No atomic
 * replacement or rollback is provided. Changed and Testing are not updated.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Export
(
    Class *CLASS, Object *self, struct MUIP_PrefsEditor_Export *message
)
{
    BOOL success = FALSE;
    BPTR fh;
    
    fh = Open(message->filename, MODE_NEWFILE);
    
    if (fh == BNULL && IoErr() == ERROR_OBJECT_NOT_FOUND)
    {
        /* Attempt to create missing directories */
        /* MakeDirs() will modify the string in-place */
        STRPTR tmp = StrDup(message->filename);
        if (tmp != NULL)
        {
            MakeDirs(tmp);
            fh = Open(message->filename, MODE_NEWFILE);
            FreeVec(tmp);
        }
    }
    
    if (fh != BNULL)
    {
        success = DoMethod(self, MUIM_PrefsEditor_ExportFH, (IPTR) fh);
        Close(fh);
    }
    
    return success;
}

/* Apply the current editor settings temporarily. TRUE after successful
 * export to ENV:; FALSE otherwise. Export Path relative to ENV:. On success
 * set Changed FALSE, then Testing TRUE. This writes settings, not a dry
 * run. It does not update the setup backup or check CanTest. On failure the
 * flags are not changed by this base method, but a partial file can remain.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Test
(
    Class *CLASS, Object *self, Msg message
)
{
    if (DoMethod(self, MUIM_PrefsEditor_ExportToDirectory, ENV))
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, TRUE);
        
        return TRUE;
    }
    
    return FALSE;
}

/* Restore the settings captured during Setup. TRUE if seeking, importing
 * the backup, and exporting to ENV: all succeed; FALSE otherwise. Seek the
 * backup to offset zero, call ImportFH, then export Path relative to ENV:.
 * On complete success set Changed FALSE, then Testing FALSE. Stop at the
 * first failure. If the final export fails, the editor may already contain
 * the backup settings while flags retain their old values. There is no
 * CanTest or valid-handle check before Seek. The backup is the setup
 * snapshot, not the most recent Test or Save.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (Seek(data->ped_BackupFH, 0, OFFSET_BEGINNING) == -1)
                 return FALSE;//error
    
    if
    (
           DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) data->ped_BackupFH)
        && DoMethod(self, MUIM_PrefsEditor_ExportToDirectory, ENV)
    )
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
    
        return TRUE;
    }

     return FALSE;//error
   
}

/* Apply settings and archive them. TRUE only if Use and the subsequent
 * ENVARC: export both succeed; FALSE otherwise. Call Use first, then export
 * Path relative to ENVARC:. If archiving fails after Use succeeds, ENV: has
 * already been written and Changed/Testing cleared. Neither those effects
 * nor a partial archive are rolled back. No CanSave check, application
 * exit, or error requester is provided.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Save
(
    Class *CLASS, Object *self, Msg message
)
{
    if
    (
           DoMethod(self, MUIM_PrefsEditor_Use)
        && DoMethod(self, MUIM_PrefsEditor_ExportToDirectory, ENVARC)
    )
    {
        return TRUE;
    }
        
    return FALSE;
}

/* Apply settings without archiving them. TRUE after successful export to
 * ENV:; FALSE otherwise. Export Path relative to ENV:. On success set
 * Changed FALSE, then Testing FALSE. Do not update the setup backup or
 * write ENVARC:. CanUse is not checked. Export failure does not cause a
 * base-class flag update but can leave a partial destination file.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    if (DoMethod(self, MUIM_PrefsEditor_ExportToDirectory, ENV))
    {
        SET(self, MUIA_PrefsEditor_Changed, FALSE);
        SET(self, MUIA_PrefsEditor_Testing, FALSE);
        
        return TRUE;
    }
    
    return FALSE;
}

/* Revert active test settings when requested by the Testing flag. The
 * Revert result if Testing is nonzero; TRUE otherwise. If Testing is false,
 * return immediately without restoring edited controls, changing flags, or
 * writing files. If Testing is true, Revert requires the valid setup
 * backup. Cancel does not itself close a window or quit an application.
 */
IPTR PrefsEditor__MUIM_PrefsEditor_Cancel
(
    Class *CLASS, Object *self, Msg message
)
{
    if (XGET(self, MUIA_PrefsEditor_Testing))
    {
        return DoMethod(self, MUIM_PrefsEditor_Revert);
    }
    
    return TRUE;
}
