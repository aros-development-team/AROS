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

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_bootloader.h"

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
        ARCHBOOTDIR "/grub/stage1",
        "c/Install-grub",
        NULL
    },
#endif
#if defined(INSTALL_BL_GRUB2)
    {
        "grub",
        ARCHBOOTDIR "/grub/" GRUBARCHDIR "/core.img",
        "c/Install-grub2",
        ARCHBOOTDIR "/grub/fonts/unicode.pf2"
    },
#endif
    {
        -1,
        -1,
        -1,
        -1
    }
};

#define BOOTLOADER_PATH_LEN 64  /* Must be large enough to contain any of the bootloader match strings */

BOOL _matchPath(char **match)
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

    for (i = 0; ( BootLoaderData[i].match != -1); i++)
    {
        D(bug("[InstallAROS] %s: #%d = '%s'\n", __func__, i, BootLoaderData[i].path));

        if (BootLoaderData[i].match && _matchPath(&BootLoaderData[i].match))
        {
            D(bug("[InstallAROS] %s: bootloader files found\n", __func__));
            if (BootLoaderData[i].match2 && _matchPath(&BootLoaderData[i].match2))
            {
                D(bug("[InstallAROS] %s: bootloader installer found\n", __func__));
#if defined(INSTALL_BL_GRUB)
# if defined(GRUB) && (GRUB == 1)
                if (i == BOOTLOADER_GRUB)
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

#if defined(INSTALL_BL_GRUB)
    strcpy(skipPath, source_Path);
    AddPart(skipPath, ARCHBOOTDIR "/grub", skipPathLen);
    AddSkipListEntry(SkipList, skipPath);
#endif
#if defined(INSTALL_BL_GRUB2)
    strcpy(skipPath, source_Path);
    AddPart(skipPath, ARCHBOOTDIR "/grub2", skipPathLen);
    AddSkipListEntry(SkipList, skipPath);
#endif
}

BOOL BOOTLOADER_PartFixUp(struct Install_DATA *data, IPTR systype)
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

void BOOTLOADER_DoInstall(Class * CLASS, Object * self)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    ULONG srcLen =
        strlen(source_Path) + strlen(ARCHBOOTDIR) + BOOTLOADER_PATH_LEN + 4;
    ULONG dstLen =
        strlen(data->install_SysTarget) + strlen(ARCHBOOTDIR) + BOOTLOADER_PATH_LEN + 4;
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
    AddPart(srcPath, ARCHBOOTDIR, srcLen);
    sprintf(dstPath, "%s:%s", data->install_SysTarget, ARCHBOOTDIR);

    data->bl_TargetUnit = XGET(data->bl_TargetUnit, MUIA_String_Integer);

    D(
        bug("[InstallAROS] %s: Boot Device = %s/%d\n", __func__, data->bl_TargetDevice, data->bl_TargetUnit);
        bug("[InstallAROS] %s: Source Path = '%s'\n", __func__, srcPath);
        bug("[InstallAROS] %s: Dest Path = '%s'\n", __func__, dstPath);
    )

    switch (BootLoaderType)
    {
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
                BOOL success = (BOOL) Execute(cmd, NULL, NULL);
                if (!success)
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
