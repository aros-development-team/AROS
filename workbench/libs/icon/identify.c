/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

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


/*** Prototypes *************************************************************/
BOOL __FindDeviceName_WB(STRPTR buffer, LONG length, CONST_STRPTR volume, struct DosLibrary *DOSBase);
struct DiskObject *__GetDefaultIconFromName_WB(CONST_STRPTR name, struct TagItem *tags, struct Library *IconBase);
struct DiskObject *__GetDefaultIconFromType_WB(LONG type, struct TagItem *tags, struct Library *IconBase);

/*** Macros *****************************************************************/
#define FindDeviceName(buffer, length, volume) (__FindDeviceName_WB((buffer), (length), (volume), DOSBase))
#define GetDefaultIconFromName(name, tags) (__GetDefaultIconFromName_WB((name), (tags), IconBase))
#define GetDefaultIconFromType(type, tags) (__GetDefaultIconFromType_WB((type), (tags), IconBase))

/*** Functions **************************************************************/
LONG __FindType_WB(BPTR lock, struct Library *IconBase) 
{
    LONG                  type = -1;
    struct FileInfoBlock *fib  = AllocDosObject(DOS_FIB, TAG_DONE);
    
    if (fib != NULL)
    {
        if (Examine(lock, fib))
        {
            /* Identify object ---------------------------------------------*/
            if (fib->fib_DirEntryType == ST_ROOT)
            {
                /* It's a disk/volume/root ---------------------------------*/
                type = WBDISK;
            }
            else if (fib->fib_DirEntryType > 0)
            {
                /* It's a directory ----------------------------------------*/
                type = WBDRAWER;
            }
            else
            {
                /* It's a file ---------------------------------------------*/
                if (DataTypesBase != NULL)
                {
                    /* Use datatypes to identify the file ------------------*/
                    struct DataType *dt = ObtainDataType
                    (
                        DTST_FILE, lock, TAG_DONE
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
                            /* It's a executable file */
                            type = WBTOOL;
                        }
                        else
                        {
                            /* It's a project file of some kind */
                            type = WBPROJECT;
                        }
                        
                        ReleaseDataType(dt);
                    }
                }
                
                if (type == -1)
                {
                    /* Fallback to a more primitive identification ---------*/
                    if ((fib->fib_Protection & FIBF_EXECUTE) == 0)
                    {
                        type = WBTOOL;
                    }
                    else
                    {
                        type = WBPROJECT;
                    }
                }
                
            }
        }
        
        FreeDosObject(DOS_FIB, fib);
    }
    
    bug("FindType: return %d (WBTOOL=%d, WBPROJ=%d)\n", type, WBTOOL, WBPROJECT);
    
    return type;
}

struct DiskObject *__FindDefaultIcon_WB
(
    struct IconIdentifyMsg *iim, struct Library *IconBase
)
{
    struct DiskObject *icon = NULL;
    
    /* Identify object -----------------------------------------------------*/
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
            /* Force the icon type */
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
            /* Force the icon type */
            icon->do_Type = WBDRAWER;
        }
    }
    else
    {
        /* It's a file -----------------------------------------------------*/
        if (DataTypesBase != NULL)
        {
            /* Use datatypes to identify the file --------------------------*/
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
                    /* It's a executable file ------------------------------*/
                    icon = GetDefaultIconFromType(WBTOOL, iim->iim_Tags);
                    
                    if (icon != NULL)
                    {
                        /* Force the icon type */
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
                        /* Force the icon type */
                        icon->do_Type = WBPROJECT;
                    }
                }
                
                ReleaseDataType(dt);
            }
        }
        
        if (icon == NULL)
        {
            /* Fallback to a more primitive identification -----------------*/
            if ((iim->iim_FIB->fib_Protection & FIBF_EXECUTE) == 0)
            {
                /* It's an executable files --------------------------------*/
                icon = GetDefaultIconFromType(WBTOOL, iim->iim_Tags);
                
                if (icon != NULL)
                {
                    /* Force the icon type */
                    icon->do_Type = WBTOOL;
                }
            }
            else
            {
                /* It's a project file of some kind ------------------------*/
                icon = GetDefaultIconFromType(WBPROJECT, iim->iim_Tags);
                
                if (icon != NULL)
                {
                    /* Force the icon type */
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
                        buffer[strlen(buffer) - 1] = '\0'; /* Remove trailing ':' */
                        if (strcasecmp(volume, buffer) == 0)
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
