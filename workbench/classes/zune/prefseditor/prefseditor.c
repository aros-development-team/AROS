/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
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
Object *PrefsEditor__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    self = (Object *) DoSuperMethodA(CLASS, self, message);

    if (self != NULL)
    {
        SETUP_INST_DATA;
	struct TagItem noforward_attrs[] =
	{
	    { MUIA_Group_Forward, FALSE                       },
	    { TAG_MORE,           (IPTR)message->ops_AttrList }
	};
	
        /*-- Handle initial attribute values -------------------------------*/
        SetAttrsA(self, noforward_attrs);
        
        /*-- Set defaults --------------------------------------------------*/
        data->ped_CanSave = TRUE;
        data->ped_CanTest = TRUE;
    }
    
    return self;
}

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
        DoMethod(self, MUIM_PrefsEditor_ImportFromDirectory, ENVARC);
    }
    
    /*-- Store backup of initial preferences -------------------------------*/
    data->ped_BackupFH = CreateTemporary(data->ped_BackupPath, BACKUP_PREFIX);    
    if (data->ped_BackupFH != NULL)
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
            data->ped_BackupFH = NULL;
        }
    }
    
    if (data->ped_BackupFH == NULL)
    {
        data->ped_CanTest = FALSE;
    }
    
    /*-- Completely disable save if ENVARC: is write-protected -------------*/
    lock = Lock("ENVARC:", SHARED_LOCK);
    if (lock != NULL)
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

IPTR PrefsEditor__MUIM_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    SETUP_INST_DATA;
    
    if (data->ped_BackupFH != NULL)
    {
        Close(data->ped_BackupFH);
        DeleteFile(data->ped_BackupPath);
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

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

IPTR PrefsEditor__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList,
                   *tag;
    
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
                
            case MUIA_PrefsEditor_Name:
                if (data->ped_Name != NULL) FreeVec(data->ped_Name);
                data->ped_Name = StrDup((CONST_STRPTR) tag->ti_Data);
                break;
                
            case MUIA_PrefsEditor_Path:
                if (data->ped_Path != NULL) FreeVec(data->ped_Path);
                data->ped_Path = StrDup((CONST_STRPTR) tag->ti_Data);
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

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
            
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR PrefsEditor__MUIM_PrefsEditor_ExportToDirectory
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ExportToDirectory *message
)
{
    SETUP_INST_DATA;
    BOOL success = FALSE;
    BPTR lock    = Lock(message->directory, ACCESS_READ);
    
    if (lock != NULL)
    {
        BPTR oldcd = CurrentDir(lock);
        
        success = DoMethod
        (
            self, MUIM_PrefsEditor_Export, (IPTR) data->ped_Path
        );
        
        CurrentDir(oldcd);
    }
    
    return success;
}

IPTR PrefsEditor__MUIM_PrefsEditor_ImportFromDirectory
(
    Class *CLASS, Object *self,
    struct MUIP_PrefsEditor_ImportFromDirectory *message
)
{
    SETUP_INST_DATA;
    BOOL success = FALSE;
    BPTR lock    = Lock(message->directory, ACCESS_READ);
    
    if (lock != NULL)
    {
        BPTR oldcd = CurrentDir(lock);
        
        success = DoMethod
        (
            self, MUIM_PrefsEditor_Import, (IPTR) data->ped_Path
        );
        
        CurrentDir(oldcd);
    }
    
    return success;
}

IPTR PrefsEditor__MUIM_PrefsEditor_Import
(
    Class *CLASS, Object *self, struct MUIP_PrefsEditor_Import *message
)
{
    BOOL success = FALSE;
    BPTR fh;
    
    if ((fh = Open(message->filename, MODE_OLDFILE)) != NULL)
    {
        success = DoMethod(self, MUIM_PrefsEditor_ImportFH, (IPTR) fh);
        Close(fh);
    }
    
    return success;
}

IPTR PrefsEditor__MUIM_PrefsEditor_Export
(
    Class *CLASS, Object *self, struct MUIP_PrefsEditor_Export *message
)
{
    BOOL success = FALSE;
    BPTR fh;     
    
    fh = Open(message->filename, MODE_NEWFILE);
    
    if (fh == NULL && IoErr() == ERROR_OBJECT_NOT_FOUND)
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
    
    if (fh != NULL)
    {
        success = DoMethod(self, MUIM_PrefsEditor_ExportFH, (IPTR) fh);
        Close(fh);
    }
    
    return success;
}

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

IPTR PrefsEditor__MUIM_PrefsEditor_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (Seek(data->ped_BackupFH, 0, OFFSET_BEGINNING) == -1) goto error;
    
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

error:    
    return FALSE;
}

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
