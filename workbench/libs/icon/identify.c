/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
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

//the following define is used to comment out gcc's incorrect section attributes
#if !defined(SECTIONCOMMENT)
#if defined(__arm__)
#define SECTIONCOMMENT "\n@"
#else
#define SECTIONCOMMENT "\n#"
#endif
#endif

/*** Prototypes *************************************************************/
BOOL __FindDeviceName_WB(STRPTR buffer, LONG length, BPTR lock, APTR *theDOSBase);
struct DiskObject *__GetDefaultIconFromName_WB(CONST_STRPTR name, const struct TagItem *tags, struct IconBase *IconBase);
struct DiskObject *__GetDefaultIconFromType_WB(LONG type, const struct TagItem *tags, struct IconBase *IconBase);
LONG __FindDiskType_WB(STRPTR volname, BPTR lock, struct IconBase *IconBase);

CONST_STRPTR    str_disk __attribute__ ((__section__(".rodata,\"a\" " SECTIONCOMMENT))) = "disk";
CONST_STRPTR    str_Disk __attribute__ ((__section__(".rodata,\"a\" " SECTIONCOMMENT))) = "Disk";
CONST_STRPTR    str_CDROM __attribute__ ((__section__(".rodata,\"a\" " SECTIONCOMMENT))) = "CDROM";
CONST_STRPTR    str_Harddisk __attribute__ ((__section__(".rodata,\"a\" " SECTIONCOMMENT))) = "Harddisk";

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

struct DiskObject *GetFSDeviceIcon(char *FS, const char *Dev, const char *DevClass,
                                                        const struct TagItem *tags, struct IconBase *IconBase)
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
            strcat(fsDev, str_disk);
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
        strcat(fsDev, str_disk);
        if (icon = GetDefaultIconFromName(fsDev, tags))
            return icon;
    }

    if (icon = GetDefaultIconFromName(DevClass, tags))
        return icon;
    return NULL;
}

/* TRUE if dev starts with the letters of prefix (given in upper case), ignoring case */
static BOOL DevPrefix(const char *dev, const char *prefix)
{
    while (*prefix)
    {
        if ((*dev++ & ~0x20) != *prefix++)
            return FALSE;
    }
    return TRUE;
}

BOOL IsDiscDevice(char *dev, int devlen)
{
    return DevPrefix(dev, "CD") || DevPrefix(dev, "DVD");
}

/*
 * The default icon of a device is looked up as "<filesystem><class>",
 * "<filesystem><device>disk", "<device>disk" and "<class>" in turn, see
 * GetFSDeviceIcon(). The filesystem part comes from the disk type; which
 * types are recognised depends on the class of the device.
 */
#define FSC_DISC        (1 << 0)
#define FSC_FLOPPY      (1 << 1)
#define FSC_HARDDISK    (1 << 2)
#define FSC_ALL         (FSC_DISC | FSC_FLOPPY | FSC_HARDDISK)
#define FSC_DISK        (FSC_FLOPPY | FSC_HARDDISK)

struct FSClassName
{
    ULONG fsc_ID;
    UBYTE fsc_Name;             /* index into fsclassnames[] */
    UBYTE fsc_Classes;          /* FSC_ flags of the device classes this applies to */
};

static const char fsclassnames[][5] = { "NDOS", "Busy", "Kick", "PFS", "FAT", "SFS", "FFS" };

static const struct FSClassName fsclasstable[] =
{
    { ID_UNREADABLE_DISK,       0, FSC_ALL      },
    { ID_NOT_REALLY_DOS,        0, FSC_ALL      },
    { ID_BUSY_DISK,             1, FSC_ALL      },
    { ID_KICKSTART_DISK,        2, FSC_FLOPPY   },
    { ID_PFS_DISK,              3, FSC_DISK     },
    { ID_PFS2_DISK,             3, FSC_DISK     },
    { ID_PFS3_DISK,             3, FSC_DISK     },
    { ID_PFS2_SCSI_DISK,        3, FSC_DISK     },
    { ID_PFS3_SCSI_DISK,        3, FSC_DISK     },
    { ID_PFS2_muFS_DISK,        3, FSC_DISK     },
    { ID_FLOPPY_PFS_DISK,       3, FSC_FLOPPY   },
    { ID_NTFS_DISK,             4, FSC_HARDDISK },
    { ID_MSDOS_DISK,            4, FSC_DISK     },
    { ID_FAT12_DISK,            4, FSC_DISK     },
    { ID_FAT16_DISK,            4, FSC_DISK     },
    { ID_FAT32_DISK,            4, FSC_DISK     },
    { ID_SFS_BE_DISK,           5, FSC_DISK     },
    { ID_SFS_LE_DISK,           5, FSC_DISK     },
    { ID_muFS_DISK,             6, FSC_DISK     },
    { ID_DOS_muFS_DISK,         6, FSC_DISK     },
    { ID_FFS_muFS_DISK,         6, FSC_DISK     },
    { ID_INTER_DOS_muFS_DISK,   6, FSC_DISK     },
    { ID_INTER_FFS_muFS_DISK,   6, FSC_DISK     },
    { ID_FASTDIR_DOS_muFS_DISK, 6, FSC_DISK     },
    { ID_FASTDIR_FFS_muFS_DISK, 6, FSC_DISK     },
    { ID_AFS0_DISK,             6, FSC_DISK     },
    { ID_AFS1_DISK,             6, FSC_DISK     },
    { ID_AFS_muFS_DISK,         6, FSC_DISK     },
    { ID_FFS_DISK,              6, FSC_DISK     },
    { ID_INTER_DOS_DISK,        6, FSC_DISK     },
    { ID_INTER_FFS_DISK,        6, FSC_DISK     },
    { ID_FASTDIR_DOS_DISK,      6, FSC_DISK     },
    { ID_FASTDIR_FFS_DISK,      6, FSC_DISK     },
};

static struct DiskObject *GetClassDeviceIcon(char *device, ULONG fsid, ULONG devclass, const char *classname,
                                             const struct TagItem *tags, struct IconBase *IconBase)
{
    char *fsstr = NULL;
    ULONG i;

    if (fsid)
    {
        for (i = 0; i < sizeof(fsclasstable) / sizeof(fsclasstable[0]); i++)
        {
            if (fsclasstable[i].fsc_ID == fsid)
            {
                if (fsclasstable[i].fsc_Classes & devclass)
                    fsstr = (char *)fsclassnames[fsclasstable[i].fsc_Name];
                break;
            }
        }
    }
    return GetFSDeviceIcon(fsstr, device, classname, tags, IconBase);
}

BOOL IsFloppyDevice(char *dev)
{
    if ((dev[2] >= '0') && (dev[2] <= '9'))
        if ((dev[0] == 'F') || (dev[1] == 'F'))
            return TRUE;
    return FALSE;
}

BOOL IsHarddiskDevice(char *dev, int devlen)
{
    // try to match a (D)Hx(.x) or H(D)x(.x) type name first.
    if (devlen < 6)
    {
        if (dev[2] >= '0' && dev[2] <= '9')
        {
            if ((dev[0] == 'H') || (dev[1] == 'H'))
                return TRUE;
        }
        else if (DevPrefix(dev, "EMU"))
            return TRUE;
    }
    // try to match an ATAxPx or NVMExPx type name.
    if (DevPrefix(dev, "ATA") &&
         (((dev[4] & ~0x20) == 'P') || ((dev[5] & ~0x20) == 'P')))
        return TRUE;
    if (DevPrefix(dev, "NVME") &&
         (((dev[5] & ~0x20) == 'P') || ((dev[6] & ~0x20) == 'P')))
        return TRUE;
    return FALSE;
}

BOOL IsUSBDevice(char *dev, int devlen)
{
    if (devlen < 6)
    {
        if ((dev[2] >= '0') && (dev[2] <= '9'))
            if ((dev[0] == 'D') && (dev[1] == 'U'))
                return TRUE;
    }
    if (DevPrefix(dev, "USB"))
        return TRUE;
    return FALSE;
}

struct DiskObject *__GetDeviceIcon_WB
(
    char *name, ULONG fsid, const struct TagItem *tags, struct IconBase *IconBase
)
{
    int devnamelen;
    if ((devnamelen = strlen(name)) < 5)
    {
        if (DevPrefix(name, "RAM") &&
             (name[3] == ':'))
            return GetDefaultIconFromName("RAM", tags);
        if (DevPrefix(name, "RAD"))
            return GetDefaultIconFromName("RAD", tags);
        if (DevPrefix(name, "HOME"))
            return GetDefaultIconFromName("Home", tags);
        else if (IsFloppyDevice(name))
            return GetClassDeviceIcon(name, fsid, FSC_FLOPPY, str_Disk, tags, IconBase);
    }
    if (IsUSBDevice(name, devnamelen))
        return GetDefaultIconFromName("USB", tags);
    else if (IsDiscDevice(name, devnamelen))
        return GetClassDeviceIcon(name, fsid, FSC_DISC, str_CDROM, tags, IconBase);
    else if (IsHarddiskDevice(name, devnamelen))
        return GetClassDeviceIcon(name, fsid, FSC_HARDDISK, str_Harddisk, tags, IconBase);

    return GetDefaultIconFromName("UnknownDevice", tags);
}

/* The generic icon of a type if there is no icon yet, with the type forced */
static struct DiskObject *TypedDefaultIcon(struct DiskObject *icon, LONG type,
                                           const struct TagItem *tags, struct IconBase *IconBase)
{
    if (icon == NULL)
    {
        icon = GetDefaultIconFromType(type, tags);
    }

    if (icon != NULL)
    {
        /* Force the icon type */
        icon->do_Type = type;
    }

    return icon;
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
            icon = __GetDeviceIcon_WB(device, type, iim->iim_Tags, IconBase);
        }
        
        /* Fall back to generic disk icon */
        icon = TypedDefaultIcon(icon, WBDISK, iim->iim_Tags, IconBase);
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
        icon = TypedDefaultIcon(icon, WBDRAWER, iim->iim_Tags, IconBase);
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
                    icon = TypedDefaultIcon(NULL, WBTOOL, iim->iim_Tags, IconBase);
                }
                else
                {
                    /* It's a project file of some kind --------------------*/
                    icon = GetDefaultIconFromName(dth->dth_Name, iim->iim_Tags);
                    
                    /* Fall back to generic filetype group icon */
                    if (icon == NULL)
                    {
                        static const struct { ULONG gid; CONST_STRPTR name; } groupnames[] =
                        {
                            { GID_SYSTEM,     "System"     },
                            { GID_TEXT,       "Text"       },
                            { GID_DOCUMENT,   "Document"   },
                            { GID_SOUND,      "Sound"      },
                            { GID_INSTRUMENT, "Instrument" },
                            { GID_MUSIC,      "Music"      },
                            { GID_PICTURE,    "Picture"    },
                            { GID_ANIMATION,  "Animation"  },
                            { GID_MOVIE,      "Movie"      },
                        };
                        CONST_STRPTR name = NULL;
                        ULONG g;

                        for (g = 0; g < sizeof(groupnames) / sizeof(groupnames[0]); g++)
                        {
                            if (groupnames[g].gid == dth->dth_GroupID)
                            {
                                name = groupnames[g].name;
                                break;
                            }
                        }
                        
                        if (name != NULL)
                        {
                            icon = GetDefaultIconFromName(name, iim->iim_Tags);
                        }
                    }
                    
                    /* Fall back to generic project icon */
                    icon = TypedDefaultIcon(icon, WBPROJECT, iim->iim_Tags, IconBase);
                }
                
                ReleaseDataType(dt);
            }
        }
        
        if (icon == NULL)
        {
            /* Fallback to a more primitive identification -----------------*/
            /* Executable files are tools, the others projects of some kind */
            icon = TypedDefaultIcon(NULL,
                ((iim->iim_FIB->fib_Protection & FIBF_EXECUTE) == 0) ? WBTOOL : WBPROJECT,
                iim->iim_Tags, IconBase);
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
