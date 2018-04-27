/*
    Copyright © 2003-2012, The AROS Development Team. All rights reserved.
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

#include "icon_intern.h"

#include <string.h>

#ifndef ID_FAT12_DISK
#define ID_FAT12_DISK      AROS_MAKE_ID('F','A','T',0) /* FAT12 */
#define ID_FAT16_DISK      AROS_MAKE_ID('F','A','T',1) /* FAT16 */
#define ID_FAT32_DISK      AROS_MAKE_ID('F','A','T',2) /* FAT32 */
#endif
#ifndef ID_CDFS_DISK
#define ID_CDFS_DISK       AROS_MAKE_ID('C','D','F','S') /* CDFS */
#endif

/*** Prototypes *************************************************************/
BOOL __FindDeviceName_WB(STRPTR buffer, LONG length, BPTR lock, APTR *theDOSBase);
struct DiskObject *__GetDefaultIconFromName_WB(CONST_STRPTR name, const struct TagItem *tags, struct IconBase *IconBase);
struct DiskObject *__GetDefaultIconFromType_WB(LONG type, const struct TagItem *tags, struct IconBase *IconBase);
LONG __FindDiskType_WB(STRPTR volname, BPTR lock, struct IconBase *IconBase);

/*** Macros *****************************************************************/
#define FindDeviceName(buffer, length, volume) (__FindDeviceName_WB((buffer), (length), (volume), DOSBase))
#define GetDefaultIconFromName(name, tags) (__GetDefaultIconFromName_WB((name), (tags), IconBase))
#define GetDefaultIconFromType(type, tags) (__GetDefaultIconFromType_WB((type), (tags), IconBase))
#define FindDiskType(volname, lock) (__FindDiskType_WB((volname),(lock),IconBase))
/*** Functions **************************************************************/
LONG __FindType_WB(BPTR lock, struct IconBase *IconBase) 
{
    LONG                  type = -1;
    struct FileInfoBlock *fib  = AllocDosObject(DOS_FIB, TAG_DONE);
    
    if (fib != NULL)
    {
        if (Examine(lock, fib))
        {
            D(bug("[%s] Examine says: fib_DirEntryType=%d\n", __func__, fib->fib_DirEntryType));
            D(bug("[%s] Examine says: fib_Protection=%x\n", __func__, fib->fib_Protection));
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
                        DTST_FILE, (APTR)lock, TAG_DONE
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
                            /* It's an executable file */
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
    
    return type;
}

struct DiskObject *__FindDefaultIcon_WB
(
    struct IconIdentifyMsg *iim, struct IconBase *IconBase
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
                iim->iim_FileLock
            )
        )
        {
            BPTR lock = Lock(device, SHARED_LOCK);
            LONG type = FindDiskType(iim->iim_FIB->fib_FileName, lock);
            UnLock(lock);
            if (strlen(device) <= 5)
            {
                if (strcasecmp(device, "RAM:") == 0)
                {
                    icon = GetDefaultIconFromName("RAM", iim->iim_Tags);
                }
                else if (strncasecmp(device, "RAD", 3) == 0)
                {
                    icon = GetDefaultIconFromName("RAD", iim->iim_Tags);
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
                else if (strcasecmp(device, "HOME") == 0)
                {
                    icon = GetDefaultIconFromName("Home", iim->iim_Tags);
                }
                else if (type)
                {
                    D(bug("[icon] Identify Type: 0x%8x\n",type));
                    switch(type)
                    {
                        case ID_MSDOS_DISK:
                        case ID_FAT12_DISK:
                        case ID_FAT16_DISK:
                        case ID_FAT32_DISK:
                            icon = GetDefaultIconFromName("FAT", iim->iim_Tags);
                            break;
                        case ID_SFS_BE_DISK:
                        case ID_SFS_LE_DISK:
                            icon = GetDefaultIconFromName("SFS", iim->iim_Tags);
                            break;
                        case ID_FFS_DISK:
                        case ID_INTER_DOS_DISK:
                        case ID_INTER_FFS_DISK:
                        case ID_FASTDIR_DOS_DISK:
                        case ID_FASTDIR_FFS_DISK:
                            icon = GetDefaultIconFromName("ADF", iim->iim_Tags);
                            break;
                        case ID_CDFS_DISK:
                            icon = GetDefaultIconFromName("CDROM", iim->iim_Tags);
                            break;
                        default:
                            icon = GetDefaultIconFromName("Disk", iim->iim_Tags);
                            break;
                    }
                }
            }
            else if (strncasecmp(device, "USB", 3) ==0)
            {
                icon = GetDefaultIconFromName("USB", iim->iim_Tags);
            }
            else
            {
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
        if (iim->iim_ParentLock != BNULL)
        {
            /* Is iim_ParentLock a root? */
            BPTR root = ParentDir(iim->iim_ParentLock);

            if (root == BNULL)
            {
                /* Yes, it's a root. See if it contains our trashcan. */
                BPTR cd   = CurrentDir(iim->iim_ParentLock);

                UBYTE buffer[MAXFILENAMELENGTH], buffer1[MAXFILENAMELENGTH];

                /* SFS .recycled Trashcan */ 
                BPTR lock = Lock(".recycled", ACCESS_READ);
                NameFromLock(iim->iim_FileLock, buffer, MAXFILENAMELENGTH);

                if (lock != BNULL)
                {
                    NameFromLock(lock, buffer1, MAXFILENAMELENGTH);
                    if (strcasecmp(buffer, buffer1) == 0)
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
                DTST_FILE, (APTR)iim->iim_FileLock, TAG_DONE
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
                    /* It's an executable file -----------------------------*/
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
LONG __FindDiskType_WB(STRPTR volname, BPTR lock, struct IconBase *IconBase)
{
    LONG disktype = ID_NO_DISK_PRESENT;
    struct DosList *dl, *dn;

    dl = LockDosList(LDF_VOLUMES|LDF_READ);
    if (dl)
    {
        dn = FindDosEntry(dl, volname, LDF_VOLUMES);
        if (dn)
        {
            disktype = dn->dol_misc.dol_volume.dol_DiskType;
        }
    UnLockDosList(LDF_VOLUMES|LDF_READ);
    }

    if (disktype == 0) //FFS workaround (dol_DiskType == 0)
    {
        struct InfoData *id = AllocMem(sizeof(struct InfoData), MEMF_PUBLIC|MEMF_CLEAR);
        if (id != NULL)
        {
            if (Info(lock, id))
            {
                disktype = id->id_DiskType;
            }
            FreeMem(id,sizeof(struct InfoData));
        }
    }
    return disktype;
}

BOOL __FindDeviceName_WB
(
    STRPTR device, LONG length, BPTR lock,
    APTR *theDOSBase
)
{
#undef DOSBase
#define DOSBase theDOSBase
    struct DosList *dl      = LockDosList(LDF_DEVICES | LDF_READ);
    BOOL            success = FALSE;
    
    if (dl != NULL)
    {
        struct DosList *dol = dl;
        struct MsgPort *port = ((struct FileLock *) BADDR(lock))->fl_Task;
        
        while ((dol = NextDosEntry(dol, LDF_DEVICES | LDF_READ)) != NULL)
        {
            STRPTR devname = AROS_BSTR_ADDR(dol->dol_Name);
            ULONG len = AROS_BSTR_strlen(dol->dol_Name);

            if (dol->dol_Task == port) {
                CopyMem(devname, device, len);
                device[len++] = ':';
                device[len] = 0;

                success = TRUE;
                break;
            }
        }
        
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }
    
    return success;
}

struct DiskObject *__GetDefaultIconFromName_WB
(
    CONST_STRPTR name, const struct TagItem *tags, struct IconBase *IconBase
)
{
    return GetIconTags
    (
        NULL,
        ICONGETA_GetDefaultName, (IPTR)name,
        TAG_END
    );
}

struct DiskObject *__GetDefaultIconFromType_WB
(
    LONG type, const struct TagItem *tags, struct IconBase *IconBase
)
{
    return GetIconTags
    (
        NULL,
        ICONGETA_GetDefaultType, type,
        TAG_END
    );
}
