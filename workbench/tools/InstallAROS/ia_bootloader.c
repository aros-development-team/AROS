/*
    Copyright (C) 2018-2023, The AROS Development Team. All rights reserved.
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
#include <dos/filehandler.h>
#include <libraries/partition.h>
#include <libraries/uuid.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_stage.h"
#include "ia_option.h"
#include "ia_stage_intern.h"
#include "ia_bootloader.h"
#include "ia_diskio.h"
#include "ia_log.h"

extern char *source_Path;

extern Object   *cycle_drivetype;
extern Object   *optObjCycleGrub2Mode;
BOOL            gfx_font_exists;

LONG BootLoaderType;
#ifdef INSTALL_BL_PC_EFI
LONG BIOSBootLoaderType;
#endif

#define GRUB_COPY_FILE_LOOP(files)                                              \
SET(data->gauge2, MUIA_Gauge_Current, 0);                                       \
                                                                                \
while (files[file_count] != NULL && data->inst_success == MUIV_Inst_InProgress) \
{                                                                               \
    ULONG newSrcLen = strlen(srcPath) + strlen(files[file_count]) + 2;        \
    ULONG newDstLen = strlen(dstPath) + strlen(files[file_count + 1]) + 2;    \
                                                                                \
    TEXT srcFile[newSrcLen];                                                    \
    TEXT dstFile[newDstLen];                                                    \
                                                                                \
    sprintf(srcFile, "%s", srcPath);                                            \
    sprintf(dstFile, "%s", dstPath);                                            \
    AddPart(srcFile, files[file_count], newSrcLen);                         \
    AddPart(dstFile, files[file_count + 1], newDstLen);                     \
                                                                                \
    DoMethod(self, MUIM_IC_CopyFile, srcFile, dstFile);                         \
                                                                                \
    file_count += 2;                                                            \
                                                                                \
    if (numgrubfiles > 0)                                                       \
    {                                                                           \
        SET(data->gauge2, MUIA_Gauge_Current,                                   \
            (LONG)((100.0 / numgrubfiles) * (file_count / 2.0)));               \
    }                                                                           \
}

/* Files to check for. Must be in the same order as BootLoaderTypes enum values */

struct BootLoaderInfo BootLoaderData[] = {
#if defined(INSTALL_BL_GRUB)
    {
        "grub",
        "SYS:",
        "boot/grub/stage1",
        "c/Install-grub",
        NULL
    },
#endif
#if defined(INSTALL_BL_GRUB2)
    {
        "grub",
        "SYS:",
        "boot/grub/" GRUBARCHDIR "/core.img",
        "c/Install-grub2",
        "boot/grub/fonts/unicode.pf2"
    },

#endif
#if defined(INSTALL_BL_EFI)
    {
        "EFI",
        "SYS:",
        "EFI/BOOT/" EFI_STUB_FILE,
        NULL,
        NULL
    },
#endif
    {
        (CONST_STRPTR)-1,
        (CONST_STRPTR)-1,
        (CONST_STRPTR)-1,
        (CONST_STRPTR)-1
    }
};

#define BOOTLOADER_PATH_LEN 64  /* Must be large enough to contain any of the bootloader match strings */

BOOL _matchPath(CONST_STRPTR *match)
{
    ULONG matchSrcLen = strlen(source_Path) + BOOTLOADER_PATH_LEN;
    TEXT matchSrc[matchSrcLen];
    BPTR lock;

    strcpy(matchSrc, source_Path);
    AddPart(matchSrc, *match, matchSrcLen);

    D(bug("[InstallAROS] %s: trying match '%s'\n", __func__, matchSrc));

    lock = Lock(matchSrc, ACCESS_READ);
    if (lock != BNULL)
    {
        UnLock(lock);
        return TRUE;
    }
    *match = NULL;
    return FALSE;
}

void BOOTLOADER_InitSupport(void)
{
    LONG i, lastmatch;
    BootLoaderType = lastmatch = BOOTLOADER_NONE;
#ifdef INSTALL_BL_PC_EFI
    BIOSBootLoaderType = BOOTLOADER_NONE;
#endif

    D(bug("[InstallAROS] %s()\n", __func__));

    for (i = 0; BootLoaderData[i].match != (CONST_STRPTR)-1; i++)
    {
        D(bug("[InstallAROS] %s: #%d = '%s'\n", __func__, i, BootLoaderData[i].path));

        if (BootLoaderData[i].match && _matchPath(&BootLoaderData[i].match))
        {
            D(bug("[InstallAROS] %s: bootloader files found\n", __func__));
            /* A NULL match2 means the backend needs no installer command
               of its own - the stub is the whole bootloader */
            if ((BootLoaderData[i].match2 == NULL)
                || _matchPath(&BootLoaderData[i].match2))
            {
                D(bug("[InstallAROS] %s: bootloader installer found\n", __func__));
#if defined(INSTALL_BL_GRUB)
# if defined(GRUB) && (GRUB == 1)
                if (i == BOOTLOADER_GRUB1)
                    BootLoaderType = i;
#endif
#endif
#if defined(INSTALL_BL_GRUB2)
# if defined(GRUB) && (GRUB == 2)
                if (i == BOOTLOADER_GRUB2)
                    BootLoaderType = i;
#endif
#endif
                lastmatch = i;
#ifdef INSTALL_BL_PC_EFI
                if (i != BOOTLOADER_EFI)
                    BIOSBootLoaderType = i;
#endif

                if (BootLoaderData[i].match3)
                    _matchPath(&BootLoaderData[i].match3);
            }
            else
                BootLoaderData[i].match = NULL;
        }
    }
#if defined(INSTALL_BL_GRUB2)
    if (BootLoaderData[BOOTLOADER_GRUB2].match3)
    {
        gfx_font_exists = TRUE;
        OPTOSET(optObjCycleGrub2Mode, MUIA_Cycle_Active, 1);
    }
    else
        gfx_font_exists = FALSE;
#endif
    if ((BootLoaderType == BOOTLOADER_NONE) && (lastmatch != BOOTLOADER_NONE))
        BootLoaderType = lastmatch;
#ifdef INSTALL_BL_PC_EFI
    if (BootLoaderType != BOOTLOADER_EFI && BootLoaderType != BOOTLOADER_NONE)
        BIOSBootLoaderType = BootLoaderType;
    if (BootLoaderData[BOOTLOADER_EFI].match && OpenResource("efi.resource"))
        BootLoaderType = BOOTLOADER_EFI;
#endif
}

void BOOTLOADER_AddCoreSkipPaths(struct List *SkipList)
{
    ULONG skipPathLen = strlen(source_Path) + BOOTLOADER_PATH_LEN;
    TEXT skipPath[skipPathLen];
#if defined(INSTALL_BL_GRUB) || defined(INSTALL_BL_GRUB2)
    BOOL grubSkipped = FALSE;
#endif

#if defined(INSTALL_BL_GRUB)
    strcpy(skipPath, source_Path);
    AddPart(skipPath, "boot/grub", skipPathLen);
    AddSkipListEntry(SkipList, skipPath);
    grubSkipped = TRUE;
#endif
#if defined(INSTALL_BL_GRUB2)
    if (!grubSkipped)
    {
        strcpy(skipPath, source_Path);
        AddPart(skipPath, "boot/grub", skipPathLen);
        AddSkipListEntry(SkipList, skipPath);
        grubSkipped = TRUE;
    }
#endif
}

BOOL BOOTLOADER_PartFixUp(struct InstallStage_DATA *data, IPTR systype)
{
    D(bug("[InstallAROS] %s()\n", __func__));

#if defined(INSTALL_BL_GRUB)
    /* Warn user about using non FFS-Intl filesystem for system
       partition with GRUB */
    if ((BootLoaderType == BOOTLOADER_GRUB1) && (systype != 0))
    {
        if (MUI_RequestA(data->installer, data->window, 0,
                _(MSG_WARNING),
                _(MSG_CONTINUECANCELPART),
                _(MSG_GRUBNONFFSWARNING), NULL) != 1)
            return FALSE;
    }
#endif
    return TRUE;
}

static LONG FindWindowsPartition(STRPTR device, LONG unit)
{
    ULONG active;
    UBYTE id;
    struct PartitionType type;
    struct PartitionHandle *root, *partition;
    LONG partition_no = -1;
    ULONG tableType = 0;

    D(bug("[InstallAROS] %s()\n", __func__));

    if ((root = OpenRootPartition(device, unit)) != NULL)
    {
        if (OpenPartitionTable(root) == 0)
        {
            GetPartitionTableAttrsTags(root, PTT_TYPE, (IPTR)&tableType, TAG_DONE);
            /* Windows chainloading here supports primary MBR partitions. */
            if (tableType == PHPTT_MBR)
            {
                ForeachNode(&root->table->list, partition)
                {
                    active = 0;
                    memset(&type, 0, sizeof(type));
                    GetPartitionAttrsTags
                        (partition,
                        PT_ACTIVE, (IPTR) & active,
                        PT_TYPE, (IPTR) & type, TAG_DONE);
                    id = type.id[0];
                    if (active && (id == 0x7 || id == 0xb || id == 0xc))
                    {
                        ULONG position = 0;
                        GetPartitionAttrsTags(partition, PT_POSITION,
                            (IPTR)&position, TAG_DONE);
                        partition_no = position;
                    }
                }
            }
            ClosePartitionTable(root);
        }
        CloseRootPartition(root);
    }

    return partition_no;
}

#if defined(INSTALL_BL_EFI)

/* Match the partition type and exact on-disk extent, not just a FAT
 * filesystem: a data partition on the destination drive is not an ESP. */
#define ID_FAT_MASK     0xffffff00

extern Object *optObjDestDevice;
extern Object *optObjDestUnit;
extern Object *optObjCheckFormatESP;
extern Object *optObjCheckCopyESP;

#define EFI_MAXDEVS     64
#define EFI_DEVNAMELEN  34

#ifdef INSTALL_BL_PC_EFI
static BOOL EFI_IsSystemPartition(struct PartitionHandle *root,
    const struct DosEnvec *de)
{
    static const uuid_t esp = MAKE_UUID(0xC12A7328, 0xF81F, 0x11D2,
        0xBA4B, 0x00A0C93EC93BULL);
    struct PartitionHandle *ph;
    UQUAD first, end, cylsize = (UQUAD)de->de_Surfaces * de->de_BlocksPerTrack;

    if (!cylsize || de->de_HighCyl < de->de_LowCyl)
        return FALSE;
    first = cylsize * de->de_LowCyl;
    end = cylsize * (de->de_HighCyl + 1) - 1;
    ForeachNode(&root->table->list, ph)
    {
        struct PartitionType type = { { 0 }, 0 };
        UQUAD a = 0, b = 0;
        GetPartitionAttrsTags(ph, PT_TYPE, (IPTR)&type,
            PT_STARTBLOCK, (IPTR)&a, PT_ENDBLOCK, (IPTR)&b, TAG_DONE);
        if (root->table->type == PHPTT_GPT && type.id_len == sizeof(esp) &&
            !memcmp(type.id, &esp, sizeof(esp)) && first == a && end == b)
            return TRUE;
        if (root->table->type == PHPTT_MBR && type.id_len == 1 && type.id[0] == 0xef)
        {
            struct DosEnvec partde = { 0 };
            GetPartitionAttrsTags(ph, PT_DOSENVEC, (IPTR)&partde, TAG_DONE);
            a = (UQUAD)partde.de_Surfaces * partde.de_BlocksPerTrack;
            b = a * (partde.de_HighCyl + 1) - 1;
            a *= partde.de_LowCyl;
            if (first == a && end == b)
                return TRUE;
        }
    }
    return FALSE;
}

#endif

static BOOL EFI_FindESP(CONST_STRPTR devName, ULONG unit, STRPTR buf,
    ULONG bufLen)
{
    TEXT names[EFI_MAXDEVS][EFI_DEVNAMELEN];
    ULONG devNameLen = strlen(devName);
    ULONG count = 0, i;
    struct DosList *dl;
    BOOL found = FALSE;
#ifdef INSTALL_BL_PC_EFI
    struct PartitionHandle *root = OpenRootPartition(devName, unit);

    if (!root)
        return FALSE;
    if (OpenPartitionTable(root) != 0)
    {
        CloseRootPartition(root);
        return FALSE;
    }

#endif

    /* CON:, RAW: and friends keep something else entirely in dol_Startup,
       so each entry has to be asked whether it is a filesystem first - and
       that cannot be done while the list is locked */
    dl = LockDosList(LDF_DEVICES | LDF_READ);
    while (((dl = NextDosEntry(dl, LDF_DEVICES)) != NULL)
        && (count < EFI_MAXDEVS))
    {
        ULONG len = AROS_BSTR_strlen(dl->dol_Name);

        if ((len == 0) || (len > EFI_DEVNAMELEN - 2))
            continue;
        CopyMem(AROS_BSTR_ADDR(dl->dol_Name), names[count], len);
        names[count][len] = '\0';
        count++;
    }
    UnLockDosList(LDF_DEVICES | LDF_READ);

    for (i = 0; (i < count) && !found; i++)
    {
        struct FileSysStartupMsg *fssm;
        struct DeviceNode *dn;
        struct DosEnvec *de, geometry;
        BOOL candidate = FALSE;
        TEXT devPath[EFI_DEVNAMELEN + 1];

        sprintf(devPath, "%s:", names[i]);
        if (!IsFileSystem(devPath))
            continue;

        dl = LockDosList(LDF_DEVICES | LDF_READ);
        dn = (struct DeviceNode *)FindDosEntry(dl, names[i], LDF_DEVICES);
        if (dn != NULL)
        {
            fssm = (struct FileSysStartupMsg *)BADDR(dn->dn_Startup);
            if ((fssm != NULL) && (fssm->fssm_Device != BNULL)
                && ((de = (struct DosEnvec *)BADDR(fssm->fssm_Environ)) != NULL)
                && (AROS_BSTR_strlen(fssm->fssm_Device) == devNameLen)
                && (Strnicmp(AROS_BSTR_ADDR(fssm->fssm_Device), devName,
                        devNameLen) == 0)
                && (fssm->fssm_Unit == unit)
                && ((de->de_DosType & ID_FAT_MASK) == ID_FAT_DISK)
                && (strlen(names[i]) < bufLen))
            {
                geometry = *de;
                candidate = TRUE;
            }
        }
        UnLockDosList(LDF_DEVICES | LDF_READ);
        if (candidate
#ifdef INSTALL_BL_PC_EFI
            && EFI_IsSystemPartition(root, &geometry)
#endif
            )
        {
            strcpy(buf, names[i]);
            found = TRUE;
        }
    }

#ifdef INSTALL_BL_PC_EFI
    ClosePartitionTable(root);
    CloseRootPartition(root);
#endif
    return found;
}

BOOL BOOTLOADER_ESPInfo(STRPTR buf, ULONG bufLen, BOOL *formatted)
{
    IPTR devName = 0, unit = 0;
    TEXT espDev[66];
    BPTR lock;

    GET(optObjDestDevice, MUIA_InstallOption_Value, &devName);
    GET(optObjDestUnit, MUIA_InstallOption_Value, &unit);

    if ((devName == 0)
        || !EFI_FindESP((CONST_STRPTR)devName, (ULONG)unit, buf, bufLen))
        return FALSE;

    sprintf(espDev, "%s:", buf);
    if ((lock = Lock(espDev, SHARED_LOCK)) != BNULL)
    {
        UnLock(lock);
        *formatted = TRUE;
    }
    else
        *formatted = FALSE;

    return TRUE;
}

static BOOL EFI_FormatESP(CONST_STRPTR name, CONST_STRPTR path)
{
    struct DevProc *dp = GetDeviceProc(path, NULL);
    struct DosList *list, *device;
    struct MsgPort *port = NULL;
    LONG error = 0;
    BOOL success = FALSE;

    if (!dp)
        return FALSE;
    list = LockDosList(LDF_DEVICES | LDF_READ);
    device = FindDosEntry(list, name, LDF_DEVICES);
    if (device)
        port = device->dol_Task;
    UnLockDosList(LDF_DEVICES | LDF_READ);
    if (port && port == dp->dvp_Port)
    {
        if (DoPkt(port, ACTION_INHIBIT, DOSTRUE, 0, 0, 0, 0))
        {
            success = DoPkt(port, ACTION_FORMAT,
                (IPTR)AROS_CONST_BSTR(EFI_VOL_NAME), ID_FAT32_DISK, 0, 0, 0) != 0;
            error = IoErr();
            if (!DoPkt(port, ACTION_INHIBIT, DOSFALSE, 0, 0, 0, 0) && success)
            {
                success = FALSE;
                error = IoErr();
            }
        }
        else
            error = IoErr();
    }
    else
        error = ERROR_DEVICE_NOT_MOUNTED;
    FreeDeviceProc(dp);
    if (success)
    {
        /* The handler finishes filesystem initialization on uninhibit.
         * Its packet reply alone does not prove that initialization worked. */
        BPTR lock = Lock(path, SHARED_LOCK);
        if (lock)
            UnLock(lock);
        else
        {
            success = FALSE;
            error = IoErr();
        }
    }
    SetIoErr(error);
    return success;
}

static void EFI_DoInstall(Class * CLASS, Object * self)
{
    struct InstallStage_DATA *data = INST_DATA(CLASS, self);
    CONST_STRPTR efi_dirs[] = {
        "boot", "boot",
#ifdef INSTALL_BL_PC_EFI
        /* Some PC images contain both EFI/BOOT and efi/boot. Copy the
         * fallback loader directory once onto the case-insensitive ESP. */
        "EFI/BOOT", "EFI/BOOT",
#else
        "EFI", "EFI",
#endif
        NULL
    };
    /* The stub reads these from the root of the volume it was loaded from */
    CONST_STRPTR efi_files[] = {
        "aros.cmd",
        "startup.nsh",
        NULL
    };
    TEXT espName[64], espDev[66];
    TEXT srcFile[256], dstFile[256];
    BOOL formatted = FALSE;
    IPTR option = FALSE;
    BPTR lock;
    int i;

    if (!BOOTLOADER_ESPInfo(espName, sizeof(espName), &formatted))
    {
        LOG_Printf("ERROR: no EFI system partition on the destination drive"
            " - the installed system will not boot");
        data->inst_success = MUIV_Inst_Failed;
        return;
    }

    sprintf(espDev, "%s:", espName);
    LOG_Printf("EFI: system partition is %s (%s)", espDev,
        formatted ? "formatted" : "unformatted");

    OPTOGET(optObjCheckFormatESP, MUIA_Selected, &option);
    if (option)
    {
        LOG_Printf("EFI: formatting %s", espDev);
        sprintf(dstFile, _(MSG_FORMATTING), espDev);
        SET(data->label, MUIA_Text_Contents, (IPTR)dstFile);

        if (!EFI_FormatESP(espName, espDev))
        {
            LOG_Printf("ERROR: could not format %s [ioerr=%ld]", espDev,
                (LONG)IoErr());
            data->inst_success = MUIV_Inst_Failed;
            return;
        }
        formatted = TRUE;
    }

    OPTOGET(optObjCheckCopyESP, MUIA_Selected, &option);
    if (!option)
    {
        LOG_Printf("EFI: not copying the boot files (not requested)");
        return;
    }

    if (!formatted)
    {
        LOG_Printf("ERROR: %s holds no filesystem and was not formatted,"
            " so the boot files cannot be written to it", espDev);
        data->inst_success = MUIV_Inst_Failed;
        return;
    }

    SET(data->label, MUIA_Text_Contents, __(MSG_COPYBOOT));
    CopyDirArray(CLASS, self, source_Path, espDev, efi_dirs, NULL);

    for (i = 0; efi_files[i]
        && (data->inst_success == MUIV_Inst_InProgress); i++)
    {
        strcpy(srcFile, source_Path);
        AddPart(srcFile, efi_files[i], sizeof(srcFile));
        sprintf(dstFile, "%s%s", espDev, efi_files[i]);

        if ((lock = Lock(srcFile, SHARED_LOCK)) == BNULL)
        {
            LOG_Printf("EFI: '%s' is not on the install medium - skipped",
                srcFile);
            continue;
        }
        UnLock(lock);

        LOG_Printf("EFI: %s -> %s", srcFile, dstFile);
        DoMethod(self, MUIM_IC_CopyFile, srcFile, dstFile);
    }
}

#endif /* INSTALL_BL_EFI */

void BOOTLOADER_DoInstall(Class * CLASS, Object * self)
{
    struct InstallStage_DATA *data = INST_DATA(CLASS, self);
    ULONG srcLen =
        strlen(source_Path) + BOOTLOADER_PATH_LEN + 8;
    ULONG dstLen =
        strlen(data->install_SysTarget) + BOOTLOADER_PATH_LEN + 8;
    int numgrubfiles = 0, file_count = 0;
    TEXT srcPath[srcLen];
    TEXT dstPath[dstLen];
    TEXT tmp[256];
    IPTR option = 0;
    LONG part_no;

    /* Installing Bootloader */
    D(bug("[InstallAROS] %s()\n", __func__));
    SET(data->label, MUIA_Text_Contents, __(MSG_INSTALLINGBOOT));
    SET(data->pageheader, MUIA_Text_Contents, __(MSG_BOOTLOADER));
    SET(data->label, MUIA_Text_Contents, __(MSG_COPYBOOT));

    strcpy(srcPath, source_Path);
    AddPart(srcPath, "boot", srcLen);
    sprintf(dstPath, "%s:boot", data->install_SysTarget);

    D(
        bug("[InstallAROS] %s: Boot Device = %s/%d\n", __func__, data->bl_TargetDevice, data->bl_TargetUnit);
        bug("[InstallAROS] %s: Source Path = '%s'\n", __func__, srcPath);
        bug("[InstallAROS] %s: Dest Path = '%s'\n", __func__, dstPath);
    )

    switch (BootLoaderType)
    {
#if defined(INSTALL_BL_EFI)
    case BOOTLOADER_EFI:
        EFI_DoInstall(CLASS, self);
        if (data->inst_success == MUIV_Inst_InProgress)
            SET(data->gauge2, MUIA_Gauge_Current, 100);
        return;
#endif
#if defined(INSTALL_BL_GRUB2)
    case BOOTLOADER_GRUB2:
        {
            CONST_STRPTR grub2_dirs[] = {
                BootLoaderData[BOOTLOADER_GRUB2].path, BootLoaderData[BOOTLOADER_GRUB2].path,
                NULL
            };
            CopyDirArray(CLASS, self, srcPath, dstPath, grub2_dirs, NULL);
            AddPart(srcPath, BootLoaderData[BOOTLOADER_GRUB2].path, srcLen);
            AddPart(dstPath, BootLoaderData[BOOTLOADER_GRUB2].path, dstLen);

            /* Warning: do not modify srcPath or dstPath beyond this point */

            /* Grub 2 text/gfx mode */
            GET(data->instc_options_grub->gopt_grub2mode, MUIA_Cycle_Active,
                &option);

            /* Rename _unicode.pf2 <-> unicode.pf2 if necessary */
            {
                ULONG newDstLen =
                    strlen(dstPath) + strlen("_unicode.pf2") + 8;
                TEXT srcFile[newDstLen];
                TEXT dstFile[newDstLen];

                sprintf(srcFile, "%s/fonts", dstPath);
                sprintf(dstFile, "%s/fonts", dstPath);
                AddPart(srcFile, "_unicode.pf2", newDstLen);
                AddPart(dstFile, "unicode.pf2", newDstLen);

                if (option == 1 && !gfx_font_exists)
                    Rename(srcFile, dstFile);
                else if (option == 0 && gfx_font_exists)
                    Rename(dstFile, srcFile);
            }

            /* Add entry to boot MS Windows if present */
            if ((part_no =
                    FindWindowsPartition(data->bl_TargetDevice, data->bl_TargetUnit)) != -1
                && XGET(cycle_drivetype, MUIA_Cycle_Active) != 2)
            {
                TEXT cmd[256];
                sprintf(tmp, "%s", dstPath);
                AddPart(tmp, "grub.cfg", 256);

                /* Make sure the config file is writeable */
                sprintf(cmd, "Protect ADD FLAGS=W ALL QUIET %s",
                    tmp);
                D(bug
                        ("[InstallAROS] %s: calling '%s'\n", __func__, cmd));
                LONG success = Execute(cmd, NULL, NULL);
                if (success)
                {
                    BPTR menu_file = Open(tmp, MODE_READWRITE);

                    if (menu_file != NULL)
                    {
                        Seek(menu_file, 0, OFFSET_END);
                        FPrintf(menu_file, "\nmenuentry \"Microsoft Windows\" {\n    chainloader (hd%ld,%ld)+1\n}\n\n", 0, part_no + 1);    /* GRUB2 counts partitions from 1 */
                        Close(menu_file);
                    }
                    D(bug
                        ("[InstallAROS] %s: Windows partition found. Adding Windows option to GRUB2 menu.\n", __func__));
                }
            }

            sprintf(tmp,
                "C:Install-grub2 DEVICE \"%s\" UNIT %d GRUB \"%s\"",
                data->bl_TargetDevice, data->bl_TargetUnit, dstPath);

            break;
        }
#endif
#if defined(INSTALL_BL_GRUB)
    case BOOTLOADER_GRUB1:

        CreateDir(dstPath);

        numgrubfiles = 3;

        TEXT *grub_files[] = {
            "stage1", "stage1",
            "stage2_hdisk", "stage2",
            "menu.lst.DH0", "menu.lst",
            NULL
        };

        GRUB_COPY_FILE_LOOP(grub_files);

        /* Add entry to boot MS Windows if present */
        if ((part_no =
                FindWindowsPartition(data->bl_TargetDevice, data->bl_TargetUnit)) != -1)
        {
            sprintf(tmp, "%s", dstPath);
            AddPart(tmp, "menu.lst", 256);

            BPTR menu_file = Open(tmp, MODE_READWRITE);
            if (menu_file != NULL)
            {
                Seek(menu_file, 0, OFFSET_END);
                FPrintf(menu_file,
                    "\ntitle Microsoft Windows\nrootnoverify (hd%ld,%ld)\nchainloader +1\n",
                    0, part_no);
                Close(menu_file);
            }
            D(bug
                ("[InstallAROS] %s: Windows partition found. Adding Windows option to GRUB menu.\n", __func__));
        }

        sprintf(tmp,
            "C:Install-grub DEVICE \"%s\" UNIT %d GRUB \"%s\" FORCELBA",
            data->bl_TargetDevice, data->bl_TargetUnit, dstPath);

        break;
#endif
    default:
        /* No installation command exists for this bootloader. */
        data->inst_success = MUIV_Inst_Failed;
        return;
    }

    if (data->inst_success != MUIV_Inst_InProgress)
        return;

    D(bug("[InstallAROS] %s: executing '%s'\n", __func__, tmp));
    LONG result = SystemTagList(tmp, NULL);
    LOG_Printf("bootloader returned %ld", result);
    if (result != RETURN_OK)
    {
        data->inst_success = MUIV_Inst_Failed;
        return;
    }
    D(bug("[InstallAROS] %s: finished\n", __func__));

    SET(data->gauge2, MUIA_Gauge_Current, 100);
}
