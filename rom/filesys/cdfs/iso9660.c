/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <string.h>
#include <endian.h>

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "cdfs.h"
#include "iso9660.h"
#include "bcache.h"

struct ISOLock {
    struct CDFSLock il_Public;
    ULONG           il_Extent;
    UQUAD           il_Offset;    /* For read/write offset */
    ULONG           il_ParentExtent;       /* Extent of the parent directory */
    struct isoPathTable *il_PathTable;
};

struct ISOVolume {
    struct isoVolume iv_Volume;
    struct ISOLock   iv_RootLock;
    TEXT             iv_Name[1 + 32 + 1];
    struct {
        ULONG   Size;   /* In bytes */
        ULONG   Extent;
        ULONG   Entries;
        struct isoPathTable **Entry;
        APTR    Data;
    } iv_PathTable;
};

static ULONG ascdec(STRPTR val, int len)
{
    ULONG out = 0;

    while ((*val >= '0' && *val <= '9') && len > 0) {
        out *= 10;
        out += *val - '9';
        len--;
        val++;
    }

    return out;
}

#define AMIGA_EPOC  722484  /* Day number of Amiga Epoc, using Zeller */

static void ascdate2stamp(ascDate *asc, struct DateStamp *date)
{
    ULONG year = ascdec(asc->Year, 4);
    ULONG month = ascdec(asc->Month, 2);
    ULONG day = ascdec(asc->Day, 2);

    if (month < 3) {
        month += 12;
        year--;
    }
    date->ds_Days = (365*year + year/4 - year/100 + year/400 + day + (153*month+8)/5) - AMIGA_EPOC;
    date->ds_Minute = ascdec(asc->Hour, 2) * 60 +
                      ascdec(asc->Minute, 2);
    date->ds_Tick = ascdec(asc->Second, 2) * 50 + ascdec(asc->Hundredth, 2) / 2;

    /* FIXME: Adjust for timezone */
}

static void bindate2stamp(binDate *bin, struct DateStamp *date)
{
    ULONG year = bin->Years + 1900;
    ULONG month = bin->Month;
    ULONG day = bin->Day;

    if (month < 3) {
        month += 12;
        year--;
    }
    date->ds_Days = (365*year + year/4 - year/100 + year/400 + day + (153*month+8)/5) - AMIGA_EPOC;
    date->ds_Minute = bin->Hour * 60 +
                      bin->Minute;
    date->ds_Tick = bin->Second * 50;

    /* FIXME: Adjust for timezone */
}

static inline ULONG isoRead32LM(int32LM *lm)
{
#if _BYTE_ORDER == _BIG_ENDIAN
    return lm->MSB ? lm->MSB : (ULONG)AROS_LE2LONG(lm->LSB);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
    return lm->LSB ? lm->LSB : (ULONG)AROS_BE2LONG(lm->MSB);
#else /* Wacky byte order? Impossible in this day and age. */
#error PDP11s are no longer supported.
#endif
}

static BOOL isoParseDir(struct CDFSVolume *vol, struct ISOLock *il, struct isoDirectory *dir, ULONG iparent)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    struct FileInfoBlock *fib = &il->il_Public.cl_FileInfoBlock;
    struct FileLock *fl = &il->il_Public.cl_FileLock;
    struct ISOVolume *iv = vol->cv_Private;
    int i, len;
    const strD *cp;
    BOOL has_Amiga_Protections = FALSE;
    BOOL has_RockRidge = FALSE;

    if (dir) {
        il->il_Extent = isoRead32LM(&dir->ExtentLocation);
        il->il_PathTable = NULL;
    } else {
        il->il_Extent = il->il_PathTable->ExtentLocation;
        iparent = iv->iv_PathTable.Entry[il->il_PathTable->ParentDirectory-1]->ExtentLocation;
    }

    D(bug("%s: \"%s\" Extent @0x%08x, Parent 0x%08x, Flags 0x%02x\n", __func__,
          dir ? dir->FileIdentifier : il->il_PathTable->DirectoryIdentifier,
          il->il_Extent, iparent, dir ? dir->Flags : 0));

    if (dir && dir->Flags & (ISO_Directory_ASSOC | ISO_Directory_HIDDEN))
        return FALSE;

    /* No need to re-parse the root lock */
    if (iv && il->il_Extent == iv->iv_RootLock.il_Extent) {
        CopyMem(&iv->iv_RootLock, il, sizeof(*il));
        return TRUE;
    }

    /* The starting location of the file is its unique 'key' */
    fl->fl_Key = il->il_Extent;

    if (dir) {
        fib->fib_Size = isoRead32LM(&dir->DataLength);
        bindate2stamp(&dir->RecordingDate, &fib->fib_Date);
        fib->fib_DirEntryType = (dir->Flags & ISO_Directory_ISDIR) ? ST_USERDIR : ST_FILE;
        len = dir->FileIdentifierLength;
        cp = &dir->FileIdentifier[0];
    } else {
        fib->fib_Size = 0;
        memset(&fib->fib_Date, 0, sizeof(fib->fib_Date));
        fib->fib_DirEntryType = ST_USERDIR;
        len = il->il_PathTable->DirectoryIdentifierLength;
        cp = &il->il_PathTable->DirectoryIdentifier[0];
    }

    if (len > MAXFILENAMELENGTH-2)
        len = MAXFILENAMELENGTH-2;
    for (i = 0; i < len; i++) {
        /* Goddam VMS guys screwing up a perfectly good specification... */
        if (cp[i] == ';') {
            len = i;
            break;
        }
        fib->fib_FileName[i + 1] = cp[i];
    }
    fib->fib_FileName[0] = len;
    fib->fib_FileName[len+1] = 0;
    fib->fib_Protection = FIBF_DELETE |
                          FIBF_EXECUTE |
                          FIBF_WRITE |
                          FIBF_GRP_DELETE |
                          FIBF_GRP_EXECUTE |
                          FIBF_GRP_WRITE |
                          FIBF_OTR_DELETE |
                          FIBF_OTR_EXECUTE |
                          FIBF_OTR_WRITE |
                          FIBF_ARCHIVE;
    fib->fib_EntryType = fib->fib_DirEntryType;
    fib->fib_NumBlocks = (fib->fib_Size + 2047) / 2048;
    fib->fib_Comment[0] = 0;
    fib->fib_OwnerUID = 0;
    fib->fib_OwnerGID = 0;

    il->il_ParentExtent = iparent;

    if (!dir)
        goto end;

    /* Parse RockRidge extensions */
    i = ((UBYTE *)&dir->FileIdentifier[dir->FileIdentifierLength] -
         (UBYTE *)dir);
    if (i & 1)
        i++;
    while (i < (dir->DirectoryLength - 3)) {
        struct rrSystemUse *rr = (APTR)(((UBYTE *)dir) + i);
        D(bug("%s: RR @%d [%c%c] (%d)\n", __func__, i, (AROS_BE2WORD(rr->Signature) >> 8) & 0xff, AROS_BE2WORD(rr->Signature) & 0xff, rr->Length));
        if (rr->Length == 0) {
            D(bug("%s: Corrupted RR section detected (%d left)\n", __func__, dir->DirectoryLength - i));
            break;
        }
        switch (AROS_BE2WORD(rr->Signature)) {
        case RR_SystemUse_AS:
            if (rr->Version == RR_SystemUse_AS_VERSION) {
                UBYTE *data = &rr->AS.Data[0];

                /* Amiga System Use area */
                if (rr->AS.Flags & RR_AS_PROTECTION) {
                    struct SystemUseASProtection *ap = (APTR)data;
                    fib->fib_Protection = (ap->User       << 24) |
                                          (ap->Zero       << 16) |
                                          (ap->MultiUser  << 8) |
                                          (ap->Protection << 0);
                    data = (UBYTE *)&ap[1];
                    has_Amiga_Protections = TRUE;
                }
                if (rr->AS.Flags & RR_AS_COMMENT) {
                    struct SystemUseASComment *ac = (APTR)data;
                    int len = ac->Length;
                    if (len > (MAXCOMMENTLENGTH - fib->fib_Comment[0])-1) {
                        len = (MAXCOMMENTLENGTH - fib->fib_Comment[0])-1;
                        CopyMem(&ac->Comment[0], &fib->fib_Comment[fib->fib_Comment[0]+1], len);
                        fib->fib_Comment[0] += len;
                    }
                }
            }
            break;
        case RR_SystemUse_PX:
            /* This field is mandatory for Rock Ridge */
            has_RockRidge = TRUE;

            /* Don't overwrite the Amiga RR protections */
            if (has_Amiga_Protections)
                break;
            if (rr->Version == RR_SystemUse_PX_VERSION) {
                ULONG prot = 0;
                ULONG mode = isoRead32LM(&rr->PX.st_mode);
                prot |= (mode & S_IXOTH) ? FIBF_OTR_EXECUTE : 0;
                prot |= (mode & S_IWOTH) ? (FIBF_OTR_WRITE | FIBF_OTR_DELETE): 0;
                prot |= (mode & S_IROTH) ? FIBF_OTR_READ : 0;
                prot |= (mode & S_IXGRP) ? FIBF_GRP_EXECUTE : 0;
                prot |= (mode & S_IWGRP) ? (FIBF_GRP_WRITE | FIBF_GRP_DELETE) : 0;
                prot |= (mode & S_IRGRP) ? FIBF_GRP_READ : 0;
                prot |= (mode & S_IXUSR) ? 0 : FIBF_EXECUTE;
                prot |= (mode & S_IWUSR) ? 0 : (FIBF_WRITE | FIBF_DELETE);
                prot |= (mode & S_IRUSR) ? 0 : FIBF_READ;

                fib->fib_Protection = prot;
                fib->fib_OwnerUID = isoRead32LM(&rr->PX.st_uid);
                fib->fib_OwnerGID = isoRead32LM(&rr->PX.st_gid);
            }
            break;
        case RR_SystemUse_NM:
            if (rr->Version == RR_SystemUse_NM_VERSION) {
                int len = rr->Length - ((UBYTE *)&rr->NM.Content[0] - (UBYTE *)rr);
                if (len > (MAXCOMMENTLENGTH - fib->fib_FileName[0])-1) {
                    len = (MAXCOMMENTLENGTH - fib->fib_FileName[0])-1;
                    CopyMem(&rr->NM.Content[0], &fib->fib_FileName[fib->fib_FileName[0]+1], len);
                    fib->fib_FileName[0] += len;
                }
            }
            break;
        case RR_SystemUse_CL:
            if (rr->Version == RR_SystemUse_CL_VERSION) {
                il->il_Extent = isoRead32LM(&rr->CL.ChildDirectory);
            }
            break;
        case RR_SystemUse_PL:
            if (rr->Version == RR_SystemUse_PL_VERSION) {
                il->il_ParentExtent = isoRead32LM(&rr->PL.ParentDirectory);
            }
            break;
        case RR_SystemUse_RE:
            if (rr->Version == RR_SystemUse_RE_VERSION) {
                /* We must ignore this entry */
                return FALSE;
            }
            break;
        case RR_SystemUse_TF:
            /* TF fields have no use under AROS */
            break;
        case RR_SystemUse_SF:
            D(bug("%s: Sparse files are not (yet) supported\n"));
            fib->fib_Size = 0;
            break;
        default:
            break;
        }
        i += rr->Length;
    }

    /* If RockRidge is detected, the PathTable is
     * useless to us, due to re-arranged directories.
     */
    if (has_RockRidge && iv && iv->iv_PathTable.Size) {
        FreeVec(iv->iv_PathTable.Entry);
        FreeVec(iv->iv_PathTable.Data);
        iv->iv_PathTable.Size = 0;
        iv->iv_PathTable.Entries = 0;
    }

end:

    D(fib->fib_FileName[fib->fib_FileName[0]+1]=0);

    D(bug("%s: %c Lock %p, 0x%08x: \"%s\", dir size %d\n", __func__,
                (fib->fib_DirEntryType == 0) ? 'F' : 'D',
                il,
                il->il_ParentExtent,
                &il->il_Public.cl_FileInfoBlock.fib_FileName[1],
                dir ? dir->DirectoryLength : il->il_PathTable->DirectoryIdentifierLength));
    D(bug("%s:  @0x%08x, %d bytes\n", __func__, il->il_Extent, il->il_Public.cl_FileInfoBlock.fib_Size));

    return TRUE;
}

static LONG isoReadDir(struct CDFSVolume *vol, struct ISOLock *ilock, ULONG extent, ULONG offset, ULONG iparent, ULONG *size)
{
    struct BCache *bcache = vol->cv_Device->cd_BCache;
    UBYTE *buff;
    LONG err;
    D(struct CDFS *cdfs = vol->cv_CDFSBase;(void)cdfs;);

    D(bug("%s: Extent @0x%08x, offset 0x%03x, parent 0x%08x\n", __func__, extent, offset, iparent));

    err = BCache_Read(bcache, extent + (offset / 2048), &buff);
    if (err == RETURN_OK) {
        struct isoDirectory *dir = (APTR)&buff[offset];
        if (!isoParseDir(vol, ilock, dir, iparent))
            err = ERROR_OBJECT_WRONG_TYPE;
        else {
            if (dir->DirectoryLength == 0) {
                err = ERROR_NO_MORE_ENTRIES;
            } else {
                if (size)
                    *size = dir->DirectoryLength;
                D(bug("%s: pe %d of %d dl %d\n", __func__, ilock->il_ParentExtent, offset, dir->DirectoryLength));
                if (offset == 0 && dir->DirectoryLength) {
                    offset = dir->DirectoryLength;
                    D(bug("%s: Reading parent dir info at %d\n", __func__, offset));
                    dir = (struct isoDirectory *)&buff[offset];
                    ilock->il_ParentExtent = isoRead32LM(&dir->ExtentLocation);
                    if (size)
                        *size += dir->DirectoryLength;
                }
            }
        }
    }

    return err;
}

static LONG isoFindCmp(struct CDFSVolume *vol, struct ISOLock *ifile, ULONG extent, BOOL (*cmp)(struct ISOLock *il, APTR data), APTR data)
{
    struct ISOVolume *iv = vol->cv_Private;
    ULONG size;
    ULONG offset;
    LONG err;
    struct ISOLock tmp;
    int i;

    /* Scan the path table first, if present */
    for (i = 0; i < iv->iv_PathTable.Entries; i++) {
        struct isoPathTable *pt = iv->iv_PathTable.Entry[i];
        if (pt->ExtentLocation == extent) {
            int j, parent = i;
            /* Is the thing we're looking for a subdirectory?  */
            for (j = 0; j < iv->iv_PathTable.Entries; j++) {
                pt = iv->iv_PathTable.Entry[j];
                if (j == parent)
                    continue;
                ifile->il_PathTable = pt;
                isoParseDir(vol, ifile, NULL, 0);
                if (cmp(ifile, data))
                    return RETURN_OK;
            }
            break;
        }
    }

    /* Read the self/parent at the begining of the directory */
    err = isoReadDir(vol, &tmp, extent, 0, 0, &size);
    if (err != RETURN_OK)
        return err;

    offset = size;

    while (offset < tmp.il_Public.cl_FileInfoBlock.fib_Size) {
        err = isoReadDir(vol, ifile, extent, offset, extent, &size);

        if (err != ERROR_OBJECT_WRONG_TYPE) {
            if (err != RETURN_OK)
                break;

            if (cmp(ifile, data))
                return RETURN_OK;
        }

        offset += size;

        /* Move to next block */
        if (err == ERROR_OBJECT_WRONG_TYPE || size == 0 ||
            (2048 - (offset % 2048)) < sizeof(struct isoDirectory) + 2) {
            offset = (offset + 2048 - 1) & ~(2048 - 1);
        }
    }

    return RETURN_FAIL;
}

struct namecmp {
    struct CDFS  *cdfs;
    CONST_STRPTR file;
    int          len;
};

BOOL cmp(struct ISOLock *fl, APTR data)
{
    struct namecmp *d = data;
    struct CDFS *cdfs = d->cdfs;

    D(bug("%s: file \"%s\"(%d) vs \"%s\"(%d)\n",  __func__,
                d->file, d->len, &fl->il_Public.cl_FileInfoBlock.fib_FileName[1],
                fl->il_Public.cl_FileInfoBlock.fib_FileName[0]));

    return (d->len == fl->il_Public.cl_FileInfoBlock.fib_FileName[0] &&
            Strnicmp(d->file, &fl->il_Public.cl_FileInfoBlock.fib_FileName[1], d->len) == 0);
}

static LONG isoFindName(struct CDFSVolume *vol, struct ISOLock *ifile, CONST_STRPTR file, struct ISOLock *iparent)
{
    struct namecmp data = {
        .cdfs = vol->cv_CDFSBase,
        .file = file,
        .len  = strlen(file)
    };

    return isoFindCmp(vol, ifile, iparent->il_Extent, cmp, &data);
}

static LONG isoFindExtent(struct CDFSVolume *vol, struct ISOLock *ifile, ULONG extent, ULONG parent)
{
    BOOL cmp(struct ISOLock *fl, APTR data) {
        ULONG extent = (ULONG)(IPTR)data;
        return extent == fl->il_Extent;
    }

    return isoFindCmp(vol, ifile, parent, cmp, (APTR)(IPTR)extent);
}

static VOID isoReadPathTables(struct CDFSVolume *vol)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    struct BCache *bcache = vol->cv_Device->cd_BCache;
    struct ISOVolume *iv = vol->cv_Private;
    ULONG sector;
    UBYTE *data, *dp;
    LONG size, err;
    int i;

    if (iv->iv_PathTable.Size == 0)
        return;

    sector = iv->iv_PathTable.Extent;
    size   = iv->iv_PathTable.Size;

    D(bug("%s: Reading path table @0x%08x, %d bytes\n", __func__, sector * 2048, size));
    data = AllocVec(size, MEMF_PUBLIC);
    if (!data)
        return;

    /* Read in the path table */
    dp = data;
    while (size > 0) {
        ULONG tocopy = size;
        UBYTE *cp;

        if (tocopy > bcache->bc_BlockSize)
            tocopy = bcache->bc_BlockSize;

        err = BCache_Read(bcache, sector, &cp);
        if (err != RETURN_OK) {
            FreeVec(data);
            return;
        }

        CopyMem(cp, dp, tocopy);
        sector++;
        dp += tocopy;
        size -= tocopy;
    }

    /* Count the number of entries in the PathTable */
    size = iv->iv_PathTable.Size;
    iv->iv_PathTable.Entries = 0;
    dp = data;
    while (size > 0) {
        UWORD len = (dp[0] + 8);
        if (len & 1)
            len++;
        dp += len;
        size -= len;
        iv->iv_PathTable.Entries++;
    }

    D(bug("%s: %d entries\n", __func__, iv->iv_PathTable.Entries));

    if (iv->iv_PathTable.Entries == 0) {
        FreeVec(data);
        iv->iv_PathTable.Size = 0;
        return;
    }

    iv->iv_PathTable.Entry = AllocVec(sizeof(struct isoPathTable *)*iv->iv_PathTable.Entries, MEMF_PUBLIC);
    if (iv->iv_PathTable.Entry == NULL) {
        FreeVec(data);
        iv->iv_PathTable.Size = 0;
        return;
    }

    iv->iv_PathTable.Data = data;

    /* Compute the pathtable offsets */
    dp = iv->iv_PathTable.Data;
    for (i = 0; i < iv->iv_PathTable.Entries; i++) {
        UWORD len = (dp[0] + 8);
        if (len & 1)
            len++;
        iv->iv_PathTable.Entry[i] = (struct isoPathTable *)dp;
        dp += len;
    }

    return;
}

LONG ISO9660_Mount(struct CDFSVolume *vol)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    struct BCache *bcache = vol->cv_Device->cd_BCache;
    struct ISOVolume *iv;
    LONG err;
    UQUAD block;
    BOOL gotdate = FALSE, gotname = FALSE;

    /* Don't mount filesystems with an invalid block size */
    if (bcache->bc_BlockSize != 2048)
        return ERROR_NOT_A_DOS_DISK;

    iv = AllocVec(sizeof(*iv), MEMF_PUBLIC | MEMF_CLEAR);
    if (!iv)
        return ERROR_NO_FREE_STORE;

    /* Check for an ISO9660 volume */
    for (block = 16;; block++) {
        struct isoVolume *iso;
        err = BCache_Read(bcache, block, (UBYTE **)&iso);
        if (err != RETURN_OK)
            break;

        if (memcmp(iso->Identifier,"CD001",5) != 0) {
            err = ERROR_NOT_A_DOS_DISK;
            break;
        }

        if (iso->Type == ISO_Volume_Primary) {
            int i;
            struct ISOLock *il = &iv->iv_RootLock;
            struct isoDirectory *dir = (APTR)&iso->Primary.RootDirectoryEntry[0];

            iv->iv_PathTable.Size = isoRead32LM(&iso->Primary.PathTableSize);
#if _BYTE_ORDER == _BIG_ENDIAN
            iv->iv_PathTable.Extent = iso->Primary.TypeMPathTable;
#elif _BYTE_ORDER == _LITTLE_ENDIAN
            iv->iv_PathTable.Extent = iso->Primary.TypeLPathTable;
#endif
            CopyMem(iso, &iv->iv_Volume, sizeof(*iso));
            for (i = 0; i < 32; i++) {
                iv->iv_Name[i+1] = iso->Primary.VolumeIdentifier[i];
                if (iv->iv_Name[i+1] == 0)
                    break;
            }
            /* Remove trailing spaces from the volume name */
            while (i > 0 && iv->iv_Name[i] == ' ') i--;
            iv->iv_Name[0] = i;
            iv->iv_Name[i+1] = 0;
            gotname = TRUE;

            /* Convert from ISO date to DOS Datestamp */
            ascdate2stamp(&iso->Primary.VolumeCreation, &vol->cv_DosVolume.dl_VolumeDate);
            gotdate = TRUE;

            isoParseDir(vol, il, dir, 0);
            isoReadDir(vol, il, il->il_Extent, 0, 0, NULL);
            il->il_Public.cl_FileLock.fl_Volume = MKB_VOLUME(vol);

            continue;
        }

        if (iso->Type == ISO_Volume_Terminator) {
            vol->cv_Private = iv;
            struct ISOLock *il = &iv->iv_RootLock;
            struct FileInfoBlock *fib = &il->il_Public.cl_FileInfoBlock;

            if (!gotname) {
                iv->iv_Name[0] = 4;
                iv->iv_Name[1] = 'I';
                iv->iv_Name[2] = 'S';
                iv->iv_Name[3] = 'O';
                iv->iv_Name[4] = '0';
                iv->iv_Name[5] = 0;
            }
#ifdef AROS_FAST_BSTR
            vol->cv_DosVolume.dl_Name = &iv->iv_Name[1];
#else
            vol->cv_DosVolume.dl_Name = MKBADDR(iv->iv_Name);
#endif
            if (!gotdate) {
                struct Library *DOSBase;
                if ((DOSBase = OpenLibrary("dos.library", 0))) {
                    DateStamp(&vol->cv_DosVolume.dl_VolumeDate);
                    CloseLibrary(DOSBase);
                }
            }

            /* We use the Volume as our root lock alias */
            vol->cv_DosVolume.dl_DiskType = AROS_MAKE_ID('I','S','O',0);
            vol->cv_DosVolume.dl_Lock = MKB_LOCK(&iv->iv_RootLock.il_Public);
            vol->cv_Private = iv;

            fib->fib_DirEntryType = ST_ROOT;
            CopyMem(AROS_BSTR_ADDR(vol->cv_DosVolume.dl_Name),
                    &fib->fib_FileName[1],
                    AROS_BSTR_strlen(vol->cv_DosVolume.dl_Name));
            fib->fib_FileName[0] = AROS_BSTR_strlen(vol->cv_DosVolume.dl_Name);

            /* Cache the path tables */
            isoReadPathTables(vol);

            return RETURN_OK;
        }
    }

    FreeVec(iv);
    vol->cv_Private = NULL;

    return err;
}

LONG ISO9660_Unmount(struct CDFSVolume *vol)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;

    FreeVec(vol->cv_Private);
    return ERROR_NO_FREE_STORE;
}

LONG ISO9660_Locate(struct CDFSVolume *vol, struct CDFSLock *idir, CONST_STRPTR file, ULONG mode, struct CDFSLock **ifile)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    struct ISOVolume *iv = vol->cv_Private;
    struct ISOLock *fl, *il, tl;
    CONST_STRPTR path, rest;
    LONG err;

    if (mode != MODE_OLDFILE)
        return ERROR_DISK_WRITE_PROTECTED;

    il = (struct ISOLock *)idir;

    D(bug("%s: 0x%08x: \"%s\"\n", __func__, il->il_ParentExtent, file));

    if (idir->cl_FileInfoBlock.fib_DirEntryType != ST_ROOT &&
        idir->cl_FileInfoBlock.fib_DirEntryType != ST_USERDIR) {
        D(bug("%s:  \"%s\" is not a directory\n", __func__, &idir->cl_FileInfoBlock.fib_FileName[1]));
        return ERROR_OBJECT_WRONG_TYPE;
    }

    fl = AllocVec(sizeof(*fl), MEMF_PUBLIC | MEMF_CLEAR);
    if (fl) {
        // Get the actual filename
        path = strchr(file, ':');
        if (path) {
            // Switch to the root directory
            il = (struct ISOLock *)B_LOCK(vol->cv_DosVolume.dl_Lock);
            path++;
        } else {
            path = file;
        }

        while (path[0] == 0 || path[0] == '/') {
            // ""
            if (path[0] == 0) {
                CopyMem(il, fl, sizeof(*fl));
                *ifile = &fl->il_Public;
                return RETURN_OK;
            }

            // "/"
            if (path[0] == '/') {
                /* Find parent */
                path++;

                /* Already at root? */
                if (il->il_Public.cl_FileInfoBlock.fib_DirEntryType == ST_ROOT)
                    continue;

                ASSERT(il->il_Public.cl_FileInfoBlock.fib_DirEntryType == ST_USERDIR);

                /* Otherwise, move to parent */
                if (il->il_PathTable) {
                    /* Non-RockRidge */
                    tl.il_PathTable = iv->iv_PathTable.Entry[il->il_PathTable->ParentDirectory-1];
                    isoParseDir(vol, &tl, NULL, 0);
                    il = &tl;
                    continue;
                } else {
                    err = isoReadDir(vol, &tl, il->il_ParentExtent, 0, 0, NULL);
                    if (err == RETURN_OK) {
                        /* .. and find its name in its parent */
                        if (tl.il_Public.cl_FileInfoBlock.fib_DirEntryType != ST_ROOT)
                            isoFindExtent(vol, &tl, tl.il_Extent, tl.il_ParentExtent);
                        il = &tl;
                        continue;
                    }
                }
                FreeVec(fl);
                return err;
            }
        }

        while (*path && (rest = strchr(path, '/'))) {
            int len;
            TEXT fname[MAXFILENAMELENGTH];

            if ((il->il_Public.cl_FileInfoBlock.fib_DirEntryType != ST_USERDIR) &&
                (il->il_Public.cl_FileInfoBlock.fib_DirEntryType != ST_ROOT)) {
                return ERROR_DIR_NOT_FOUND;
            }

            len = rest - path;
            if (len >= MAXFILENAMELENGTH) {
                FreeVec(fl);
                return ERROR_OBJECT_NOT_FOUND;
            }

            CopyMem(path, fname, len);
            fname[len] = 0;
            D(bug("%s: path=\"%s\" rest=\"%s\" fname=\"%s\"\n",
                        __func__, path, rest, fname));

            err = isoFindName(vol, fl, fname, il);
            D(bug("%s:%d isoFindName => %d\n", __func__, __LINE__, err));
            if (err != RETURN_OK) {
                FreeVec(fl);
                return err;
            }

            CopyMem(fl, &tl, sizeof(*fl));
            il = &tl;
            path = rest;
            while (*path == '/')
                path++;
        }

        if (*path == 0) {
            *ifile = &fl->il_Public;
            return RETURN_OK;
        }

        err = isoFindName(vol, fl, path, il);
        D(bug("%s:%d isoFindName => %d\n", __func__, __LINE__, err));
        if (err == RETURN_OK) {
            *ifile = &fl->il_Public;
            return RETURN_OK;
        }

        FreeVec(fl);
    } else {
        err = ERROR_NO_FREE_STORE;
    }

    return err;
}

VOID ISO9660_Close(struct CDFSVolume *vol, struct CDFSLock *fl)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    FreeVec(fl);
}

LONG ISO9660_ExamineNext(struct CDFSVolume *vol, struct CDFSLock *dl, struct FileInfoBlock *fib)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    ULONG offset;
    struct ISOLock tl = {}, *il = (struct ISOLock *)dl;
    LONG err;
    ULONG size;

    D(bug("%s: Root %p, This %p, DiskKey %d of %d\n", __func__,
                &((struct ISOVolume *)vol->cv_Private)->iv_RootLock, dl,
                fib->fib_DiskKey, dl->cl_FileInfoBlock.fib_Size));

    /* Do we have a valid size for this directory? */
    if (dl->cl_FileInfoBlock.fib_Size == 0) {
        struct isoDirectory *dir;
        err = BCache_Read(vol->cv_Device->cd_BCache, il->il_Extent, (UBYTE **)&dir);
        if (err != RETURN_OK)
            return err;
        dl->cl_FileInfoBlock.fib_Size = isoRead32LM(&dir->DataLength);
    }
            
    if (fib->fib_DiskKey == 0) {
        /* Skip the first two entries */
        err = isoReadDir(vol, &tl, il->il_Extent, 0, 0, &size);
        if (err != RETURN_OK)
            return err;
        fib->fib_DiskKey = size;
    }

    if (fib->fib_DiskKey >= dl->cl_FileInfoBlock.fib_Size) {
        D(bug("%s: No more entries (%d >= %d)\n", __func__, fib->fib_DiskKey, dl->cl_FileInfoBlock.fib_Size));
        return ERROR_NO_MORE_ENTRIES;
    }

    offset = fib->fib_DiskKey;

    while (offset < dl->cl_FileInfoBlock.fib_Size) {
        err = isoReadDir(vol, &tl, il->il_Extent, offset, il->il_Extent, &size);

        if (err != RETURN_OK && err != ERROR_OBJECT_WRONG_TYPE)
            break;

        D(bug("%s: err=%d, size=%d\n", __func__, err, size));
        offset += size;

        /* Move to next block */
        if (err == ERROR_OBJECT_WRONG_TYPE || size == 0 ||
            (2048 - (offset % 2048)) < sizeof(struct isoDirectory) + 2) {
            offset = (offset + 2048 - 1) & ~(2048 - 1);
        }

        if (err == RETURN_OK) {
            struct FileInfoBlock *tfib = &tl.il_Public.cl_FileInfoBlock;
            CopyMem(tfib, fib, sizeof(*fib));
            break;
        }

        err = ERROR_NO_MORE_ENTRIES;
    } 

    fib->fib_DiskKey = offset;

    D(bug("%s: Error %d\n", __func__, err));

    return err;
}

LONG ISO9660_Read(struct CDFSVolume *vol, struct CDFSLock *fl, APTR buff, SIPTR len, SIPTR *actualp)
{
    struct CDFS *cdfs = vol->cv_CDFSBase;
    struct BCache *bcache = vol->cv_Device->cd_BCache;
    struct ISOLock *il = (struct ISOLock *)fl;
    UQUAD size = fl->cl_FileInfoBlock.fib_Size;
    ULONG block;
    ULONG offset;
    LONG err = RETURN_OK;
    SIPTR actual = 0;
    UBYTE *cache;

    block = il->il_Offset / 2048;
    offset = il->il_Offset % 2048;
    if ((len + il->il_Offset) > size)
        len = size - il->il_Offset;

    while (len) {
        ULONG tocopy = 2048 - offset;
        if (tocopy > len)
            tocopy = len;

        err = BCache_Read(bcache, il->il_Extent + block, &cache);
        if (err != RETURN_OK)
            break;
        CopyMem(cache + offset, buff, tocopy);

        buff += tocopy;
        len -= tocopy;
        actual += tocopy;
        il->il_Offset += tocopy;

        offset = 0;
        block++;
    }

    *actualp = actual;
    return err;
}

LONG ISO9660_Seek(struct CDFSVolume *vol, struct CDFSLock *fl, SIPTR pos, LONG mode, SIPTR *oldpos)
{
    struct ISOLock *il = (struct ISOLock *)fl;
    UQUAD size = fl->cl_FileInfoBlock.fib_Size;

    *oldpos = il->il_Offset;
    switch (mode) {
    case OFFSET_BEGINNING:
        if (pos < 0 || pos > size)
            return ERROR_SEEK_ERROR;
        il->il_Offset = pos;
        break;
    case OFFSET_CURRENT:
        if (-pos > il->il_Offset ||
            (il->il_Offset + pos) > size)
            return ERROR_SEEK_ERROR;
        il->il_Offset += pos;
        break;
    case OFFSET_END:
        if (pos < 0 || -pos > size)
            return ERROR_SEEK_ERROR;
        il->il_Offset = size + pos;
        break;
    default:
        return ERROR_SEEK_ERROR;
    }

    return RETURN_OK;
}

const struct CDFSOps ISO9660_Ops = {
    .op_Type = AROS_MAKE_ID('I','S','O',0),
    .op_Mount = ISO9660_Mount,
    .op_Unmount = ISO9660_Unmount,
    .op_Locate = ISO9660_Locate,
    .op_Close = ISO9660_Close,
    .op_ExamineNext = ISO9660_ExamineNext,
    .op_Seek = ISO9660_Seek,
    .op_Read = ISO9660_Read,
};
