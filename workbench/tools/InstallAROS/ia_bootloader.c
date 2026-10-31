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
#if (0)
    {
        "grub EFI",
        "SYS:",
        "EFI/BOOT/BOOTX64.EFI",
        NULL,
        NULL
    },
#endif
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
    IPTR active, id;
    struct PartitionType type;
    struct PartitionHandle *root, *partition;
    LONG partition_no = -1, i = 0;

    D(bug("[InstallAROS] %s()\n", __func__));

    if ((root = OpenRootPartition(device, unit)) != NULL)
    {
        if (OpenPartitionTable(root) == 0)
        {
            /* Look for an active partition with a Windows FS */
            ForeachNode(&root->table->list, partition)
            {
                GetPartitionAttrsTags
                    (partition,
                    PT_ACTIVE, (IPTR) & active,
                    PT_TYPE, (IPTR) & type, TAG_DONE);
                id = type.id[0];
                if (active && (id == 0x7 || id == 0xb || id == 0xc))
                    partition_no = i;
                i++;
            }
            ClosePartitionTable(root);
        }
        CloseRootPartition(root);
    }

    return partition_no;
}

#if defined(INSTALL_BL_EFI)

/* The ESP holds no AROS filesystem, so it is identified by its DOS type on
   the drive being installed to rather than by a volume name */
/* ID_FAT_DISK, ID_FAT32_DISK and friends come from dos/filesystemids.h;
   FAT12, FAT16 and FAT32 differ only in the last byte */
#define ID_FAT_MASK     0xffffff00

extern Object *optObjDestDevice;
extern Object *optObjDestUnit;
extern Object *optObjCheckFormatESP;
extern Object *optObjCheckCopyESP;

extern BOOL FormatPartition(CONST_STRPTR device, CONST_STRPTR name,
    ULONG dostype);

#define EFI_MAXDEVS     64
#define EFI_DEVNAMELEN  34

static BOOL EFI_FindESP(CONST_STRPTR devName, ULONG unit, STRPTR buf,
    ULONG bufLen)
{
    TEXT names[EFI_MAXDEVS][EFI_DEVNAMELEN];
    ULONG devNameLen = strlen(devName);
    ULONG count = 0, i;
    struct DosList *dl;
    BOOL found = FALSE;

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
        struct DosEnvec *de;
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
                strcpy(buf, names[i]);
                found = TRUE;
            }
        }
        UnLockDosList(LDF_DEVICES | LDF_READ);
    }

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

static void EFI_DoInstall(Class * CLASS, Object * self)
{
    struct InstallStage_DATA *data = INST_DATA(CLASS, self);
    CONST_STRPTR efi_dirs[] = {
        "boot", "boot",
        "EFI", "EFI",
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
        SET(data->label, MUIA_Text_Contents, __(MSG_FORMATTING));

        if (!FormatPartition(espDev, EFI_VOL_NAME, ID_FAT32_DISK))
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
        break;
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
        /* TODO: support more bootloaders */
        break;
    }

    D(bug("[InstallAROS] %s: executing '%s'\n", __func__, tmp));
    Execute(tmp, NULL, NULL);
    D(bug("[InstallAROS] %s: finished\n", __func__));

    SET(data->gauge2, MUIA_Gauge_Current, 100);
}
