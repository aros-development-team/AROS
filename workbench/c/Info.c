/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Info Cli Command
    Lang: English
*/

/******************************************************************************


    NAME

        Info

    SYNOPSIS

        DISKS/S, VOLS=VOLUMES/S, ALL/S, BLOCKS/S, DEVICES/M

    LOCATION

        C:

    FUNCTION

    Show information on file system devices and volumes. When given no
    arguments, information on all devices and volumes found in the system
    is displayed. If information is wanted only for some specific devices,
    these names may be given as arguments.

    INPUTS

    DISKS     --  show information on file system devices
    VOLS      --  show information on volumes
    ALL       --  show information on bad devices or volumes
    BLOCKS    --  show additional block size and usage information
    DEVICES   --  device names to show information about

    RESULT

    NOTES

    EXAMPLE

    Info

    Unit                 Size    Used    Free Full Errs   State    Type Name
    Harddisk:         964.1M  776.7M  187.4M  81%    0 read/write  OFS AROS
    RAM:                8.0M    7.1M    7.1M  12%    0 read/write  OFS Ram Disk

    BUGS

    SEE ALSO

    INTERNALS

    The original source showed that AROS version of ReadArgs() handles
    the /M switch with zero arguments differently from AmigaOS. While AROS
    returns an array where the first pointer is NULL, AmigaOS just returns
    NULL.

    HISTORY

    16.11.2000  SDuvan  --  converted to AROS
    23.12.2000  SDuvan  --  changed semantics and updated
                            (now fully functional)
    17.02.2005  Joe Fenton -- fixed 64bit calculation

    Based on the original by:
    © 1997-1998 by Stephan Rupprecht
    All rights resevered
******************************************************************************/

#define  DEBUG 0
#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/memory.h>
#include <libraries/locale.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/alib.h>

#include <string.h>

#define ID_MAC_DISK2       (0x4d414300L) /* MAC\0 - xfs mac disk */
#define ID_MNX1_DISK       (0x4d4e5801L) /* MNX\1 - xfs minix disk */
#define ID_MNX2_DISK       (0x4d4e5802L) /* MNX\2 - xfs minix disk */
#define ID_QL5A_DISK       (0x514c3541L) /* QL5A - xfs ql 720k / ed disk */
#define ID_QL5B_DISK       (0x514c3542L) /* QL5B - xfs ql 1440k disk */
#define ID_ZXS0_DISK       (0x5a585300L) /* Spectrum Disciple - xfs */
#define ID_ZXS1_DISK       (0x5a585301L) /* Spectrum UniDos - xfs */
#define ID_ZXS2_DISK       (0x5a585302L) /* Spectrum SamDos - xfs */
#define ID_ZXS4_DISK       (0x5a585304L) /* Spectrum Opus 180k - xfs */
#define ID_ARME_DISK       (0x41524d44L) /* Archimedes - xfs */
#define ID_ARMD_DISK       (0x41524d43L) /* Archimedes - xfs */
#define ID_CPM_DISK        (0x43505c4dL) /* CP/M - xfs */
#define ID_ZXS3_DISK       (0x5a585303L) /* ZXS\3 - Plus3Dos xfs */
#define ID_1541_DISK       (0x31353431L) /* 1541 - xfs */
#define ID_1581_DISK       (0x31353831L) /* 1581 - xfs */
#define ID_MAC_DISK        (0x4d534800L) /* MSH\0 - CrossDos MACDisk ?! */
#define ID_ACD0_DISK       (0x41434400L) /* ACD\0 - AmiCDFS disk */
#define ID_CDFS_DISK       (0x43444653L) /* CDFS  - AmiCDFS disk */
#define ID_CACHECDFS_DISK  (0x43443031L)
#define ID_ASIMCDFS_DISK   (0x662dabacL)
#define ID_PFS_DISK        (0x50465301L) /* PFS */
#define ID_PFS2_DISK       (0x50465302L)
#define ID_PFS2_SCSI_DISK  (0x50445300L)
#define ID_PFS2_muFS_DISK  (0x6d755046L)
#define ID_FLOPPY_PFS_DISK (0x50465300L)
#define ID_P2A0_DISK       (0x50324130L)
#define ID_AFS0_DISK       (0x41465300L) /* AFS\0 */
#define ID_muFS_DISK       (0x6d754653L) /* muFS - Multiuserfsys */
#define ID_FAT12_DISK      (0x46415400L) /* FAT12 */
#define ID_FAT16_DISK      (0x46415401L) /* FAT16 */
#define ID_FAT32_DISK      (0x46415402L) /* FAT32 */
#define ID_EXT2_DISK       (0x45585432L) /* Extended 2 - Linux */


/* Prototypes */

ULONG ComputeKBytes(ULONG a, ULONG b);
void FmtProcedure(struct Hook *hook, char a, struct Locale *locale);
ULONG ExtUDivMod32(ULONG a, ULONG b, ULONG *mod);
void doInfo();


const TEXT VersionStr[] = "$VER: Info 41.2 (26.05.2012)";

struct Catalog  *cat;
struct Locale   *loc = NULL;
ULONG            MaxLen;

APTR             Pool;


/* catalog string id:s */
enum 
{
    UNIT,
    DEVTITLE,
    DISKSTITLE,
    DEVFMTSTR,
    DATEFMTSTR,
    READONLY,
    READWRITE,
    VALIDATING,
    MOUNTEDSTR,
    SMALLNUMFMT,
    BIGNUMFMT,
    VOLNAMEFMTSTR,
    BLOCKSSTR
};


struct InfoDosNode 
{
    struct InfoDosNode *Next;
    ULONG               IsVolume;
    ULONG               DosType;
    struct MsgPort     *Task;
    struct DateStamp    VolumeDate;
    TEXT                Name[108];
};

struct InfoDosNode *head = NULL;


struct DiskTypeList
{   ULONG   id;
    STRPTR  str;
};

struct DiskTypeList dtl[] = 
{
    { ID_DOS_DISK,         "OFS" },
    { ID_FFS_DISK,         "FFS" },
    { ID_INTER_DOS_DISK,   "OFS-INT" },
    { ID_INTER_FFS_DISK,   "FFS-INT" },
    { ID_FASTDIR_DOS_DISK, "OFS-DC" },
    { ID_FASTDIR_FFS_DISK, "FFS-DC" },
    { ID_MSDOS_DISK,       "MS-DOS" },
    { ID_ACD0_DISK,        "CDFS" },
    { ID_CACHECDFS_DISK,   "CDFS" },
    { ID_ASIMCDFS_DISK,    "CDFS" },
    { ID_NOT_REALLY_DOS,   "NO DOS" },
    { ID_MAC_DISK2,        "MAC" },
    { ID_MNX1_DISK,        "Minix" },
    { ID_QL5A_DISK,        "QL720k" },
    { ID_QL5B_DISK,        "QL1.4M" },
    { ID_CPM_DISK,         "CP/M" },
    { ID_ZXS3_DISK,        "+3Dos" },
    { ID_ZXS0_DISK,        "Disciple " },
    { ID_ZXS1_DISK,        "UniDos" },
    { ID_ZXS2_DISK,        "SamDos" },
    { ID_ZXS4_DISK,        "Opus" },
    { ID_P2A0_DISK,        "NETWORK" },
    { ID_FAT12_DISK,       "FAT12" },
    { ID_FAT16_DISK,       "FAT16" },
    { ID_FAT32_DISK,       "FAT32" },
    { ID_SFS_BE_DISK,      "SFS" },
    { ID_SFS_LE_DISK,      "sfs" },
    { ID_PFS_DISK,         "PFS" },
    { ID_EXT2_DISK,        "EXT2" },
    { 0L, 0L }
};


/****************************************************************************/

int UtilityBase_version = 0;
int LocaleBase_version = 0;

int __nocommandline;

int main(void)
{
    static struct TagItem loctags[] = { { OC_Version, 1 },
                                                { TAG_END   , 0 } };
    cat = OpenCatalogA(NULL, "info_com.catalog", loctags);
    loc = OpenLocale(NULL);

    D(bug("Calling doInfo()\n"));

    doInfo();

    CloseLocale(loc);
    CloseCatalog(cat);

    return RETURN_OK;  /* TODO: Fix this */
}


CONST_STRPTR GetStrFromCat(ULONG id, CONST_STRPTR def)
{
    if(cat != NULL)
    {
        def = GetCatalogStr(cat, id, def);
    }
    
    return def;
}


void VLPrintf(ULONG id, CONST_STRPTR def, const IPTR* argarray)
{
    def = GetStrFromCat(id, def);
    VPrintf(def, (IPTR *)argarray);
}


BOOL myMatchPatternNoCase(STRPTR *array, STRPTR str)
{
    if(*array != NULL)
    {
        while(*array != NULL)
        {
            UBYTE   matchstr[128];
            UBYTE   name[32];
            UBYTE  *p = *array++;
            UBYTE   len = strlen(p);
            
            if(p[len - 1] != ':')
            {
                CopyMem(p, name, len);
                name[len] = ':';
                name[len + 1] = 0;
                p = name;
            }
            
            if(ParsePatternNoCase(p, matchstr, sizeof(matchstr)) != -1)
            {
                if(MatchPatternNoCase(matchstr, str))
                {
                    return TRUE;
                }
            }
        }
        
        return FALSE;
    }
    
    return TRUE;
}


BOOL ScanDosList(STRPTR *filter)
{
    struct InfoDosNode *idn = 0L;
    struct DosList     *ndl, *dl;
    STRPTR             *strray = NULL, dummy = NULL;
    BOOL                err = FALSE;

    D(bug("Entered ScanDosList()\n"));
    
    if (filter == NULL) filter = &dummy;
    
    if(*filter != NULL)
    {
        strray = AllocPooled(Pool, sizeof(STRPTR)*MAX_MULTIARGS);

        if(strray != NULL)
        {
            STRPTR  *p = filter;
            LONG     i = 0;
            
            while(*p)
                strray[i++] = *p++;

            while(i < MAX_MULTIARGS)
                strray[i++] = NULL;
        }
        else
            return FALSE;
    }

    /* lock list of devices & vols */
    dl = ndl = LockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_DEVICES | LDF_READ);

    if(strray != NULL)
    {
        STRPTR *p = strray;

        while(*p)
            p++;

        while((ndl = NextDosEntry(ndl, LDF_ASSIGNS | LDF_VOLUMES | LDF_READ)) != NULL)
        {
            TEXT    name[108];
            STRPTR  taskName = NULL;  /* Initialized to avoid a warning */

            __sprintf(name, "%b:", ndl->dol_Name);

            if ((ndl->dol_Type > DLT_VOLUME) || !(myMatchPatternNoCase(strray, name)))
            {
                continue;
            }

            switch (ndl->dol_Type)
            {
            case DLT_VOLUME:
                taskName =  ((struct Task *)ndl->dol_Task->mp_SigTask)->tc_Node.ln_Name;

                D(bug("Found volume %s\n", taskName));
                break;

            case DLT_DIRECTORY:
                {
                    struct AssignList *al = ndl->dol_misc.dol_assign.dol_List;

                    taskName = ((struct Task *)((struct FileLock *)BADDR(ndl->dol_Lock))->fl_Task->mp_SigTask)->tc_Node.ln_Name;

                    D(bug("Found directory %s\n", taskName));
                    
                    while(al != NULL)
                    {
                        *p++ = ""; // TODO!!!  ((struct Task *)((struct FileLock *)BADDR(al->al_Lock))->fl_Task->mp_SigTask)->tc_Node.ln_Name;
                        al = al->al_Next;
                    }
                }
                break;
            }
            
            *p++ = taskName;
        }
    }
    else
        strray = filter;
    
    ndl = dl;
    
    while((ndl = NextDosEntry(ndl, LDF_VOLUMES | LDF_DEVICES | LDF_READ)) != NULL)
    {
        UBYTE  len = 0;
        UBYTE  type = ndl->dol_Type;
        UBYTE  name[108];

        /* do not start non-started handlers or open CON: or RAW: windows.. */
        if(type == DLT_DEVICE && !ndl->dol_Task)
            continue;

        __sprintf(name, "%b:", ndl->dol_Name);
        D(bug("Found name %s\n", name));

        if((type == DLT_DEVICE) && (myMatchPatternNoCase(strray, name) == FALSE))
        {
            int i;

            D(bug("Failure! -- name = %s, strray = %p\n", name, (void *)strray));

            for (i = 0; strray[i] != NULL; i++)
            {
                D(bug("Strray %i = %s\n", i, strray[i]));
            }
            
            continue;
        }

        idn = (struct InfoDosNode *)AllocPooled(Pool, sizeof(struct InfoDosNode));

        if(idn == NULL)
        {
            err = TRUE;
            break;
        }
        
        idn->Task     = ndl->dol_Task;
        idn->IsVolume = type == DLT_VOLUME;
        
        while((idn->Name[len] = name[len])) 
            len++;
        
        if(type == DLT_VOLUME)
        {
            idn->VolumeDate = ((struct DeviceList *)ndl)->dl_VolumeDate;
            idn->Name[len - 1] = '\0';       /* remove ':' */
        }
        else
        {
            BPTR ptr = ndl->dol_misc.dol_handler.dol_Startup;
            struct FileSysStartupMsg *fssm = NULL;

            if (IsFileSystem(idn->Name))
            {
                // Only filesystems have a valid FileSysStartupMsg
                fssm = (struct FileSysStartupMsg *)BADDR(ptr);
            }
            
            idn->DosType = ID_DOS_DISK;

            //  DLT_DEVICE
            if (len > MaxLen)
                MaxLen = len;
            
            if (fssm) 
            {
                struct DosEnvec *de;
                de = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
                    
                if (de && (de->de_TableSize & 0xffffff00) == 0)
                    if (de->de_DosType)
                        idn->DosType = de->de_DosType;
            }
        }
        
        /* kinda insert sort */
        {
            struct InfoDosNode *work = head;
            struct InfoDosNode *prev = NULL;
            
            while((work != NULL) && (Stricmp(idn->Name, work->Name) > 0))
            {
                prev = work;
                work = work->Next;
            }
            
            if(prev != NULL)
                prev->Next = idn;
            else
                head = idn;
            
            idn->Next = work;
        }
    }
    
    /* unlock list of devices and volumes */
    UnLockDosList(LDF_ASSIGNS | LDF_VOLUMES | LDF_DEVICES | LDF_READ);
    
    // strray freed at DeletePool
    
    return !err;
}


void PrintNum(ULONG num)
{
    /* MBytes ? */
    if(num > 1023) 
    {
        ULONG  x, xx;
        char   fmt = 'M';
        
        /* GBytes ? */
        if(num > 0xfffff)
        { 
            num >>= 10; 
            fmt = 'G'; 
        }
        
        num = ExtUDivMod32(UMult32(num, 100) >> 10, 100, &x);
        
        /* round */
        x = ExtUDivMod32(x, 10, &xx);

        if(xx > 4)
        {
            if(++x > 9)
            {
                x = 0;
                num++;
            }
        }

        IPTR args[] = {num, x, fmt};
        VLPrintf(BIGNUMFMT, "%5ld.%ld%lc", args);
    } 
    else 
    {
        IPTR args[] = { num };
        VLPrintf(SMALLNUMFMT, "%7ldK", args);
    }
}


STRPTR GetFSysStr(ULONG DiskType)
{
    struct DiskTypeList *dtlptr = dtl;

    STRPTR ptr = NULL;
    
    do {
        if(dtlptr->id == DiskType)
        { 
            ptr = dtlptr->str;
            break;
        }
    } while(*((ULONG *)dtlptr++));
    
    if(ptr == NULL)
    {
        static TEXT buffer[5];
        
        ptr = (STRPTR)buffer;
        *((ULONG *)ptr) = AROS_LONG2BE(DiskType);
        
        if(ptr[3] < ' ')
            ptr[3] += '0';
        
        ptr[4] = '\0';
    }
    
    return ptr;
}


enum
{
    ARG_DISKS,
    ARG_VOLS,
    ARG_ALL,
    ARG_BLOCKS,
    ARG_DEVS,
    NOOFARGS
};


void doInfo()
{
    struct RDArgs       *rdargs;
    struct Process      *proc;
    struct Window       *win;
    struct InfoDosNode  *idn;
    
    struct InfoData *id = AllocVec(sizeof(struct InfoData), MEMF_ANY);
    
    IPTR   args[] = { (IPTR)FALSE,
                      (IPTR)FALSE,
                      (IPTR)FALSE,
                      (IPTR)FALSE,
                      (IPTR)NULL };
        
    CONST_STRPTR unit = GetStrFromCat(UNIT, "Unit");

    if(id == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, NULL);
        return;
    }

    Pool = CreatePool(MEMF_ANY, 1024, 1024);

    if(Pool == NULL)
    {
        PrintFault(ERROR_NO_FREE_STORE, NULL);
        return;  /* ??? */
    }

    D(bug("Calling ReadArgs()\n"));
    
    /* read arguments */
    rdargs = ReadArgs("DISKS/S,VOLS=VOLUMES/S,ALL/S,BLOCKS/S,DEVICES/M",
                      args, NULL);
    
    if(rdargs != NULL)
    {
        BOOL     disks    = (BOOL)args[ARG_DISKS];
        BOOL     vols     = (BOOL)args[ARG_VOLS];
        BOOL     showall  = (BOOL)args[ARG_ALL];
        BOOL     blocks   = (BOOL)args[ARG_BLOCKS];
        STRPTR  *devs     = (STRPTR *)args[ARG_DEVS];

        if (devs && (*devs == NULL)) devs = NULL;
        
        /* If nothing is specified, show everything we got */
        if(devs == NULL && !disks && !vols)
        {
            vols = TRUE;
            disks = TRUE;
        }

        /* check pattern strings */
        
        if(devs != NULL)
        {
            STRPTR  *p = devs;
            
            while(*p != NULL)
            {
                TEXT  matchstr[128];

                if(ParsePatternNoCase(*p, matchstr, sizeof(matchstr)) == -1)
                {
                    PrintFault(IoErr(), *p);
                    goto end;
                }
                
                p++;
            }
        }

        /* avoid requesters */
        proc = (struct Process *)FindTask(NULL);
        win  = (struct Window *)proc->pr_WindowPtr;
        proc->pr_WindowPtr = (struct Window *)~0;
        
        MaxLen = strlen(unit);

        D(bug("Calling ScanDosList()\n"));
        
        /* scan doslist */
        if(ScanDosList(devs))
        {
            CONST_STRPTR  dstate[3] = { GetStrFromCat(READONLY,   "read only"),
                                  GetStrFromCat(VALIDATING, "validating"),
                                  GetStrFromCat(READWRITE,  "read/write") };
            STRPTR  datetimeFmt = NULL;
            BOOL    first = TRUE;
            TEXT    nfmtstr[16];
            TEXT    buf[64];
            
            D(bug("Printing stuff\n"));

            /* get datetimefmt string */
            if(loc && (GetVar("info_datetime", buf, sizeof(buf), 0L) > 0L))
            {
                datetimeFmt = buf;
            }
            
            /* calc format string for 'Unit' */
            __sprintf(nfmtstr, "%%-%lds", MaxLen);
            
            /* show device infomation */
            if(devs != NULL || disks || !vols)
            {
                for(idn = head; idn; idn = idn->Next)
                {
                    BPTR    lock;
                    STRPTR  name = idn->Name;

                    D(bug("Got name = %s\n", name));
                    
                    if(!idn->IsVolume && IsFileSystem(name))
                    {
                        BOOL gotinfo = FALSE;
                        /* if first device to print, print title */
                        if(first || blocks)
                        {    
                            if(!first)
                                Printf("\n");

                            D(bug("Printing device\n"));
                            
                            VLPrintf(~0, nfmtstr, (IPTR*) &unit);
                            VLPrintf(DEVTITLE, "    Size    Used    Free Full Errs   State    Type    Name\n", NULL);
                            
                            first = FALSE;
                        }
                        
                        VLPrintf(~0, nfmtstr, (IPTR*) &name);

                        D(bug("Locking \"%s\"\n", name));
                        lock = Lock(name, SHARED_LOCK);

                        D(bug("Lock = %p\n", (APTR)lock));

                        if(lock != BNULL)
                        {
                            D(bug("Got lock on %s\n", name));

                            if(Info(lock, id) == DOSTRUE)
                            {
                                D(bug("Calling NameFromLock()\n"));

                                if(NameFromLock(lock, name, 108L))
                                {
                                    LONG len = strlen(name) - 1;
                                    
                                    if(name[len] == ':')
                                    {
                                        name[len] = '\0';
                                    }
                                }
                                
                                gotinfo = TRUE;
                            }
                            UnLock(lock);

                        } else if (idn->Task) {
                            name = NULL;
                            D(bug("Calling ACTION_DISK_INFO\n"));
                            if (DoPkt(idn->Task, ACTION_DISK_INFO, (SIPTR)MKBADDR(id), (SIPTR)BNULL, (SIPTR)BNULL, (SIPTR)BNULL, (SIPTR)BNULL)) {
                                gotinfo = TRUE;
                            }
                        }
                                
                        if (gotinfo) {
                            ULONG  x, y;
                                
                            D(bug("Got info on %s\n", name));

                            if (id->id_DiskType == ID_NO_DISK_PRESENT) {
                                VLPrintf(~0, " No disk present\n", NULL);
                            } else if (id->id_DiskType == ID_NOT_REALLY_DOS) {
                                VLPrintf(~0, " Not a DOS disk\n", NULL);
                            } else if (id->id_DiskType == ID_UNREADABLE_DISK) {
                                VLPrintf(~0, " Unreadable disk\n", NULL);
                            } else {
                                x = ComputeKBytes(id->id_NumBlocks, id->id_BytesPerBlock);
                                y = ComputeKBytes(id->id_NumBlocksUsed, id->id_BytesPerBlock);
                                
                                PrintNum(x);
                                PrintNum(y); 
                                PrintNum(x - y);
                                
                                if(x > 0xfffff)
                                { 
                                    x >>= 10; 
                                    y >>= 10;
                                }
                                
                                if(x)
                                {
                                    x = ExtUDivMod32(UDivMod32(UMult32(y, 1000), x), 10, &y);
                                    
                                    if(y > 4)
                                        x++;
                                }
                                else
                                    x = 0;

                                // y = ((struct DeviceList *)BADDR(id->id_VolumeNode))->dl_DiskType;
                                
                                // if(!y)
                                    y = id->id_DiskType;
                                
                                if((idn->DosType & ID_DOS_DISK) != ID_DOS_DISK)
                                    y = idn->DosType;

                                {
                                    IPTR args[] = {
                                    x,
                                    id->id_NumSoftErrors,
                                    ((id->id_DiskState >= ID_WRITE_PROTECTED) && (id->id_DiskState <= ID_VALIDATED)) ?
                                    (IPTR) dstate[id->id_DiskState - ID_WRITE_PROTECTED] : (IPTR) "",
                                    (IPTR) GetFSysStr(y),
                                    (IPTR) name};
                                     VLPrintf(DEVFMTSTR, "%4ld%% %4ld %-11s%-8s%s\n", args);
                                }

                                if(blocks)
                                {
                                    IPTR args[] = {
                                        id->id_NumBlocks,
                                        id->id_NumBlocksUsed,
                                        id->id_NumBlocks-id->id_NumBlocksUsed,
                                        id->id_BytesPerBlock};
                                    VLPrintf(BLOCKSSTR,
                                            "\nTotal blocks: %-10ld  Blocks used: %ld\n"
                                            " Blocks free: %-10ld    Blocksize: %ld\n",
                                            args);
                                }
                            }
                        }
                        else
                        {
                            D(bug("Info failure\n"));
                            VLPrintf(~0, "\n", NULL);
                        }
                            
                        {
                            LONG err = IoErr();

                            /* just ignore PIPEFS */
                            if (err == ERROR_ACTION_NOT_KNOWN)
                                if (strcmp(name, "PIPEFS:") == 0)
                                    err = 0;
                            
                            if (err && showall)
                            {
                                VLPrintf(~0, nfmtstr, (IPTR*) &name);
                                PrintFault(err, NULL);
                            }
                        }
                    }
                }
            }
            
            /* show volumes */
            if(vols || (!devs && !disks)) 
            {
                if(!first)
                    PutStr("\n");
                
                VLPrintf(DISKSTITLE, "Volumes\n", NULL);
                
                for(MaxLen = 15, idn = head; idn; idn = idn->Next)
                {
                    if(idn->IsVolume)
                    {     
                        LONG len = strlen(idn->Name);
                        
                        if(len > MaxLen)
                            MaxLen = len;
                    }
                }
                
                __sprintf(nfmtstr, "%%-%lds%%-10s", MaxLen+1);
                
                for(idn = head; idn; idn = idn->Next)
                {
                    if(idn->IsVolume)
                    {
                        IPTR args[] = {
                            (IPTR) idn->Name,
                            (IPTR) GetStrFromCat(MOUNTEDSTR, "[Mounted]")};
                            // idn->Task ? GetStrFromCat(MOUNTEDSTR, "[Mounted]") : ""); TODO
                        VLPrintf(VOLNAMEFMTSTR, nfmtstr, args);
                        
                        if(datetimeFmt)
                        {
                            UBYTE datestr[128];
                            static struct Hook hook;

                            memset(&hook, 0, sizeof(struct Hook));

                            hook.h_SubEntry = (HOOKFUNC)FmtProcedure;
                            hook.h_Data = datestr;
                            
                            FormatDate(loc, datetimeFmt, &idn->VolumeDate, &hook);
                            
                            PutStr(datestr);
                        }
                        else
                        {
                            TEXT  StrDay[LEN_DATSTRING];
                            TEXT  StrDate[LEN_DATSTRING];
                            TEXT  StrTime[LEN_DATSTRING];
                            
                            struct DateTime dt;
                            
                            dt.dat_Flags   = DTF_SUBST;
                            dt.dat_Format  = FORMAT_DOS;
                            dt.dat_StrDay  = StrDay;
                            dt.dat_StrDate = StrDate;
                            dt.dat_StrTime = StrTime;
                            dt.dat_Stamp   = idn->VolumeDate;
                            
                            if(DateToStr(&dt))
                            {
                                if(Strnicmp(StrDate, StrDay, strlen(StrDay)) == 0)
                                {
                                    dt.dat_Flags = 0L;
                                    DateToStr(&dt);
                                }
                                
                                IPTR args[] = {(IPTR) StrDay, (IPTR) StrDate, (IPTR) StrTime};
                                VLPrintf(DATEFMTSTR, "created %.3s, %-10s %s", args);
                            }
                        }
                        
                        PutStr("\n");
                    }
                }
            }
        }
        else
        {
            PrintFault( ERROR_NO_FREE_STORE, NULL);
        }

        /* reset window pointer of our process */
        proc->pr_WindowPtr = win;
        
        /* free args */
        FreeArgs(rdargs);
    }
    
    
 end: /* free allocated memory */
    FreeVec(id);
    DeletePool(Pool);
}


ULONG ComputeKBytes(ULONG a, ULONG b)
{
    // UQUAD result = UMult64(a, b);

    UQUAD result = (UQUAD)a * b;

    return (ULONG)(result >> 10);
}


void FmtProcedure(struct Hook *hook, char a, struct Locale *locale)
{
    *((STRPTR)hook->h_Data) = a;
    hook->h_Data = (STRPTR) hook->h_Data + 1;
}


ULONG ExtUDivMod32(ULONG a, ULONG b, ULONG *mod)
{
    *mod = a % b;

    return a/b;
}
