/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/datatypes.h>

#include <string.h>

#   include <aros/debug.h>

/*** Prototypes *************************************************************/
BOOL __FindDeviceName_WB(STRPTR buffer, LONG length, CONST_STRPTR volume, struct DosLibrary *DOSBase);
struct DiskObject *__GetDefaultIconFromName_WB(CONST_STRPTR name, struct TagItem *tags, struct Library *IconBase);
struct DiskObject *__GetDefaultIconFromType_WB(LONG type, struct TagItem *tags, struct Library *IconBase);

/*** Macros *****************************************************************/
#define FindDeviceName(buffer, length, volume) (__FindDeviceName_WB((buffer), (length), (volume), DOSBase))
#define GetDefaultIconFromName(name, tags) (__GetDefaultIconFromName_WB((name), (tags), IconBase))
#define GetDefaultIconFromType(type, tags) (__GetDefaultIconFromType_WB((type), (tags), IconBase))

AROS_UFH3
(
    struct DiskObject *, FindDefaultIcon,
    AROS_UFHA(struct Hook *,            hook,     A0),
    AROS_UFHA(APTR,                     reserved, A2),
    AROS_UFHA(struct IconIdentifyMsg *, iim,      A1) 
)
{
    struct DiskObject *icon = NULL;
    
    /* Identify object -----------------------------------------------------*/
    if (DataTypesBase == NULL)
    {
        // FIXME: implement a primitive identification here?
        return NULL;
    }
    else
    {
        if (iim->iim_FIB->fib_DirEntryType == ST_ROOT)
        {
            /* It's a disk/volume/root -------------------------------------*/
            TEXT device[MAXFILENAMELENGTH];
            
            if
            (
                FindDeviceName
                (
                    device, MAXFILENAMELENGTH, 
                    iim->iim_FIB->fib_FileName
                )
            )
            {
                if (strlen(device) == 4) 
                {
                    if (strcasecmp(device, "RAM:") == 0)
                    {
                        icon = GetDefaultIconFromName("RAM", iim->iim_Tags);
                    }
                    else if (strncasecmp(device, "DF", 2) == 0)
                    {
                        icon = GetDefaultIconFromName("Floppy", iim->iim_Tags);
                    }
                    else if (strncasecmp(device, "CD", 2) == 0)
                    {
                        icon = GetDefaultIconFromName("CDROM", iim->iim_Tags);
                    }
                    else if
                    (
                           strncasecmp(device, "DH",  2) == 0 
                        || strncasecmp(device, "HD",  2) == 0
                        || strncasecmp(device, "EMU", 3) == 0
                    )
                    {
                        icon = GetDefaultIconFromName("Harddisk", iim->iim_Tags);
                    }
                }
                else if (strcasecmp(device, "HOME:") == 0)
                {
                    icon = GetDefaultIconFromName("Home", iim->iim_Tags);
                    
                    /* Fall back to generic harddisk icon */
                    if (icon ==  NULL)
                    {
                        icon = GetDefaultIconFromName("Harddisk", iim->iim_Tags);
                    }
                }
                
            }
            
            /* Fall back to generic disk icon */
            if (icon == NULL)
            {
                icon = GetDefaultIconFromType(WBDISK, iim->iim_Tags);
            }
            
            if (icon != NULL)
            {
                /* Force the icon type, in case we have a broken icon */
                icon->do_Type = WBDISK;
            }
        }
        else if (iim->iim_FIB->fib_DirEntryType > 0)
        {
            /* It's a directory --------------------------------------------*/
            /* Check if it is a trashcan directory */
            if (iim->iim_ParentLock != NULL)
            {
                /* Is iim_ParentLock a root? */
                BPTR root = ParentDir(iim->iim_ParentLock);
                
                if (root == NULL)
                {
                    /* Yes, it's a root. See if it contains our trashcan. */
                    BPTR cd   = CurrentDir(iim->iim_ParentLock);
                    BPTR lock = Lock("Trashcan", ACCESS_READ);
                    
                    if (lock != NULL)
                    {
                        if (SameLock(iim->iim_FileLock, lock) == LOCK_SAME)
                        {
                            icon = GetDefaultIconFromType(WBGARBAGE, iim->iim_Tags);
                        }
                    
                        UnLock(lock);
                    }
                    
                    CurrentDir(cd);
                }
                else
                {
                    UnLock(root);
                }
            }
            
            /* Fall back to generic drawer icon */
            if (icon == NULL)
            {
                icon = GetDefaultIconFromType(WBDRAWER, iim->iim_Tags);
            }
            
            if (icon != NULL)
            {
                /* Force the icon type, in case we have a broken icon */
                icon->do_Type = WBDRAWER;
            }
        }
        else
        {
            /* It's a file -------------------------------------------------*/
            struct DataType *dt = ObtainDataType
            (
                DTST_FILE, iim->iim_FileLock, TAG_DONE
            );
            
            if (dt != NULL)
            {
                struct DataTypeHeader *dth = dt->dtn_Header;
                
                if
                (
                       dth->dth_GroupID == GID_SYSTEM 
                    && dth->dth_ID      == ID_EXECUTABLE
                )
                {
                    /* It's a exutable file --------------------------------*/
                    icon = GetDefaultIconFromType(WBTOOL, iim->iim_Tags);
                    
                    if (icon != NULL)
                    {
                        /* Force the icon type, in case we have a broken icon */
                        icon->do_Type = WBTOOL;
                    }
                }
                else
                {
                    /* It's a project file of some kind --------------------*/
                    icon = GetDefaultIconFromName(dth->dth_Name, iim->iim_Tags);
                    
                    /* Fall back to generic filetype group icon */
                    if (icon == NULL)
                    {
                        STRPTR name = NULL;
                        
                        switch (dth->dth_GroupID)
                        {
                            case GID_SYSTEM:     name = "System";     break;
                            case GID_TEXT:       name = "Text";       break;
                            case GID_DOCUMENT:   name = "Document";   break;
                            case GID_SOUND:      name = "Sound";      break;
                            case GID_INSTRUMENT: name = "Instrument"; break;
                            case GID_MUSIC:      name = "Music";      break;
                            case GID_PICTURE:    name = "Picture";    break;
                            case GID_ANIMATION:  name = "Animation";  break;
                            case GID_MOVIE:      name = "Movie";      break;
                        }
                        
                        if (name != NULL)
                        {
                            icon = GetDefaultIconFromName(name, iim->iim_Tags);
                        }
                    }
                    
                    /* Fall back to generic project icon */
                    if (icon == NULL)
                    {
                        icon = GetDefaultIconFromType(WBPROJECT, iim->iim_Tags);
                    }
                    
                    if (icon != NULL)
                    {
                        /* Force the icon type, in case we have a broken icon */
                        icon->do_Type = WBPROJECT;
                    }
                }
                
                ReleaseDataType(dt);
            }
            else
            {
                icon = GetDefaultIconFromType(WBPROJECT, iim->iim_Tags);
                
                if (icon != NULL)
                {
                    /* Force the icon type, in case we have a broken icon */
                    icon->do_Type = WBPROJECT;
                }
            }
        }
    }
    
    return icon;
}

/*** Support functions ******************************************************/
BOOL __FindDeviceName_WB
(
    STRPTR buffer, LONG length, CONST_STRPTR volume, 
    struct DosLibrary *DOSBase
)
{
    struct DosList *dl      = LockDosList(LDF_DEVICES | LDF_READ);
    BOOL            success = FALSE;
    
    if (dl != NULL)
    {
        struct DosList *dol = dl;
        
        while ((dol = NextDosEntry(dol, LDF_DEVICES | LDF_READ)) != NULL)
        {
            TEXT device[MAXFILENAMELENGTH];
            
            strlcpy(device, dol->dol_DevName, MAXFILENAMELENGTH);
            
            if (strlcat(device, ":", MAXFILENAMELENGTH) < MAXFILENAMELENGTH)
            {
                BPTR lock;
                
                if
                (       
                       IsFileSystem(device)
                    && (lock = Lock(device, ACCESS_READ)) != NULL
                )
                {
                    if (NameFromLock(lock, buffer, length))
                    {
                        if (strncasecmp(volume, buffer, strlen(buffer) - 1) == 0)
                        {
                            if (strlcpy(buffer, device, length) < length)
                            {
                                success = TRUE;
                            }
                            
                            break;
                        }
                    }
                    
                    UnLock(lock);
                }
            }
        }
        
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    
    return success;
}

struct DiskObject *__GetDefaultIconFromName_WB
(
    CONST_STRPTR name, struct TagItem *tags, struct Library *IconBase
)
{
    return GetIconTags
    (
        NULL, 
        ICONGETA_GetDefaultName, (IPTR) name, 
        TAG_MORE,                (IPTR) tags
    );
}

struct DiskObject *__GetDefaultIconFromType_WB
(
    LONG type, struct TagItem *tags, struct Library *IconBase
)
{
    return GetIconTags
    (
        NULL,
        ICONGETA_GetDefaultType,        type,
        TAG_MORE,                (IPTR) tags
    );
}
