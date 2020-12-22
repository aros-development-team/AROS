/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <libraries/mui.h>

#include <dos/dos.h>
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>

#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/colorwheel.h>

#include <libraries/asl.h>
#include <libraries/expansionbase.h>

#include <devices/trackdisk.h>
#include <devices/scsidisk.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <mui/TextEditor_mcc.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_bootloader.h"

extern struct ExpansionBase *ExpansionBase;

extern Object *optObjCycleSysUnits;
extern Object *optObjCycleWorkUnits;

extern Object *optObjCheckSysSize;
extern Object *optObjCheckSizeWork;
extern Object *sys_size;
extern Object *work_size;

char * GetDevNameForVolume(char *volumeName)
{
    struct DosList *dl;
    char *devName = NULL;

    /* Get the work drives device name */
    dl = LockDosList(LDF_VOLUMES | LDF_READ);
    dl = FindDosEntry(dl, volumeName, LDF_VOLUMES | LDF_READ);
    UnLockDosList(LDF_VOLUMES | LDF_READ);

    if (dl != NULL)
    {
        APTR voltask = dl->dol_Task;
        D(bug("[InstallAROS] %s: dosentry for '%s' @ %p\n", __func__, volumeName, dl));
        dl = LockDosList(LDF_DEVICES|LDF_READ);
        if (dl) {
            while((dl = NextDosEntry(dl, LDF_DEVICES)))
            {
                if (dl->dol_Task == voltask)
                {
                    struct FileSysStartupMsg *fsstartup = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
                    D(bug("[InstallAROS] %s:     device entry '%s' @ %p\n", __func__, AROS_BSTR_ADDR(dl->dol_Name), dl));
                    devName = AROS_BSTR_ADDR(dl->dol_Name);
                    D(bug("[InstallAROS] %s:     devName '%s'\n", __func__, devName));
                }
            }
            UnLockDosList(LDF_VOLUMES|LDF_READ);
        }
    }
    return devName;
}

BOOL GetVolumeForDevName(char *devName, char *buffer)
{
    BPTR tmpLock;
    BOOL retVal = FALSE;

    if ((tmpLock = Lock(devName, SHARED_LOCK)) != BNULL)
    {
        if (NameFromLock(tmpLock, buffer, 100))
        {
            D(bug("[InstallAROS] %s: '%s' = '%s'\n", __func__, devName, buffer));
            retVal = TRUE;
        }
        UnLock(tmpLock);
    }
    return retVal;
}

/* Return the unit number of the drive that's likely to be the first in the
 * system, or zero if none found */
ULONG GuessFirstHD(CONST_STRPTR device)
{
    struct PartitionHandle *ph;
    ULONG i;
    BOOL found = FALSE;

    for (i = 0; i < 8 && !found; i++)
    {
        ph = OpenRootPartition(device, i);
        if (ph != NULL)
        {
            found = TRUE;
            CloseRootPartition(ph);
        }
    }
    if (!found)
        i = 1;

    return i - 1;
}

/* Return TRUE if we suspect a floppy disk */
BOOL myCheckFloppy(struct DosEnvec * DriveEnv)
{
    switch (DriveEnv->de_HighCyl)
    {
    case 79:
        /* Standard Floppy size
           for PC floppies, DD = 9, HD = 18
           for Amiga floppies, DD = 11, HD = 22
         */
        if ((DriveEnv->de_BlocksPerTrack == 18) ||
            (DriveEnv->de_BlocksPerTrack == 9) ||
            (DriveEnv->de_BlocksPerTrack == 22) ||
            (DriveEnv->de_BlocksPerTrack == 11))
            return TRUE;

        break;
    case 2890:
        /* Standard Zip (95Mb) */
        if ((DriveEnv->de_BlocksPerTrack == 60) ||
            (DriveEnv->de_BlocksPerTrack == 68))
            return TRUE;
    case 196601:
    case 196607:
        /* Standard Zip & LS120 sizes */
        if (DriveEnv->de_BlocksPerTrack == 1)
            return TRUE;
    default:
        break;
    }
    /* OK - shouldn't be a floppy... */
    return FALSE;
}

char *CheckPartition(struct PartitionHandle *part, char **name)
{
    struct PartitionType *type = NULL;
    struct PartitionType pttype;
    char *success = NULL;

    D(bug("[InstallAROS] %s: checking PARTITION\n", __func__));

    *name = AllocVec(100, MEMF_CLEAR | MEMF_PUBLIC);

    GetPartitionAttrsTags
        (part,
        PT_NAME, (IPTR)*name, PT_TYPE, (IPTR) & pttype, TAG_DONE);

    type = &pttype;
    D(bug("[InstallAROS] %s: Type Len = %u\n", __func__, type->id_len));
    if (type->id_len == 4)
    {
        D(bug("[InstallAROS] %s: Found RDB Partition!\n", __func__));
        D(bug("[InstallAROS] %s: type '%c%c%c%c'\n", __func__, type->id[0], type->id[1], type->id[2], type->id[3]));
        if ((type->id[0] == 68) && (type->id[1] == 79)
            && (type->id[2] == 83))
        {
            D(bug("[InstallAROS] %s: Found AFFS Partition! '%s'\n", __func__, *name));
            success = *name;
        }
        else if ((type->id[0] == 83) && (type->id[1] == 70)
            && (type->id[2] == 83))
        {
            D(bug("[InstallAROS] %s: Found SFS Partition! '%s'\n", __func__, *name));
            success = *name;
        }
    }
    else if (type->id_len == 3)
    {
        D(bug("[InstallAROS] %s: type '%c%c%c'\n", __func__, type->id[0], type->id[1], type->id[2]));
    }
    else if (type->id_len == 2)
    {
        D(bug("[InstallAROS] %s: type '%c%c'\n", __func__, type->id[0], type->id[1]));
    }
    return  success;
}

/* Returns the first AROS-supported filesystem's name */
char *FindPartition(struct PartitionHandle *root)
{
    struct PartitionHandle *partition = NULL;
    char *success = NULL;
    char *name = NULL;

    D(bug("[InstallAROS] %s()\n", __func__));
    
    if (root->table)
    {
        ForeachNode(&root->table->list, partition)
        {
            D(bug("[InstallAROS] %s: checking part\n", __func__));

            if (OpenPartitionTable(partition) == 0)
            {
                D(bug("[InstallAROS] %s: checking Child Parts... \n", __func__));
                success = FindPartition(partition);
                ClosePartitionTable(partition);
                D(bug("[InstallAROS] %s: Children Done...\n", __func__));
            }
            else
            {
                success = CheckPartition(partition, &name);
            }
            if (success != NULL)
            {
                D(bug("[InstallAROS] %s: Found '%s'\n", __func__, success));
                break;
            }
        }
    }
    else
    {
        success = CheckPartition(root, &name);
    }

    D(bug("[InstallAROS] %s: Scan finished\n", __func__));

    if ((!success) && (name))
        FreeVec(name);

    D(bug("[InstallAROS] %s: returning %p\n", __func__, success));

    return success;
}

struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path)
{
    struct DosList *dl;
    struct DeviceNode *dn;
    TEXT dname[32];
    UBYTE i;

    D(bug("[InstallAROS] %s('%s')\n", __func__, path));

    for (i = 0; (path[i]) && (path[i] != ':'); i++)
        dname[i] = path[i];
    if (path[i] == ':')
    {
        dname[i] = 0;
        dl = LockDosList(LDF_READ);
        if (dl)
        {
            dn = (struct DeviceNode *)FindDosEntry(dl, dname, LDF_DEVICES);
            UnLockDosList(LDF_READ);
            if (dn)
            {
                if (IsFileSystem(dname))
                {
                    return (struct FileSysStartupMsg *)BADDR(dn->
                        dn_Startup);
                }
                else
                    printf(_(MSG_NOTFILESYSTEM), dname);
            }
        }
    }
    else
        printf(_(MSG_NOTDEVICE), path);
    return NULL;
}

LONG GetPartitionSize(BOOL get_work)
{
    LONG size = -1;
    IPTR tmp = 0;

    if (!get_work)
    {
        if ((BOOL) XOPTOGET(optObjCheckSysSize, MUIA_Selected))
        {
            GET(sys_size, MUIA_String_Integer, &tmp);
            size = (LONG) tmp;
            if (XOPTOGET(optObjCycleSysUnits, MUIA_Cycle_Active) == 1)
                size *= 1024;
        }
    }
    else
    {
        if ((BOOL) XOPTOGET(optObjCheckSizeWork, MUIA_Selected))
        {
            GET(work_size, MUIA_String_Integer, &tmp);
            size = (LONG) tmp;
            if (XOPTOGET(optObjCycleWorkUnits, MUIA_Cycle_Active) == 1)
                size *= 1024;
        }
    }

    return size;
}

BOOL SkipPath(char *matchName, struct List *SkipList)
{
    struct Node *matchNode;
    ForeachNode(SkipList, matchNode)
    {
        if (!strcmp(matchName, matchNode->ln_Name))
        {
            D(bug("[InstallAROS] %s: %s\n", __func__, matchName));
            return TRUE;
        }
    }
    return FALSE;
}

LONG CountFiles(CONST_STRPTR directory, struct List *SkipList, CONST_STRPTR fileMask,
    BOOL recursive)
{
    UBYTE *buffer = NULL;
    TEXT matchString[3 * strlen(fileMask)];
    BPTR dirLock = BNULL;
    LONG fileCount = 0;

    D(bug("[InstallAROS] %s: Entry, directory: %s, mask: %s\n", __func__, directory,
            fileMask));

    /* Check if directory exists */
    dirLock = Lock(directory, SHARED_LOCK);

    if (dirLock == BNULL)
    {
        return -1;
    }

    buffer = AllocVec(kExallBufSize, MEMF_CLEAR | MEMF_PUBLIC);

    if (buffer == NULL)
    {
        UnLock(dirLock);
        return -1;
    }

    if (ParsePatternNoCase(fileMask, matchString, 3 * strlen(fileMask)) < 0)
    {
        UnLock(dirLock);
        FreeVec(buffer);
        return -1;
    }

    struct ExAllData *ead = (struct ExAllData *)buffer;
    struct ExAllControl *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    BOOL loop;
    struct ExAllData *oldEad = ead;

    do
    {
        ead = oldEad;
        loop = ExAll(dirLock, ead, kExallBufSize, ED_COMMENT, eac);

        if (!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
            break;

        if (eac->eac_Entries != 0)
        {
            do
            {
                ULONG entryPathLen = strlen(directory) + strlen(ead->ed_Name) + 2;
                TEXT entryPath[entryPathLen];

                sprintf(entryPath, "%s", directory);
                AddPart(entryPath, ead->ed_Name, entryPathLen);

                if ((!SkipList) || (!SkipPath(entryPath, SkipList)))
                {
                    if (ead->ed_Type == ST_FILE
                        && MatchPatternNoCase(matchString, ead->ed_Name))
                        fileCount++;

                    if (ead->ed_Type == ST_USERDIR && recursive)
                    {
                        LONG subFileCount =
                            CountFiles(entryPath, SkipList, fileMask, recursive);

                        if (subFileCount >= 0)
                            fileCount += subFileCount;
                        else
                        {
                            /* Error at lower level */
                            FreeDosObject(DOS_EXALLCONTROL, eac);
                            UnLock(dirLock);
                            FreeVec(buffer);
                            return -1;
                        }
                    }
                }
                ead = ead->ed_Next;
            }
            while (ead != NULL);
        }
    }
    while (loop);

    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(dirLock);
    FreeVec(buffer);

    return fileCount;
}

LONG InternalCopyFiles(Class * CLASS, Object * self, CONST_STRPTR srcDir,
    CONST_STRPTR dstDir, struct List *SkipList, CONST_STRPTR fileMask, BOOL recursive,
    LONG totalFiles, LONG totalFilesCopied)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    UBYTE *buffer = NULL;
    TEXT matchString[3 * strlen(fileMask)];
    BPTR srcDirLock = BNULL, dstDirLock = BNULL;
    LONG totalFilesCopiedThis = 0;

    /* Check entry condition */
    if (data->inst_success != MUIV_Inst_InProgress)
        return totalFilesCopied;

    /* Check if source directory exists */
    do
    {
        srcDirLock = Lock(srcDir, SHARED_LOCK);

        if (srcDirLock == BNULL)
        {
            ULONG retry =
                AskRetry(CLASS, self, _(MSG_FINDRETRY), srcDir,
                _(MSG_YES), _(MSG_SKIP), _(MSG_QUIT));
            switch (retry)
            {
            case 0:            /*retry */
                break;
            case 1:            /* skip */
                return -1;
            default:           /* quit */
                DoMethod(self, MUIM_IC_QuitInstall);
                return -1;
            }
        }
    }
    while (srcDirLock == BNULL);

    /* Check if destination directory exists and create it */
    dstDirLock = Lock(dstDir, SHARED_LOCK);

    if (dstDirLock != BNULL)
        UnLock(dstDirLock);
    else
    {
        dstDirLock = RecursiveCreateDir(dstDir);
        if (dstDirLock != NULL)
            UnLock(dstDirLock);
        else
        {
            UnLock(srcDirLock);
            return -1;
        }
    }

    /* Allocate buffer for ExAll */
    buffer = AllocVec(kExallBufSize, MEMF_CLEAR | MEMF_PUBLIC);

    if (buffer == NULL)
    {
        UnLock(srcDirLock);
        return -1;
    }

    if (ParsePatternNoCase(fileMask, matchString, 3 * strlen(fileMask)) < 0)
    {
        UnLock(srcDirLock);
        FreeVec(buffer);
        return -1;
    }

    struct ExAllData *ead = (struct ExAllData *)buffer;
    struct ExAllControl *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    BOOL loop;
    struct ExAllData *oldEad = ead;


    /* Main copy file loop */
    do
    {
        ead = oldEad;
        loop = ExAll(srcDirLock, ead, kExallBufSize, ED_COMMENT, eac);

        if (!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
            break;

        if (eac->eac_Entries != 0)
        {
            do
            {
                ULONG srcLen = strlen(srcDir);
                ULONG newSrcLen = srcLen + strlen(ead->ed_Name) + 2;
                TEXT srcPath[newSrcLen];

                sprintf(srcPath, "%s", srcDir);
                AddPart(srcPath, ead->ed_Name, newSrcLen);

                if ((!SkipList) || (!SkipPath(srcPath, SkipList)))
                {
                    if (((ead->ed_Type == ST_FILE)
                            && MatchPatternNoCase(matchString, ead->ed_Name))
                        || ((ead->ed_Type == ST_USERDIR) && recursive))
                    {
                        ULONG dstLen = strlen(dstDir);
                        ULONG newDstLen = dstLen + strlen(ead->ed_Name) + 2;
                        TEXT sdtPath[newDstLen];

                        sprintf(sdtPath, "%s", dstDir);
                        AddPart(sdtPath, ead->ed_Name, newDstLen);

                        if (ead->ed_Type == ST_FILE)
                        {
                            totalFilesCopiedThis +=
                                (ULONG) DoMethod(self, MUIM_IC_CopyFile,
                                srcPath, sdtPath);

                            if (totalFiles > 0)
                            {
                                SET(data->gauge2, MUIA_Gauge_Current,
                                    (LONG) ((100.0 / totalFiles) *
                                        (totalFilesCopied +
                                            totalFilesCopiedThis)));
                            }
                        }

                        if (ead->ed_Type == ST_USERDIR)
                        {

                            LONG totalFilesCopiedSub =
                                InternalCopyFiles(CLASS, self, srcPath, sdtPath,
                                SkipList, fileMask,
                                recursive, totalFiles,
                                totalFilesCopied + totalFilesCopiedThis);
                            if (totalFilesCopiedSub >= 0)
                                totalFilesCopiedThis += totalFilesCopiedSub;
                            else
                            {
                                /* Do nothing. It will be caught at level of Install__MUIM_IC_CopyFiles */
                            }
                        }
                    }
                }
                ead = ead->ed_Next;
            }
            while ((ead != NULL)
                && (data->inst_success == MUIV_Inst_InProgress));
        }
    }
    while ((loop) && (data->inst_success == MUIV_Inst_InProgress));

    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(srcDirLock);
    FreeVec(buffer);

    return totalFilesCopiedThis;
}

void AddSkipListEntry(struct List *SkipList, char *SkipEntry)
{
    struct Node *entry = AllocVec(sizeof(struct Node), MEMF_CLEAR);
    entry->ln_Name = AllocVec(strlen(SkipEntry) + 1, MEMF_CLEAR);
    strcpy(entry->ln_Name, SkipEntry);
    AddTail(SkipList, entry);
}

void ClearSkipList(struct List *SkipList)
{
    struct Node *entry, *tmp;
    ForeachNodeSafe(SkipList, entry, tmp)
    {
        Remove(entry);
        FreeVec(entry->ln_Name);
        FreeVec(entry);
    }
}

LONG CopyDirArray(Class * CLASS, Object * self, CONST_STRPTR sourcePath,
    CONST_STRPTR destinationPath, CONST_STRPTR directories[], struct List *SkipList)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    LONG numdirs = 0, dir_count = 0;
    BPTR lock = BNULL;

    while (directories[numdirs] != NULL)
        numdirs++;

    numdirs = (numdirs - 1) / 2;

    D(bug("[InstallAROS] %s: Copying %d Dirs...\n", __func__, numdirs);)

    while ((directories[dir_count] != NULL)
        && (data->inst_success == MUIV_Inst_InProgress))
    {
        ULONG newSrcLen =
            strlen(sourcePath) + strlen(directories[dir_count]) + 2;
        ULONG newDstLen =
            strlen(destinationPath) + strlen(directories[dir_count + 1]) +
            2;

        TEXT srcDirs[newSrcLen + strlen(".info")];
        TEXT dstDirs[newDstLen + strlen(".info")];

        sprintf(srcDirs, "%s", sourcePath);
        sprintf(dstDirs, "%s", destinationPath);
        AddPart(srcDirs, directories[dir_count], newSrcLen);
        AddPart(dstDirs, directories[dir_count + 1], newDstLen);

        SET(data->actioncurrent, MUIA_Text_Contents, strchr(srcDirs,
                ':') + 1);

        /* OK Now copy the contents */
        DoMethod(self, MUIM_IC_CopyFiles, srcDirs, dstDirs, SkipList, data->install_Pattern, TRUE);

        if (data->inst_success == MUIV_Inst_InProgress)
        {
            /* Check if folder has an icon */
            CopyMem(".info", srcDirs + strlen(srcDirs),
                strlen(".info") + 1);
            CopyMem(".info", dstDirs + strlen(dstDirs),
                strlen(".info") + 1);
            /* If the icon already exists in the destination, don't overwrite it.
               It may contain snapshotted position and/or edited tooltypes.
               TODO: may be consider replacing icon's image here using icon.library ? */
            if ((lock = Lock(dstDirs, SHARED_LOCK)) != BNULL)
            {
                UnLock(lock);
            }
            else
            {
                if ((lock = Lock(srcDirs, SHARED_LOCK)) != NULL)
                {
                    UnLock(lock);
                    DoMethod(self, MUIM_IC_CopyFile, srcDirs, dstDirs);
                }
            }
        }

        /* Folder copied */
        dir_count += 2;
    }

    return dir_count / 2;       /* Return no. of copied dirs */
}

BOOL FormatPartition(CONST_STRPTR device, CONST_STRPTR name, ULONG dostype)
{
    BOOL success = FALSE;

    if (Inhibit(device, DOSTRUE))
    {
        success = Format(device, name, dostype);
        Inhibit(device, DOSFALSE);
    }

    return success;
}

BPTR RecursiveCreateDir(CONST_STRPTR dirpath)
{
    /* Will create directory even if top level directory does not exist */

    BPTR lock = BNULL;
    ULONG lastdirseparator = 0;
    ULONG dirpathlen = strlen(dirpath);
    STRPTR tmpdirpath = AllocVec(dirpathlen + 2, MEMF_CLEAR | MEMF_PUBLIC);

    D(bug("[InstallAROS] %s('%s')\n", __func__, dirpath);)

    CopyMem(dirpath, tmpdirpath, dirpathlen);

    /* Recurvice directory creation */
    while (TRUE)
    {
        if (lastdirseparator >= dirpathlen)
            break;

        for (; lastdirseparator < dirpathlen; lastdirseparator++)
            if (tmpdirpath[lastdirseparator] == '/')
                break;

        tmpdirpath[lastdirseparator] = '\0';    /* cut */

        /* Unlock any lock from previous interation. Last iteration lock will be returned. */
        if (lock != BNULL)
        {
            UnLock(lock);
            lock = BNULL;
        }

        /* Check if directory exists */
        lock = Lock(tmpdirpath, SHARED_LOCK);
        if (lock == BNULL)
        {
            lock = CreateDir(tmpdirpath);
            if (lock == BNULL)
                break;          /* Error with creation */
        }

        tmpdirpath[lastdirseparator] = '/';     /* restore */
        lastdirseparator++;
    }

    FreeVec(tmpdirpath);
    return lock;
}

BOOL BackUpFile(CONST_STRPTR filepath, CONST_STRPTR backuppath,
    struct InstallC_UndoRecord * undorecord)
{
    ULONG filepathlen = strlen(filepath);
    ULONG backuppathlen = strlen(backuppath);
    ULONG i = 0;
    STRPTR tmp = NULL;
    STRPTR pathpart = NULL;
    BPTR lock = BNULL, from = BNULL, to = BNULL;
    static TEXT buffer[kBufSize];
    BOOL err = FALSE;

    if (undorecord == NULL)
        return FALSE;

    undorecord->undo_src =
        AllocVec(filepathlen + backuppathlen + 3, MEMF_CLEAR | MEMF_PUBLIC);
    undorecord->undo_dst =
        AllocVec(filepathlen + 2, MEMF_CLEAR | MEMF_PUBLIC);

    /* Create backup file name */
    tmp = AllocVec(filepathlen + 2, MEMF_CLEAR | MEMF_PUBLIC);
    CopyMem(filepath, tmp, filepathlen);
    for (i = 0; i < filepathlen; i++)
        if (tmp[i] == ':')
            tmp[i] = '/';       /* Substitute : with / */
    sprintf(undorecord->undo_src, "%s/%s", backuppath, tmp);
    FreeVec(tmp);

    /* Create source file name */
    CopyMem(filepath, undorecord->undo_dst, filepathlen);

    /* Create backup file path */
    tmp =
        AllocVec(strlen(undorecord->undo_src) + 2,
        MEMF_CLEAR | MEMF_PUBLIC);
    CopyMem(undorecord->undo_src, tmp, strlen(undorecord->undo_src));
    pathpart = PathPart(tmp);
    if (pathpart == NULL)
    {
        FreeVec(tmp);
        return FALSE;
    }
    *pathpart = '\0';           /* 'cut' string at end of path */

    D(bug("[InstallAROS] %s: Backup '%s' @ '%s'\n", __func__, undorecord->undo_dst,
            undorecord->undo_src);)

    undorecord->undo_method = MUIM_IC_CopyFile;

    /* Create backup directory */
    if ((lock = Lock(tmp, SHARED_LOCK)) != BNULL)
    {
        D(bug("[InstallAROS] %s: Dir '%s' Exists - no need to create\n", __func__, tmp);)
        UnLock(lock);
    }
    else
    {
        lock = RecursiveCreateDir(tmp);
        if (lock != BNULL)
            UnLock(lock);
        else
        {
            D(bug("[InstallAROS] %s: Failed to create %s dir!!\n", __func__, tmp));
            FreeVec(tmp);
            return FALSE;
        }
    }

    FreeVec(tmp);

    /* Copy file */
    if ((from = Open(undorecord->undo_dst, MODE_OLDFILE)) != BNULL)
    {
        if ((to = Open(undorecord->undo_src, MODE_NEWFILE)) != BNULL)
        {
            LONG s = 0;

            do
            {
                if ((s = Read(from, buffer, kBufSize)) == -1)
                {
                    err = TRUE;
                    break;
                };

                if (Write(to, buffer, s) == -1)
                {
                    err = TRUE;
                    break;
                };

            }
            while (s == kBufSize && !err);

            Close(to);
        }

        Close(from);
    }

    return !err;
}
