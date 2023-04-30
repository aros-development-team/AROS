/*
    Copyright (C) 2003-2023, The AROS Development Team. All rights reserved.
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
#include <stdio.h>

#ifndef ID_BUSY_DISK
# define ID_BUSY_DISK       AROS_MAKE_ID('B','U','S','Y')
#endif
#ifndef ID_FAT12_DISK
# define ID_FAT12_DISK      AROS_MAKE_ID('F','A','T',0) /* FAT12 */
# define ID_FAT16_DISK      AROS_MAKE_ID('F','A','T',1) /* FAT16 */
# define ID_FAT32_DISK      AROS_MAKE_ID('F','A','T',2) /* FAT32 */
#endif
#ifndef ID_CDFS_DISK
# define ID_CDFS_DISK       AROS_MAKE_ID('C','D','F','S') /* CDFS */
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


struct DiskObject *GetFSDeviceIcon(char *FS, char*Dev, char*DevClass, const struct TagItem *tags, struct IconBase *IconBase)
{
    struct DiskObject *icon = NULL;
    char fsDev[MAXFILENAMELENGTH];
    if (FS)
    {
        if (Dev)
        {
            int devlen = strnlen(Dev,32);
            fsDev[0] = '\0';
            strcat(fsDev, FS);
            strncat(fsDev, Dev, devlen - 1);
            strcat(fsDev, "disk");
            if (icon = GetDefaultIconFromName(fsDev, tags))
                return icon;
        }
        sprintf(fsDev, "%s%s", FS, DevClass);
        if (icon = GetDefaultIconFromName(fsDev, tags))
            return icon;
    }
    if (Dev)
    {
        int devlen = strnlen(Dev,32);
        fsDev[0] = '\0';
        strncat(fsDev, Dev, devlen - 1);
        strcat(fsDev, "disk");
        if (icon = GetDefaultIconFromName(fsDev, tags))
            return icon;
    }

    if (icon = GetDefaultIconFromName(DevClass, tags))
        return icon;
    return NULL;
}

BOOL IsDiscDevice(char *dev)
{
    if (strncasecmp(dev, "CD", 2) == 0)
        return TRUE;
    return FALSE;
}

struct DiskObject *GetDiscIcon(char *device, ULONG fsid, const struct TagItem *tags, struct IconBase *IconBase)
{
    char *fsstr = NULL;

    if (fsid)
        switch(fsid)
        {
        case ID_UNREADABLE_DISK:
        case ID_NOT_REALLY_DOS:
            fsstr = "NDOS";
            break;
        case ID_BUSY_DISK:
            fsstr = "Busy";
            break;
        }
    return GetFSDeviceIcon(fsstr, device, "CDROM", tags, IconBase);
}

BOOL IsFloppyDevice(char *dev)
{
    if ((dev[2] >= '0') && (dev[2] <= '9'))
        if ((dev[0] == 'F') || (dev[1] == 'F'))
            return TRUE;
    return FALSE;
}

struct DiskObject *GetFloppydiskIcon(char *device, ULONG fsid, const struct TagItem *tags, struct IconBase *IconBase)
{
    char *fsstr = NULL;

    if (fsid)
        switch(fsid)
        {
        case ID_UNREADABLE_DISK:
        case ID_NOT_REALLY_DOS:
            fsstr = "NDOS";
            break;
        case ID_BUSY_DISK:
            fsstr = "Busy";
            break;
        case ID_KICKSTART_DISK:
            fsstr = "Kick";
            break;
        case ID_PFS_DISK:
        case ID_PFS2_DISK:
        case ID_PFS3_DISK:
        case ID_PFS2_SCSI_DISK:
        case ID_PFS3_SCSI_DISK:
        case ID_PFS2_muFS_DISK:
        case ID_FLOPPY_PFS_DISK:
            fsstr = "PFS";
            break;
        case ID_MSDOS_DISK:
        case ID_FAT12_DISK:
        case ID_FAT16_DISK:
        case ID_FAT32_DISK:
            fsstr = "FAT";
            break;
        case ID_SFS_BE_DISK:
        case ID_SFS_LE_DISK:
            fsstr = "SFS";
            break;
        case ID_muFS_DISK:
        case ID_DOS_muFS_DISK:
        case ID_FFS_muFS_DISK:
        case ID_INTER_DOS_muFS_DISK:
        case ID_INTER_FFS_muFS_DISK:
        case ID_FASTDIR_DOS_muFS_DISK:
        case ID_FASTDIR_FFS_muFS_DISK:
        case ID_AFS0_DISK:
        case ID_AFS1_DISK:
        case ID_AFS_muFS_DISK:
        case ID_FFS_DISK:
        case ID_INTER_DOS_DISK:
        case ID_INTER_FFS_DISK:
        case ID_FASTDIR_DOS_DISK:
        case ID_FASTDIR_FFS_DISK:
            fsstr = "FFS";
            break;
        }
    return GetFSDeviceIcon(fsstr, device, "Disk", tags, IconBase);
}

BOOL IsHarddiskDevice(char *dev)
{
    if (dev[2] >= '0' && dev[2] <= '9')
    {
        if ((dev[0] == 'H') || (dev[1] == 'H'))
            return TRUE;
    }
    else if (strncasecmp(dev, "EMU", 3) == 0)
        return TRUE;
    return FALSE;
}

struct DiskObject *GetHarddiskIcon(char *device, ULONG fsid, const struct TagItem *tags, struct IconBase *IconBase)
{
    char *fsstr = NULL;

    if (fsid)
        switch(fsid)
        {
        case ID_UNREADABLE_DISK:
        case ID_NOT_REALLY_DOS:
            fsstr = "NDOS";
            break;
        case ID_BUSY_DISK:
            fsstr = "Busy";
            break;
        case ID_PFS_DISK:
        case ID_PFS2_DISK:
        case ID_PFS3_DISK:
        case ID_PFS2_SCSI_DISK:
        case ID_PFS3_SCSI_DISK:
        case ID_PFS2_muFS_DISK:
            fsstr = "PFS";
            break;
        case ID_NTFS_DISK:
        case ID_MSDOS_DISK:
        case ID_FAT12_DISK:
        case ID_FAT16_DISK:
        case ID_FAT32_DISK:
            fsstr = "FAT";
            break;
        case ID_SFS_BE_DISK:
        case ID_SFS_LE_DISK:
            fsstr = "SFS";
            break;
        case ID_muFS_DISK:
        case ID_DOS_muFS_DISK:
        case ID_FFS_muFS_DISK:
        case ID_INTER_DOS_muFS_DISK:
        case ID_INTER_FFS_muFS_DISK:
        case ID_FASTDIR_DOS_muFS_DISK:
        case ID_FASTDIR_FFS_muFS_DISK:
        case ID_AFS0_DISK:
        case ID_AFS1_DISK:
        case ID_AFS_muFS_DISK:
        case ID_FFS_DISK:
        case ID_INTER_DOS_DISK:
        case ID_INTER_FFS_DISK:
        case ID_FASTDIR_DOS_DISK:
        case ID_FASTDIR_FFS_DISK:
            fsstr = "FFS";
            break;
        }
    return GetFSDeviceIcon(fsstr, device, "Harddisk", tags, IconBase);
}

struct DiskObject *__GetDeviceIcon_WB
(
    char *name, ULONG fsid, const struct TagItem *tags, struct IconBase *IconBase
)
{
    if (strlen(name) <= 5)
    {
        if (strncasecmp(name, "RAM:", 4) == 0)
            return GetDefaultIconFromName("RAM", tags);
        else if (strncasecmp(name, "RAD", 3) == 0)
            return GetDefaultIconFromName("RAD", tags);
        else if (strcasecmp(name, "HOME") == 0)
            return GetDefaultIconFromName("Home", tags);
        else if (strncasecmp(name, "USB", 3) ==0)
            return GetDefaultIconFromName("USB", tags);
        else if (IsFloppyDevice(name))
            return GetFloppydiskIcon(name, fsid, tags, IconBase);
        else if (IsDiscDevice(name))
            return GetDiscIcon(name, fsid, tags, IconBase);
        else if (IsHarddiskDevice(name))
            return GetHarddiskIcon(name, fsid, tags, IconBase);
    }
    return GetDefaultIconFromName("UnknownDevice", tags);
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
            bug("[Icon] %s: Lock for '%s' @ 0x%p\n", __func__, device, lock);
            LONG type = FindDiskType(iim->iim_FIB->fib_FileName, lock);
            UnLock(lock);
            icon = __GetDeviceIcon_WB(device, type, iim->iim_Tags, IconBase);
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

struct DiskObject *__FindDeviceIcon_WB
(
    struct IconIdentifyMsg *iim, struct IconBase *IconBase
)
{
    struct InfoData devIData;
    struct DosList *dl;
    devIData.id_DiskType = ID_UNREADABLE_DISK;
    dl = LockDosList(LDF_DEVICES|LDF_READ);
    if (dl)
    {
        struct DosList *dn = FindDosEntry(dl, iim->iim_FIB->fib_FileName, LDF_DEVICES);
        if ((dn) && (dn->dol_Task))
        {
            DoPkt(dn->dol_Task, ACTION_DISK_INFO,
                    (SIPTR) MKBADDR(&devIData),
                    (SIPTR) BNULL, (SIPTR) BNULL,
                    (SIPTR) BNULL, (SIPTR) BNULL);
        }
        UnLockDosList(LDF_DEVICES|LDF_READ);
    }
    return __GetDeviceIcon_WB(iim->iim_FIB->fib_FileName, devIData.id_DiskType, iim->iim_Tags, IconBase);
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
