/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#define DEBUG 0
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

#include "install.h"



#define kBufSize          (4*65536)
#define kExallBufSize          (4096)

#define SYS_PART_NAME         "DH0"
#define WORK_PART_NAME        "DH1"
#define USB_SYS_PART_NAME     "DU0"
#define USB_WORK_PART_NAME    "DU1"
#define SYS_VOL_NAME          "AROS"
#define WORK_VOL_NAME         "Work"
#define USB_SYS_VOL_NAME      "AROS Live Drive"
#define USB_WORK_VOL_NAME     "Briefcase"

#define MAX_FFS_SIZE (4L * 1024)
#define MAX_SFS_SIZE (124L * 1024)

#define INSTALLER_TMP_PATH      "T:Installer"
#define INSTALLAROS_TMP_PATH    "T:Installer/InstallAROS"

#define localeFile_path         "Prefs/Locale\""
#define inputFile_path          "Prefs/Input\""
#define prefssrc_path           "ENV:SYS"
#define prefs_path              "Prefs/Env-Archive/SYS"
#define BOOT_PATH               "boot"
#define GRUB_PATH               "grub"

#define locale_prfs_file        "locale.prefs"  /* please note the suffixed \" */
#define input_prfs_file        "input.prefs"

#define DEF_INSTALL_IMAGE       "IMAGES:Logos/install.logo"
#define DEF_BACK_IMAGE          "IMAGES:Logos/install.logo"
#define DEF_LIGHTBACK_IMAGE     "IMAGES:Logos/install.logo"

#define POST_INSTALL_SCRIPT     "PROGDIR:InstallAROS-Post-Install"
#define AROS_BOOT_FILE          "AROS.boot"

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

#define OPTION_PREPDRIVES           1
#define OPTION_FORMAT               2
#define OPTION_LANGUAGE             3
#define OPTION_CORE                 4
#define OPTION_EXTRAS               5
#define OPTION_BOOTLOADER           6

#define INSTV_TITLE                 101001
#define INSTV_LOGO                  101002
#define INSTV_PAGE                  101003

#define INSTV_TEXT                  101004
#define INSTV_SPACE                 101005
#define INSTV_BOOL                  101006
#define INSTV_RETURN                101007

#define INSTV_CURR                  101100

enum BootLoaderTypes
{
    BOOTLOADER_NONE = -1,
    BOOTLOADER_GRUB1,
    BOOTLOADER_GRUB2
};

/* Files to check for. Must be in the same order as enum values before */
STRPTR BootLoaderFiles[] = {
    "boot/grub/stage1",
    "boot/grub/core.img",
    NULL
};

#define BOOTLOADER_PATH_LEN 20  /* Length of the largest string in BootLoaders array plus 2 */

struct ExpansionBase *ExpansionBase = NULL;

char *source_Path = NULL;       /* full path to source "tree" */
char *extras_source = NULL;

char *dest_Path = NULL;         /* DOS DEVICE NAME of part used to store "aros" */
char *work_Path = NULL;         /* DOS DEVICE NAME of part used to store "work" */
TEXT *extras_path = NULL;       /* DOS DEVICE NAME of part used to store extras */

char *boot_Device = "ata.device";
ULONG boot_Unit = 0;

Object *check_copytowork = NULL;
Object *check_work = NULL;
Object *show_formatsys = NULL;
Object *show_formatwork = NULL;
Object *check_formatsys = NULL;
Object *check_formatwork = NULL;
Object *cycle_fstypesys = NULL;
Object *cycle_fstypework = NULL;
Object *cycle_sysunits = NULL;
Object *cycle_workunits = NULL;

Object *dest_volume = NULL;
Object *work_volume = NULL;

Object *dest_device = NULL;
Object *cycle_drivetype = NULL;
Object *dest_unit = NULL;
Object *show_sizesys = NULL;
Object *show_sizework = NULL;
Object *check_sizesys = NULL;
Object *check_sizework = NULL;
Object *check_creatework = NULL;
Object *sys_size = NULL;
Object *work_size = NULL;
Object *sys_devname = NULL;
Object *work_devname = NULL;

Object *grub_device = NULL;
Object *grub_unit = NULL;

Object *reboot_group = NULL;
LONG BootLoaderType;

ULONG GuessFirstHD(CONST_STRPTR device);
static struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path);
static LONG GetPartitionSize(BOOL get_work);
static LONG FindWindowsPartition(STRPTR device, LONG unit);
LONG CopyDirArray(Class * CLASS, Object * self, CONST_STRPTR sourcePath,
    CONST_STRPTR destinationPath, CONST_STRPTR directories[]);

IPTR Install__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);

    struct Install_DATA *data = INST_DATA(CLASS, self);
    BPTR lock = NULL;
    char sys_path[100];

    /* We will generate this info shortly */

    /* IO Related */

    data->IO_Always_overwrite = IIO_Overwrite_Ask;

    /* Main stuff */

    data->welcomeMsg =
        (APTR) GetTagData(MUIA_WelcomeMsg, (IPTR) NULL,
        message->ops_AttrList);
    data->doneMsg =
        (APTR) GetTagData(MUIA_FinishedMsg, (IPTR) NULL,
        message->ops_AttrList);

    data->page =
        (APTR) GetTagData(MUIA_Page, (IPTR) NULL, message->ops_AttrList);
    data->gauge1 =
        (APTR) GetTagData(MUIA_Gauge1, (IPTR) NULL, message->ops_AttrList);
    data->gauge2 =
        (APTR) GetTagData(MUIA_Gauge2, (IPTR) NULL, message->ops_AttrList);
    data->label =
        (APTR) GetTagData(MUIA_Install, (IPTR) NULL, message->ops_AttrList);

    data->installer =
        (APTR) GetTagData(MUIA_OBJ_Installer, (IPTR) NULL,
        message->ops_AttrList);

    data->window =
        (APTR) GetTagData(MUIA_OBJ_Window, (IPTR) NULL,
        message->ops_AttrList);
    data->contents =
        (APTR) GetTagData(MUIA_OBJ_WindowContent, (IPTR) NULL,
        message->ops_AttrList);

    data->pagetitle =
        (APTR) GetTagData(MUIA_OBJ_PageTitle, (IPTR) NULL,
        message->ops_AttrList);
    data->pageheader =
        (APTR) GetTagData(MUIA_OBJ_PageHeader, (IPTR) NULL,
        message->ops_AttrList);

    data->actioncurrent =
        (APTR) GetTagData(MUIA_OBJ_CActionStrng, (IPTR) NULL,
        message->ops_AttrList);
    data->back =
        (APTR) GetTagData(MUIA_OBJ_Back, (IPTR) NULL,
        message->ops_AttrList);
    data->proceed =
        (APTR) GetTagData(MUIA_OBJ_Proceed, (IPTR) NULL,
        message->ops_AttrList);
    data->cancel =
        (APTR) GetTagData(MUIA_OBJ_Cancel, (IPTR) NULL,
        message->ops_AttrList);

    data->instc_lic_file =
        (char *)GetTagData(MUIA_IC_License_File, (IPTR) NULL,
        message->ops_AttrList);
    data->instc_copt_licensemandatory =
        (BOOL) GetTagData(MUIA_IC_License_Mandatory, (IPTR) FALSE,
        message->ops_AttrList);

    data->instc_options_main =
        (APTR) GetTagData(MUIA_List_Options, (IPTR) NULL,
        message->ops_AttrList);
    data->instc_options_grub =
        (APTR) GetTagData(MUIA_Grub_Options, (IPTR) NULL,
        message->ops_AttrList);

    data->instc_copt_undoenabled =
        (BOOL) GetTagData(MUIA_IC_EnableUndo, (IPTR) FALSE,
        message->ops_AttrList);

    data->instc_options_main->partitioned = FALSE;
    data->instc_options_main->bootloaded = FALSE;
    data->instc_options_grub->bootinfo = FALSE;

    GET(data->window, MUIA_Window_Width, &data->cur_width);
    GET(data->window, MUIA_Window_Height, &data->cur_height);

    SET(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
    SET(data->back, MUIA_Disabled, TRUE);

    data->instc_stage_next = EPartitionOptionsStage;

    data->inst_success = FALSE;
    data->disable_back = FALSE;

    data->instc_cflag_driveset = (BOOL) DoMethod(self, MUIM_FindDrives);

    /* Default to USB if a USB system volume appears to be present and we
     * haven't booted from it */

    lock = Lock("SYS:", SHARED_LOCK);
    NameFromLock(lock, sys_path, 100);
    if (getDiskFSSM(USB_SYS_PART_NAME ":") != NULL
        && strncmp(sys_path, USB_SYS_VOL_NAME ":",
        strlen(USB_SYS_VOL_NAME) + 1))
    {
        SET(dest_volume, MUIA_String_Contents, USB_SYS_PART_NAME);
        SET(work_volume, MUIA_String_Contents, USB_WORK_PART_NAME);

        SET(cycle_drivetype, MUIA_Cycle_Active, 1);
    }
    UnLock(lock);

    boot_Unit = GuessFirstHD(boot_Device);

    DoMethod(data->proceed, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self,
        1, MUIM_IC_NextStep);
    DoMethod(data->back, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1,
        MUIM_IC_PrevStep);
    DoMethod(data->cancel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1,
        MUIM_IC_CancelInstall);

    DoMethod(self, MUIM_Notify, MUIA_InstallComplete, TRUE, (IPTR) self, 1,
        MUIM_Reboot);

    /* set up the license info */

    if (data->instc_lic_file)
    {
        register struct FileInfoBlock *fib = NULL;
        BPTR from = NULL;
        LONG s = 0;

        lock = (BPTR) Lock(data->instc_lic_file, SHARED_LOCK);
        if (lock != NULL)
        {
            fib = (void *)AllocVec(sizeof(*fib), MEMF_PUBLIC);
            Examine(lock, fib);
        }

        if ((from = Open(data->instc_lic_file, MODE_OLDFILE)))
        {
            D(bug
                ("[INSTALLER.i] Allocating buffer [%d] for license file '%s'!",
                    fib->fib_Size, data->instc_lic_file));
            data->instc_lic_buffer =
                AllocVec(fib->fib_Size + 1, MEMF_CLEAR | MEMF_PUBLIC);
            if ((s = Read(from, data->instc_lic_buffer,
                        fib->fib_Size)) == -1)
            {
                D(bug("[INSTALLER.i] Error processing license file!"));
                if ((BOOL) data->instc_copt_licensemandatory)
                {
                    Close(from);
                    UnLock(lock);
                    return 0;
                }
            }
            else
            {
                DoMethod(data->instc_options_main->opt_lic_box,
                    MUIM_TextEditor_InsertText, data->instc_lic_buffer,
                    MUIV_TextEditor_InsertText_Top);
            }
            Close(from);
        }

        if (lock != NULL)
        {
            if (fib)
                FreeVec(fib);
            UnLock(lock);
        }

        if (!data->instc_copt_licensemandatory)
            SET(data->instc_options_main->opt_lic_mgrp, MUIA_ShowMe, FALSE);
        else
            DoMethod(data->instc_options_main->opt_license, MUIM_Notify,
                MUIA_Selected, MUIV_EveryTime, (IPTR) data->proceed, 3,
                MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
    }

    /* UNDO Record */

    if (data->instc_copt_undoenabled)
    {
        lock = 0;
        NEWLIST((struct List *)&data->instc_undorecord);
        D(bug("[INSTALLER.i] Prepared UNDO list @ %p\n",
                &data->instc_undorecord));

        if ((lock = Lock(INSTALLER_TMP_PATH, ACCESS_READ)) != NULL)
        {
            D(bug("[INSTALLER.i] Dir '%s' Exists - no nead to create\n",
                    INSTALLER_TMP_PATH));
            UnLock(lock);
        }
        else
        {
            lock = CreateDir(INSTALLER_TMP_PATH);
            if (lock != NULL)
                UnLock(lock);
            else
            {
                D(bug("[INSTALLER.i] Failed to create dir '%s'!!\n",
                        INSTALLER_TMP_PATH));
                data->inst_success = MUIV_Inst_Failed;
                return 0;
            }
        }

        if ((lock = Lock(INSTALLAROS_TMP_PATH, ACCESS_READ)) != NULL)
        {
            D(bug("[INSTALLER.i] Dir '%s' Exists - no nead to create\n",
                    INSTALLAROS_TMP_PATH));
            UnLock(lock);
        }
        else
        {
            lock = CreateDir(INSTALLAROS_TMP_PATH);
            if (lock != NULL)
                UnLock(lock);
            else
            {
                D(bug("[INSTALLER.i] Failed to create dir '%s'!!\n",
                        INSTALLAROS_TMP_PATH));
                data->inst_success = MUIV_Inst_Failed;
                return 0;
            }
        }
    }

    return (IPTR) self;
}

/* make page */

ULONG AskRetry(Class * CLASS, Object * self, CONST_STRPTR message,
    CONST_STRPTR file, CONST_STRPTR opt1, CONST_STRPTR opt2,
    CONST_STRPTR opt3)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    STRPTR finalmessage = NULL;
    STRPTR finaloptions = NULL;
    ULONG result = -1;

    finalmessage =
        AllocVec(strlen(message) + strlen(file) + 2,
        MEMF_CLEAR | MEMF_PUBLIC);
    finaloptions =
        AllocVec(strlen(opt1) + strlen(opt2) + strlen(opt3) + 5,
        MEMF_CLEAR | MEMF_PUBLIC);

    sprintf(finalmessage, message, file);
    sprintf(finaloptions, "%s|%s|%s", opt1, opt2, opt3);

    result =
        MUI_RequestA(data->installer, data->window, 0,
        "IO Error has occured", finaloptions, finalmessage, NULL);
    FreeVec(finalmessage);
    FreeVec(finaloptions);

    return result - 1;
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

/* Returns the first AROS-supported filesystem's name */
char *FindPartition(struct PartitionHandle *root)
{
    struct PartitionHandle *partition = NULL;
    char *success = NULL;
    char *name = NULL;
    struct PartitionType *type = NULL;

    ForeachNode(&root->table->list, partition)
    {
        D(bug("[INSTALLER.fp] checking part\n"));
        if (OpenPartitionTable(partition) == 0)
        {
            D(bug("[INSTALLER.fp] checking Child Parts... \n"));
            success = FindPartition(partition);
            ClosePartitionTable(partition);
            D(bug("[INSTALLER.fp] Children Done...\n"));
            if (success != NULL)
            {
                D(bug("[INSTALLER.fp] Found '%s'\n", success));
                break;
            }
        }
        else
        {
            D(bug("[INSTALLER.fp] checking PARTITION\n"));
            struct PartitionType pttype;

            name = AllocVec(100, MEMF_CLEAR | MEMF_PUBLIC);

            GetPartitionAttrsTags
                (partition,
                PT_NAME, (IPTR) name, PT_TYPE, (IPTR) & pttype, TAG_DONE);

            type = &pttype;

            if (type->id_len == 4)
            {
                D(bug("[INSTALLER.fp] Found RDB Partition!\n"));
                if ((type->id[0] == 68) && (type->id[1] == 79)
                    && (type->id[2] == 83))
                {
                    D(bug("[INSTALLER.fp] Found AFFS Partition! '%s'\n",
                            name));
                    success = name;
                    break;
                }
                if ((type->id[0] == 83) && (type->id[1] == 70)
                    && (type->id[2] == 83))
                {
                    D(bug("[INSTALLER.fp] Found SFS Partition! '%s'\n",
                            name));
                    success = name;
                    break;
                }
            }
        }
    }

    if ((!success) && (name))
        FreeVec(name);

    return success;
}

IPTR Install__MUIM_FindDrives(Class * CLASS, Object * self, Msg message)
{
    struct BootNode *CurBootNode = NULL;
    struct PartitionHandle *root;

    struct DevInfo *devnode = NULL;
    struct FileSysStartupMsg *StartMess = NULL;
    struct DosEnvec *DriveEnv = NULL;

    char *result = NULL;
    BOOL founddisk = FALSE;

    ForeachNode(&ExpansionBase->MountList, CurBootNode)
    {
        devnode = CurBootNode->bn_DeviceNode;
        StartMess = devnode->dvi_Startup;
        if (!StartMess)
            continue;
        D(bug("[INSTALLER.fd] Drive found [%s unit %d]\n",
                StartMess->fssm_Device, StartMess->fssm_Unit));

        DriveEnv = StartMess->fssm_Environ;

        if (!myCheckFloppy(DriveEnv))
        {
            if ((root =
                    OpenRootPartition(StartMess->fssm_Device,
                        StartMess->fssm_Unit)) != NULL)
            {
                if (!founddisk)
                {
                    /* First drive in system - save its info for grub */
                    D(bug
                        ("[INSTALLER.fd] First DRIVE found [%s unit %d]...\n",
                            StartMess->fssm_Device, StartMess->fssm_Unit));
                    founddisk = TRUE;
                    //                    boot_Device = StartMess->fssm_Device;
                    //                    boot_Unit = StartMess->fssm_Unit;
                }

                if (OpenPartitionTable(root) == 0)
                {
                    result = FindPartition(root);
                    D(bug("[INSTALLER.fd] Partition '%s'\n", result));
                    ClosePartitionTable(root);
                }
                CloseRootPartition(root);
            }
        }
    }

    return (IPTR) result;
}

static struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path)
{
    struct DosList *dl;
    struct DeviceNode *dn;
    TEXT dname[32];
    UBYTE i;

    D(bug("[install] getDiskFSSM('%s')\n", path));

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
                    printf("device '%s' doesn't contain a file system\n",
                        dname);
            }
            //            else
            //                PrintFault(ERROR_OBJECT_NOT_FOUND, dname);
        }
    }
    else
        printf("'%s' doesn't contain a device name\n", path);
    return NULL;
}

void w2strcpy(STRPTR name, UWORD * wstr, ULONG len)
{
    while (len)
    {
        *((UWORD *) name) = AROS_BE2WORD(*wstr);
        name += sizeof(UWORD);
        len -= 2;
        wstr++;
    }

    name -= 2;

    while ((*name == 0) || (*name == ' '))
        *name-- = 0;
}

void identify(struct IOStdReq *ioreq, STRPTR name)
{
    struct SCSICmd scsicmd;
    UWORD data[256];
    UBYTE cmd = 0xEC;           /* identify */

    scsicmd.scsi_Data = data;
    scsicmd.scsi_Length = 512;
    scsicmd.scsi_Command = &cmd;
    scsicmd.scsi_CmdLength = 1;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = &scsicmd;
    ioreq->io_Length = sizeof(struct SCSICmd);
    if (DoIO((struct IORequest *)ioreq))
        return;

    w2strcpy(name, &data[27], 40);
}

IPTR Install__MUIM_IC_NextStep(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR this_page = 0, next_stage = 0, option = 0;

    GET(data->page, MUIA_Group_ActivePage, &this_page);

    if ((EDoneStage == this_page) && (this_page == data->instc_stage_next))
        set(self, MUIA_InstallComplete, TRUE);  //ALL DONE!!

    SET(data->back, MUIA_Disabled, (BOOL) data->disable_back);

    next_stage = data->instc_stage_next;
    data->instc_stage_prev = this_page;

    SET(data->back, MUIA_Selected, FALSE);
    SET(data->proceed, MUIA_Selected, FALSE);
    SET(data->cancel, MUIA_Selected, FALSE);

    switch (data->instc_stage_next)
    {

    case ELicenseStage:
        if (data->instc_lic_file)
        {
            if (data->instc_copt_licensemandatory)
            {
                /* Force acceptance of the license */
                SET(data->instc_options_main->opt_license, MUIA_Selected,
                    FALSE);
                SET(data->proceed, MUIA_Disabled, TRUE);
            }
            data->instc_stage_next = EInstallOptionsStage;
            next_stage = ELicenseStage;
            break;
        }
        /* if no license we ignore this step... and go to partition options */

    case EPartitionOptionsStage:
        if (data->instc_cflag_driveset)
            SET(data->instc_options_main->opt_partmethod, MUIA_Radio_Active,
                2);
        else
            SET(dest_unit, MUIA_String_Integer, boot_Unit);
        data->instc_stage_next = EPartitioningStage;
        next_stage = EPartitionOptionsStage;
        break;

    case EInstallOptionsStage:
        SET(data->welcomeMsg, MUIA_Text_Contents, KMsgInstallOptions);
        data->instc_stage_next = EDestOptionsStage;
        next_stage = EInstallOptionsStage;
        break;

    case EDestOptionsStage:
        if ((BOOL) XGET(data->instc_options_main->opt_format,
                MUIA_Selected))
        {
            SET(show_formatsys, MUIA_ShowMe, TRUE);
            SET(show_formatwork, MUIA_ShowMe, TRUE);
        }
        else
        {
            SET(check_formatsys, MUIA_Selected, FALSE);
            SET(check_formatwork, MUIA_Selected, FALSE);
            SET(show_formatsys, MUIA_ShowMe, FALSE);
            SET(show_formatwork, MUIA_ShowMe, FALSE);
        }
        data->instc_stage_next = EInstallMessageStage;
        next_stage = EDestOptionsStage;
        break;

    case EInstallMessageStage:
        /* PARTITION DRIVES */

        /* have we already done this? */
        if (!data->instc_options_main->partitioned)
        {
            data->instc_options_main->partitioned = TRUE;
            data->instc_stage_next = EPartitioningStage;
            next_stage = EPartitionOptionsStage;
            data->instc_stage_prev = this_page;
            break;
        }

        /* BOOTLOADER */

        option = 0;

        GET(data->instc_options_main->opt_bootloader, MUIA_Selected,
            &option);
        if (option != 0)
        {
            //have we already done this?
            if (!data->instc_options_main->bootloaded)
            {
                data->instc_options_main->bootloaded = TRUE;

                if (!data->instc_options_grub->bootinfo)
                {
#if 0
                    char *tmp_drive = NULL;
                    char *tmp_device = NULL;
#endif
                    char *tmp_grub = NULL;
#if 0
                    struct IOStdReq *ioreq = NULL;
                    struct MsgPort *mp = NULL;
#endif
                    struct FileSysStartupMsg *fssm;

                    data->instc_options_grub->bootinfo = TRUE;

#if 0
                    tmp_drive = AllocVec(100, MEMF_CLEAR | MEMF_PUBLIC);
                    tmp_device = AllocVec(100, MEMF_CLEAR | MEMF_PUBLIC);
#endif
                    tmp_grub = AllocVec(100, MEMF_CLEAR | MEMF_PUBLIC);

                    GET(dest_volume, MUIA_String_Contents, &option);
                    sprintf(tmp_grub, "%s:boot/grub",
                        (CONST_STRPTR) option);

                    /* Guess the best disk to install GRUB's bootblock to */
                    fssm = getDiskFSSM(tmp_grub);
                    if (fssm != NULL)
                    {
                        boot_Device = fssm->fssm_Device;
                        if (strcmp(fssm->fssm_Device, "ata.device") != 0)
                            boot_Unit = fssm->fssm_Unit;
                    }
                    else
                        boot_Device = "";

                    SET(grub_device, MUIA_String_Contents,
                        (IPTR) boot_Device);
                    SET(grub_unit, MUIA_String_Integer, boot_Unit);

#if 0
                    mp = CreateMsgPort();
                    if (mp)
                    {
                        ioreq =
                            (struct IOStdReq *)CreateIORequest(mp,
                            sizeof(struct IOStdReq));
                        if (ioreq)
                        {
                            if (OpenDevice(boot_Device, boot_Unit,
                                    (struct IORequest *)ioreq, 0) == 0)
                            {
                                identify(ioreq, tmp_drive);
                                sprintf(tmp_device, "%s [%s unit %d]",
                                    tmp_drive, boot_Device, boot_Unit);
                                CloseDevice((struct IORequest *)ioreq);
                            }
                            else
                                sprintf(tmp_device,
                                    "Unknown Drive [%s unit %d]",
                                    boot_Device, boot_Unit);
                            DeleteIORequest((struct IORequest *)ioreq);
                        }
                        else
                            sprintf(tmp_device,
                                "Unknown Drive [%s unit %d]", boot_Device,
                                boot_Unit);
                        DeleteMsgPort(mp);
                    }
                    else
                        sprintf(tmp_device, "Unknown Drive [%s unit %d]",
                            boot_Device, boot_Unit);

                    SET(data->instc_options_grub->gopt_drive,
                        MUIA_Text_Contents, tmp_device);
#endif
                    SET(data->instc_options_grub->gopt_grub,
                        MUIA_Text_Contents, tmp_grub);
                }

                data->instc_stage_next = EInstallMessageStage;
                next_stage = EGrubOptionsStage;
                data->instc_stage_prev = EInstallOptionsStage;
                break;
            }
            else if (strlen((STRPTR) XGET(grub_device,
                        MUIA_String_Contents)) == 0)
            {
                /* Go back if user hasn't entered a device name for GRUB */
                MUI_RequestA(data->installer, data->window, 0, "Error",
                    "OK", KMsgNoGrubDevice, NULL);
                data->instc_stage_next = EInstallMessageStage;
                next_stage = EGrubOptionsStage;
                data->instc_stage_prev = EInstallOptionsStage;
                return 0;
            }
        }

        if (XGET(check_formatsys, MUIA_Selected)
            || XGET(check_formatwork, MUIA_Selected))
            SET(data->welcomeMsg, MUIA_Text_Contents,
                KMsgBeginWithPartition);
        else
            SET(data->welcomeMsg, MUIA_Text_Contents,
                KMsgBeginWithoutPartition);
        data->instc_stage_next = EInstallStage;
        next_stage = EMessageStage;
        break;

    case EPartitioningStage:
        get(data->instc_options_main->opt_partmethod, MUIA_Radio_Active,
            &option);
        if ((int)option == 0 || (int)option == 1)
        {
            LONG syssize, worksize;
            IPTR systype = 0, worktype = 0;

            /* Let user try again if either partition size is too big.
               Note that C:Partition will ensure that automatically sized
               partitions are within size limits */
            syssize = GetPartitionSize(FALSE);
            worksize = GetPartitionSize(TRUE);

            get(cycle_fstypesys, MUIA_Cycle_Active, &systype);
            get(cycle_fstypework, MUIA_Cycle_Active, &worktype);

            if (syssize > (systype ? MAX_SFS_SIZE : MAX_FFS_SIZE) ||
                worksize > (worktype ? MAX_SFS_SIZE : MAX_FFS_SIZE))
            {
                TEXT msg[sizeof(KMsgPartitionTooBig) + 40];
                sprintf(msg, KMsgPartitionTooBig,
                    MAX_SFS_SIZE / 1024, MAX_SFS_SIZE,
                    MAX_FFS_SIZE / 1024, MAX_FFS_SIZE);
                MUI_RequestA(data->installer, data->window, 0, "Error",
                    "OK", msg, NULL);
                return 0;
            }

            /* Warn user about using non FFS-Intl filesystem for system
               partition with GRUB */
            if ((BootLoaderType == BOOTLOADER_GRUB1) && (systype != 0))
            {
                if (MUI_RequestA(data->installer, data->window, 0,
                        "Warning",
                        "Continue Partitioning|*Cancel Partitioning",
                        KMsgGRUBNonFFSWarning, NULL) != 1)
                    return 0;
            }
        }
        data->disable_back = TRUE;

        SET(data->page, MUIA_Group_ActivePage, EPartitioningStage);

        switch (option)
        {
        case 0:
        case 1:
            if (DoMethod(self, MUIM_Partition) != RETURN_OK)
            {
                D(bug("[INSTALLER] Partitioning FAILED!!!!\n"));
                data->disable_back = FALSE;
                SET(data->page, MUIA_Group_ActivePage,
                    EInstallMessageStage);
                data->instc_stage_next = EPartitioningStage;
                data->instc_options_main->partitioned = FALSE;
                MUI_RequestA(data->installer, data->window, 0, "Error",
                    "Quit", KMsgPartitioningFailed, NULL);
                DoMethod(self, MUIM_IC_QuitInstall);
                return 0;
            }
            data->instc_options_main->partitioned = TRUE;
            next_stage = EDoneStage;
            DoMethod(data->page, MUIM_Group_InitChange);
            if (XGET(cycle_drivetype, MUIA_Cycle_Active) == 0)
                SET(data->doneMsg, MUIA_Text_Contents, KMsgDoneReboot);
            else
                SET(data->doneMsg, MUIA_Text_Contents, KMsgDoneUSB);
            SET(reboot_group, MUIA_ShowMe, TRUE);
            if (XGET(cycle_drivetype, MUIA_Cycle_Active) == 0)
                SET(data->instc_options_main->opt_reboot, MUIA_Selected,
                    TRUE);
            DoMethod(data->page, MUIM_Group_ExitChange);
            SET(data->back, MUIA_Disabled, TRUE);
            SET(data->cancel, MUIA_Disabled, TRUE);
            data->instc_stage_next = EDoneStage;
            break;
        case 2:
            data->disable_back = FALSE;
            data->instc_options_main->partitioned = TRUE;
            data->instc_stage_next = EDestOptionsStage;
            next_stage = EInstallOptionsStage;
            break;
        default:
            D(bug("[INSTALLER] Launching QuickPart...\n"));
            Execute("SYS:Tools/QuickPart", NULL, NULL);
            break;
        }
        break;

    case EInstallStage:
        data->disable_back = TRUE;
        SET(data->page, MUIA_Group_ActivePage, EInstallStage);

        DoMethod(self, MUIM_IC_Install);

        next_stage = EDoneStage;
        SET(data->back, MUIA_Disabled, TRUE);
        SET(data->cancel, MUIA_Disabled, TRUE);
        data->instc_stage_next = EDoneStage;
        break;

    default:
        break;
    }

    SET(data->page, MUIA_Group_ActivePage, next_stage);
    return 0;
}

IPTR Install__MUIM_IC_PrevStep(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR this_page = 0;

    GET(data->page, MUIA_Group_ActivePage, &this_page);
    SET(data->back, MUIA_Selected, FALSE);
    SET(data->proceed, MUIA_Selected, FALSE);
    SET(data->cancel, MUIA_Selected, FALSE);

    SET(data->back, MUIA_Disabled, (BOOL) data->disable_back);
    data->instc_stage_next = this_page;

    switch (this_page)
    {
    case EMessageStage:
        /* BACK should only be possible when page != first_page */
        if (data->instc_stage_prev != EMessageStage)
        {
            SET(data->welcomeMsg, MUIA_Text_Contents,
                KMsgBeginWithPartition);
            if (data->instc_stage_prev == EDestOptionsStage)
            {
                SET(data->page, MUIA_Group_ActivePage, EDestOptionsStage);

                data->instc_stage_prev = EInstallOptionsStage;
            }
            else
            {
                if (!data->instc_options_grub->bootinfo)
                {
                    SET(data->page, MUIA_Group_ActivePage,
                        EPartitionOptionsStage);
                }
                else
                {
                    SET(data->page, MUIA_Group_ActivePage,
                        EGrubOptionsStage);
                }
                data->instc_stage_prev = EDestOptionsStage;
            }
            data->instc_stage_next = EInstallMessageStage;
        }
        break;

    case EPartitionOptionsStage:
        if (data->instc_lic_file)
        {
            SET(data->instc_options_main->opt_license, MUIA_Selected,
                FALSE);
            SET(data->proceed, MUIA_Disabled, TRUE);
            SET(data->page, MUIA_Group_ActivePage, EPartitionOptionsStage);
            data->instc_stage_prev = ELicenseStage;
            break;
        }

    case ELicenseStage:
        SET(data->proceed, MUIA_Disabled, FALSE);
        SET(data->back, MUIA_Disabled, TRUE);
        SET(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
        SET(data->page, MUIA_Group_ActivePage, EMessageStage);
        data->instc_stage_prev = EMessageStage;
        break;

    case EInstallOptionsStage:
        SET(data->instc_options_main->opt_license, MUIA_Selected, FALSE);
        SET(data->page, MUIA_Group_ActivePage, EPartitionOptionsStage);
        data->instc_stage_prev = ELicenseStage;
        data->instc_stage_next = EPartitioningStage;
        break;

    case EDestOptionsStage:
        SET(data->page, MUIA_Group_ActivePage, EInstallOptionsStage);
        data->instc_stage_next = EDestOptionsStage;
        data->instc_stage_prev = EMessageStage;
        break;

    case EGrubOptionsStage:
        SET(data->page, MUIA_Group_ActivePage, EDestOptionsStage);
        data->instc_options_main->bootloaded = FALSE;
        data->instc_options_grub->bootinfo = FALSE;
        data->instc_stage_next = EInstallMessageStage;
        data->instc_stage_prev = EInstallOptionsStage;
        break;

    case EInstallMessageStage:

        /* Back is disabled from here on... */

    case EPartitioningStage:
    case EInstallStage:
    case EDoneStage:
    default:
        break;
    }

    return TRUE;
}

IPTR Install__MUIM_IC_CancelInstall
    (Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    struct optionstmp *backupOptions = NULL;
    IPTR this_page = 0;
    const char *cancelmessage = NULL;

    if ((backupOptions = data->instc_options_backup) == NULL)
    {
        backupOptions =
            AllocMem(sizeof(struct optionstmp), MEMF_CLEAR | MEMF_PUBLIC);
        data->instc_options_backup = backupOptions;
    }

    GET(data->page, MUIA_Group_ActivePage, &this_page);

    GET(data->back, MUIA_Disabled, &data->status_back);
    GET(data->proceed, MUIA_Disabled, &data->status_proceed);
    GET(data->cancel, MUIA_Disabled, &data->status_cancel);

    switch (this_page)
    {
    case EPartitioningStage:
    case EInstallStage:
    case EDoneStage:
        cancelmessage = KMsgCancelDanger;
        break;

    case EInstallOptionsStage:
        GET(data->instc_options_main->opt_format, MUIA_Disabled,
            &backupOptions->opt_format);
        GET(data->instc_options_main->opt_locale, MUIA_Disabled,
            &backupOptions->opt_locale);
        GET(data->instc_options_main->opt_copycore, MUIA_Disabled,
            &backupOptions->opt_copycore);
        GET(data->instc_options_main->opt_copyextra, MUIA_Disabled,
            &backupOptions->opt_copyextra);
        GET(data->instc_options_main->opt_development, MUIA_Disabled,
            &backupOptions->opt_development);
        GET(data->instc_options_main->opt_bootloader, MUIA_Disabled,
            &backupOptions->opt_bootloader);
        GET(data->instc_options_main->opt_reboot, MUIA_Disabled,
            &backupOptions->opt_reboot);

        SET(data->instc_options_main->opt_format, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_locale, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_copycore, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_copyextra, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_development, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_bootloader, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_reboot, MUIA_Disabled, TRUE);
        goto donecancel;

    case EDestOptionsStage:
        SET(dest_volume, MUIA_Disabled, TRUE);
        SET(work_volume, MUIA_Disabled, TRUE);
        SET(check_copytowork, MUIA_Disabled, TRUE);
        SET(check_work, MUIA_Disabled, TRUE);
        goto donecancel;

    case EPartitionOptionsStage:
        SET(data->instc_options_main->opt_partmethod, MUIA_Disabled, TRUE);
        goto donecancel;

    case EGrubOptionsStage:
        goto donecancel;

    default:
      donecancel:
        cancelmessage = KMsgCancelOK;
        break;
    }

    SET(data->back, MUIA_Selected, FALSE);
    SET(data->back, MUIA_Disabled, TRUE);

    SET(data->proceed, MUIA_Selected, FALSE);
    SET(data->proceed, MUIA_Disabled, TRUE);

    SET(data->cancel, MUIA_Selected, FALSE);
    SET(data->cancel, MUIA_Disabled, TRUE);

    if (!MUI_RequestA(data->installer, data->window, 0,
            "Cancel Installation...", "*Continue Install|Cancel Install",
            cancelmessage, NULL))
    {
        DoMethod(self, MUIM_IC_QuitInstall);
    }
    else
        DoMethod(self, MUIM_IC_ContinueInstall);

    return 0;
}

IPTR Install__MUIM_IC_ContinueInstall
    (Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    struct optionstmp *backupOptions = NULL;
    IPTR this_page = 0;

    backupOptions = data->instc_options_backup;

    GET(data->page, MUIA_Group_ActivePage, &this_page);

    if (!(BOOL) data->disable_back)
        SET(data->back, MUIA_Disabled, data->status_back);
    else
        SET(data->back, MUIA_Disabled, TRUE);
    SET(data->back, MUIA_Selected, FALSE);

    SET(data->proceed, MUIA_Disabled, data->status_proceed);
    SET(data->proceed, MUIA_Selected, FALSE);

    SET(data->cancel, MUIA_Disabled, data->status_cancel);
    SET(data->cancel, MUIA_Selected, FALSE);

    switch (this_page)
    {
    case EInstallOptionsStage:
        SET(data->instc_options_main->opt_format, MUIA_Disabled,
            (BOOL) backupOptions->opt_format);
        SET(data->instc_options_main->opt_locale, MUIA_Disabled,
            (BOOL) backupOptions->opt_locale);
        SET(data->instc_options_main->opt_copycore, MUIA_Disabled,
            (BOOL) backupOptions->opt_copycore);
        SET(data->instc_options_main->opt_copyextra, MUIA_Disabled,
            (BOOL) backupOptions->opt_copyextra);
        SET(data->instc_options_main->opt_development, MUIA_Disabled,
            (BOOL) backupOptions->opt_development);
        SET(data->instc_options_main->opt_bootloader, MUIA_Disabled,
            (BOOL) backupOptions->opt_bootloader);
        SET(data->instc_options_main->opt_reboot, MUIA_Disabled,
            (BOOL) backupOptions->opt_reboot);
        break;

    case EDestOptionsStage:
        SET(dest_volume, MUIA_Disabled, FALSE);
        SET(check_work, MUIA_Disabled, FALSE);

        IPTR reenable = 0;
        GET(check_work, MUIA_Selected, &reenable);

        if (reenable)
        {
            SET(check_copytowork, MUIA_Disabled, FALSE);
            SET(work_volume, MUIA_Disabled, FALSE);
        }
        break;

    case EPartitionOptionsStage:
        SET(data->instc_options_main->opt_partmethod, MUIA_Disabled, FALSE);
        break;

    case EGrubOptionsStage:
        break;

    default:
        break;
    }

    return 0;
}

IPTR Install__MUIM_IC_QuitInstall(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);

    if (data->inst_success == MUIV_Inst_InProgress)
    {
        data->inst_success = MUIV_Inst_Cancelled;

        DoMethod(self, MUIM_Reboot);
    }

    return 0;
}

/* ****** FUNCTION IS CALLED BY THE PROCEDURE PROCESSOR

   IT LAUNCHES THE NECESSARY FUNCTION TO PERFORM WHATEVER IS BEING ASKED TO DO
 */

IPTR Install__MUIM_DispatchInstallProcedure
    (Class * CLASS, Object * self, Msg message)
{
    // struct Install_DATA* data = INST_DATA(CLASS, self);

    return 0;
}


static LONG GetPartitionSize(BOOL get_work)
{
    LONG size = -1;
    IPTR tmp = 0;

    if (!get_work)
    {
        if ((BOOL) XGET(check_sizesys, MUIA_Selected))
        {
            GET(sys_size, MUIA_String_Integer, &tmp);
            size = (LONG) tmp;
            if (XGET(cycle_sysunits, MUIA_Cycle_Active) == 1)
                size *= 1024;
        }
    }
    else
    {
        if ((BOOL) XGET(check_sizework, MUIA_Selected))
        {
            GET(work_size, MUIA_String_Integer, &tmp);
            size = (LONG) tmp;
            if (XGET(cycle_workunits, MUIA_Cycle_Active) == 1)
                size *= 1024;
        }
    }

    return size;
}

IPTR Install__MUIM_Partition(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR option = FALSE;
    IPTR tmp = 0;

    if (data->inst_success == MUIV_Inst_InProgress)
    {
        SET(data->back, MUIA_Disabled, TRUE);
        SET(data->proceed, MUIA_Disabled, TRUE);

        char tmpcmd[150], tmparg[100];
        GET(dest_device, MUIA_String_Contents, &tmp);
        GET(dest_unit, MUIA_String_Integer, &option);
        sprintf(tmpcmd, "C:Partition DEVICE=%s UNIT=%ld FORCE QUIET",
            (char *)tmp, option);

        /* Specify SYS size */
        GET(check_sizesys, MUIA_Selected, &option);
        if (option)
        {
            tmp = GetPartitionSize(FALSE);
            sprintf(tmparg, " SYSSIZE=%ld", tmp);
            strcat(tmpcmd, tmparg);
        }

        /* Specify SYS name */
        GET(sys_devname, MUIA_String_Contents, &tmp);
        sprintf(tmparg, " SYSNAME=\"%s\"", (char *)tmp);
        strcat(tmpcmd, tmparg);

        /* Specify SYS filesystem (defaults to FFSIntl) */
        get(cycle_fstypesys, MUIA_Cycle_Active, &tmp);
        if ((int)tmp == 1)
            strcat(tmpcmd, " SYSTYPE=SFS");
        else
            strcat(tmpcmd, " SYSTYPE=FFSIntl");

        /* Specify Work size */
        GET(check_creatework, MUIA_Selected, &option);
        if (option)
        {
            GET(check_sizework, MUIA_Selected, &option);
            if (option)
            {
                tmp = GetPartitionSize(TRUE);
                sprintf(tmparg, " WORKSIZE=%ld", tmp);
                strcat(tmpcmd, tmparg);
            }
            else
            {
                strcat(tmpcmd, " MAXWORK");
            }

            /* Specify WORK filesystem (defaults to SFS) */
            get(cycle_fstypework, MUIA_Cycle_Active, &tmp);
            if ((int)tmp == 0)
                strcat(tmpcmd, " WORKTYPE=FFSIntl");
            else
                strcat(tmpcmd, " WORKTYPE=SFS");

            /* Specify WORK name */
            GET(work_devname, MUIA_String_Contents, &tmp);
            sprintf(tmparg, " WORKNAME=\"%s\"", (char *)tmp);
            strcat(tmpcmd, tmparg);
        }

        /* Specify whether to wipe disk or not */
        GET(data->instc_options_main->opt_partmethod, MUIA_Radio_Active,
            &option);
        if (option == 1)
        {
            D(bug("[INSTALLER] Partitioning EVERYTHING! MUAHAHAHA...\n"));
            strcat(tmpcmd, " WIPE");
        }
        else
            D(bug("[INSTALLER] Partitioning Free Space...\n"));

        D(bug("[INSTALLER] ### Executing '%s'\n", &tmpcmd));
        tmp = SystemTagList(tmpcmd, NULL);

        SET(data->proceed, MUIA_Disabled, FALSE);
    }

    return tmp;
}

void create_environment_variable(CONST_STRPTR envarchiveDisk,
    CONST_STRPTR name, CONST_STRPTR value)
{
    BPTR env_variable_fh = NULL;
    TEXT env_variable_path[100];

    if ((envarchiveDisk == NULL) || (name == NULL) || (value == NULL))
        return;


    sprintf(env_variable_path, "%s:", envarchiveDisk);
    AddPart(env_variable_path, "Prefs/Env-Archive/", 100);
    AddPart(env_variable_path, name, 100);

    D(bug
        ("[INSTALLER] create_environment_variable: Setting Var '%s' to '%s'\n",
            env_variable_path, value));

    if ((env_variable_fh = Open(env_variable_path, MODE_NEWFILE)) != NULL)
    {
        FPuts(env_variable_fh, value);
        Close(env_variable_fh);
    }
}

static BOOL read_environment_variable(CONST_STRPTR envarchiveDisk,
    CONST_STRPTR name, STRPTR buffer, ULONG size)
{
    BPTR env_variable_fh = NULL;
    TEXT env_variable_path[100];

    if ((envarchiveDisk == NULL) || (name == NULL) || (buffer == NULL))
        return FALSE;

    sprintf(env_variable_path, "%s:", envarchiveDisk);
    AddPart(env_variable_path, "Prefs/Env-Archive/", 100);
    AddPart(env_variable_path, name, 100);

    D(bug("[INSTALLER] read_environment_variable: Getting Var '%s'\n",
        env_variable_path));

    if ((env_variable_fh = Open(env_variable_path, MODE_OLDFILE)) != NULL)
    {
        FGets(env_variable_fh, buffer, size);
        Close(env_variable_fh);
        return TRUE;
    }
    return FALSE;
}

LONG CountFiles(CONST_STRPTR directory, CONST_STRPTR fileMask,
    BOOL recursive)
{
    UBYTE *buffer = NULL;
    TEXT matchString[3 * strlen(fileMask)];
    BPTR dirLock = NULL;
    LONG fileCount = 0;

    D(bug("[INSTALLER.Count] Entry, directory: %s, mask: %s\n", directory,
            fileMask));

    /* Check if directory exists */
    dirLock = Lock(directory, SHARED_LOCK);

    if (dirLock == NULL)
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
                if (ead->ed_Type == ST_FILE
                    && MatchPatternNoCase(matchString, ead->ed_Name))
                    fileCount++;

                if (ead->ed_Type == ST_USERDIR && recursive)
                {
                    LONG subFileCount = 0;
                    ULONG subDirNameLen =
                        strlen(directory) + strlen(ead->ed_Name) + 2;
                    TEXT subDirName[subDirNameLen];

                    sprintf(subDirName, "%s", directory);
                    AddPart(subDirName, ead->ed_Name, subDirNameLen);

                    subFileCount =
                        CountFiles(subDirName, fileMask, recursive);

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
    CONST_STRPTR dstDir, CONST_STRPTR fileMask, BOOL recursive,
    LONG totalFiles, LONG totalFilesCopied)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    UBYTE *buffer = NULL;
    TEXT matchString[3 * strlen(fileMask)];
    BPTR srcDirLock = NULL, dstDirLock = NULL;
    LONG totalFilesCopiedThis = 0;

    /* Check entry condition */
    if (data->inst_success != MUIV_Inst_InProgress)
        return totalFilesCopied;

    /* Check if source directory exists */
    do
    {
        srcDirLock = Lock(srcDir, SHARED_LOCK);

        if (srcDirLock == NULL)
        {
            ULONG retry =
                AskRetry(CLASS, self, "Could not find %s\nRetry?", srcDir,
                "Yes", "Skip", "Quit");
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
    while (srcDirLock == NULL);

    /* Check if destination directory exists and create it */
    dstDirLock = Lock(dstDir, SHARED_LOCK);

    if (dstDirLock != NULL)
        UnLock(dstDirLock);
    else
    {
        dstDirLock = CreateDir(dstDir);
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
                if (((ead->ed_Type == ST_FILE)
                        && MatchPatternNoCase(matchString, ead->ed_Name))
                    || ((ead->ed_Type == ST_USERDIR) && recursive))
                {
                    ULONG srcLen = strlen(srcDir);
                    ULONG dstLen = strlen(dstDir);
                    ULONG newSrcLen = srcLen + strlen(ead->ed_Name) + 2;
                    ULONG newDstLen = dstLen + strlen(ead->ed_Name) + 2;

                    TEXT srcFile[newSrcLen];
                    TEXT dstFile[newDstLen];

                    sprintf(srcFile, "%s", srcDir);
                    sprintf(dstFile, "%s", dstDir);

                    AddPart(srcFile, ead->ed_Name, newSrcLen);
                    AddPart(dstFile, ead->ed_Name, newDstLen);

                    if (ead->ed_Type == ST_FILE)
                    {
                        totalFilesCopiedThis +=
                            (ULONG) DoMethod(self, MUIM_IC_CopyFile,
                            srcFile, dstFile);

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
                            InternalCopyFiles(CLASS, self, srcFile, dstFile,
                            fileMask,
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

IPTR Install__MUIM_IC_CopyFiles
    (Class * CLASS, Object * self, struct MUIP_CopyFiles * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    LONG totalFiles = -1, totalFilesCopied = 0;


    D(bug("[INSTALLER.CFs] Entry, src: %s, dst: %s, mask: %s\n",
            message->srcDir, message->dstDir, message->fileMask));

    /* Check entry condition */
    if (data->inst_success != MUIV_Inst_InProgress)
    {
        D(bug("[INSTALLER.CFs] Installation failed\n"));
        return totalFilesCopied;
    }

    SET(data->gauge2, MUIA_Gauge_Current, 0);

    /* Get file count */
    do
    {
        totalFiles =
            CountFiles(message->srcDir, message->fileMask,
            message->recursive);
        D(bug("[INSTALLER.CFs] Found %ld files in %s\n", totalFiles,
                message->srcDir));

        if (totalFiles < 0)
        {
            ULONG retry =
                AskRetry(CLASS, self, "Error scanning %s\nRetry?",
                message->srcDir, "Yes", "Skip", "Quit");
            switch (retry)
            {
            case 0:            /* retry */
                break;
            case 1:            /* skip */
                totalFiles = 0;
                break;
            default:           /* quit */
                DoMethod(self, MUIM_IC_QuitInstall);
                return totalFilesCopied;
            }
        }
    }
    while (totalFiles < 0);

    /* Copy files */
    totalFilesCopied =
        InternalCopyFiles(CLASS, self, message->srcDir, message->dstDir,
        message->fileMask, message->recursive, totalFiles,
        totalFilesCopied);

    /* Check final condition */
    if ((data->inst_success == MUIV_Inst_InProgress)
        && (totalFiles != totalFilesCopied))
    {
        TEXT msg[strlen(KMsgNotAllFilesCopied) + strlen(message->srcDir) +
            3];
        sprintf(msg, KMsgNotAllFilesCopied, message->srcDir);

        if (MUI_RequestA(data->installer, data->window, 0, "Error",
                "Continue|*Quit", msg, NULL) != 1)
            DoMethod(self, MUIM_IC_QuitInstall);

        return totalFilesCopied;
    }

    return totalFilesCopied;
}

IPTR Install__MUIM_IC_Install(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    BPTR lock = NULL;
    IPTR option = FALSE;

    GET(dest_volume, MUIA_String_Contents, &option);
    strcpy(dest_Path, (STRPTR) option);
    GET(work_volume, MUIA_String_Contents, &option);
    strcpy(work_Path, (STRPTR) option);

    SET(data->back, MUIA_Disabled, TRUE);
    SET(data->proceed, MUIA_Disabled, TRUE);

    SET(data->pagetitle, MUIA_Text_Contents, "Installing AROS...");

    /* set up destination Work name to use */

    GET(check_copytowork, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
        extras_path = work_Path;
    else
        extras_path = dest_Path;

    /* STEP : FORMAT */

    GET(data->instc_options_main->opt_format, MUIA_Selected, &option);
    if (option && data->inst_success == MUIV_Inst_InProgress)
    {
        GET(data->instc_options_main->opt_partmethod, MUIA_Radio_Active,
            &option);

        DoMethod(self, MUIM_Format);
    }

    /* MAKE SURE THE WORK PART EXISTS TO PREVENT CRASHING! */

    if ((BOOL) XGET(check_work, MUIA_Selected))
    {
        char tmp[100];
        sprintf(tmp, "%s:", work_Path);
        D(bug
            ("[INSTALLER] Install : Using a Work partition - checking validity..."));
        if ((lock = Lock(tmp, SHARED_LOCK)))    /* check the dest dir exists */
        {
            D(bug("OK!\n"));
            UnLock(lock);
        }
        else
        {
            D(bug
                ("FAILED!\n[INSTALLER] (Warning) INSTALL - Failed to locate chosen work partition '%s' : defaulting to sys only\n",
                    work_Path));
            extras_path = dest_Path;
        }
        lock = 0;
    }
    else
    {
        D(bug("[INSTALLER] Install: Using SYS partition only (%s)\n",
                dest_Path));
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : LOCALE */

    GET(data->instc_options_main->opt_locale, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        D(bug("[INSTALLER] Launching Locale Prefs...\n"));

        ULONG srcLen = strlen(source_Path), dstLen =
            (strlen(dest_Path) + 1);
        ULONG envsrcLen = strlen(prefssrc_path), envdstLen =
            strlen(prefs_path);

        ULONG localeFileLen = srcLen + strlen(localeFile_path) + 3;
        ULONG inputFileLen = srcLen + strlen(inputFile_path) + 3;

        ULONG localePFileLen =
            dstLen + envdstLen + strlen(locale_prfs_file) + 4;

        ULONG inputPFileLen =
            dstLen + envdstLen + strlen(input_prfs_file) + 4;

        ULONG envdstdirLen = 1024;
        TEXT envDstDir[envdstdirLen];   /* "DH0:Prefs/Env-Archive/SYS" */

        TEXT localeFile[localeFileLen]; /* "CD0:Prefs/Locale" */
        TEXT localesrcPFile[localePFileLen];    /* "ENV:SYS/locale.prefs" */
        TEXT localePFile[localePFileLen];       /* "DH0:Prefs/Env-Archive/SYS/locale.prefs" */
        TEXT inputFile[inputFileLen];   /* "CD0:Prefs/Input" */
        TEXT inputsrcPFile[inputPFileLen];      /* "ENV:SYS/input.prefs" */
        TEXT inputPFile[inputPFileLen]; /* "DH0:Prefs/Env-Archive/SYS/input.prefs" */

        sprintf(envDstDir, "%s:", dest_Path);
        sprintf(localeFile, "\"%s", source_Path);
        CopyMem(prefssrc_path, localesrcPFile, envsrcLen + 1);
        sprintf(localePFile, "%s:", dest_Path);
        sprintf(inputFile, "\"%s", source_Path);
        CopyMem(prefssrc_path, inputsrcPFile, envsrcLen + 1);
        sprintf(inputPFile, "%s:", dest_Path);

        AddPart(localeFile, inputFile_path, localeFileLen);

        AddPart(localesrcPFile, locale_prfs_file, localePFileLen);

        AddPart(localePFile, prefs_path, localePFileLen);
        AddPart(localePFile, locale_prfs_file, localePFileLen);

        AddPart(inputFile, localeFile_path, inputFileLen);

        AddPart(inputsrcPFile, input_prfs_file, inputPFileLen);

        AddPart(inputPFile, prefs_path, inputPFileLen);
        AddPart(inputPFile, input_prfs_file, inputPFileLen);

        D(bug("[INSTALLER] Excecuting '%s'...\n", localeFile));

        Execute(localeFile, NULL, NULL);

        DoMethod(data->installer, MUIM_Application_InputBuffered);

        D(bug("[INSTALLER] Excecuting '%s'...\n", inputFile));

        Execute(inputFile, NULL, NULL);

        DoMethod(data->installer, MUIM_Application_InputBuffered);

        D(bug("[INSTALLER] Copying Locale Settings...\n"));

        //create the dirs "Prefs","Prefs/Env-Archive" and "Prefs/Env-Archive/SYS"
        AddPart(envDstDir, "Prefs", dstLen + envdstLen);
        D(bug("[INSTALLER] Create Dir '%s' \n", envDstDir));

        BPTR bootDirLock = NULL;

        if ((lock = Lock(envDstDir, ACCESS_READ)) != NULL)
        {
            D(bug("[INSTALLER] Dir '%s' Exists - no nead to create\n",
                    envDstDir));
            UnLock(lock);
        }
        else
        {
            bootDirLock = CreateDir(envDstDir);
            if (bootDirLock != NULL)
                UnLock(bootDirLock);
            else
            {
              createdirfaild:
                D(bug("[INSTALLER] Failed to create %s dir!!\n",
                        envDstDir));
                /* TODO: Should prompt on failure to try again/continue anyhow/exit */
                goto localecopydone;
                //data->inst_success = MUIV_Inst_Failed;
                //return 0;
            }
        }

        bootDirLock = NULL;
        lock = 0;

        AddPart(envDstDir, "Env-Archive", envdstdirLen);
        D(bug("[INSTALLER] Create Dir '%s' \n", envDstDir));
        if ((lock = Lock(envDstDir, ACCESS_READ)) != NULL)
        {
            D(bug("[INSTALLER] Dir '%s' Exists - no nead to create\n",
                    envDstDir));
            UnLock(lock);
        }
        else
        {
            bootDirLock = CreateDir(envDstDir);
            if (bootDirLock != NULL)
                UnLock(bootDirLock);
            else
                goto createdirfaild;
        }

        bootDirLock = NULL;
        lock = 0;

        AddPart(envDstDir, "SYS", envdstdirLen);
        D(bug("[INSTALLER] Create Dir '%s' \n", envDstDir));
        if ((lock = Lock(envDstDir, ACCESS_READ)) != NULL)
        {
            D(bug("[INSTALLER] Dir '%s' Exists - no nead to create\n",
                    envDstDir));
            UnLock(lock);
        }
        else
        {
            bootDirLock = CreateDir(envDstDir);
            if (bootDirLock != NULL)
                UnLock(bootDirLock);
            else
                goto createdirfaild;
        }

        bootDirLock = NULL;
        lock = 0;

        D(bug("[INSTALLER] Copying files\n"));

        if ((lock = Lock(localesrcPFile, ACCESS_READ)) != NULL)
        {
            UnLock(lock);
            DoMethod(self, MUIM_IC_CopyFile, localesrcPFile, localePFile);
        }

        bootDirLock = NULL;
        lock = 0;

        if ((lock = Lock(inputsrcPFile, ACCESS_READ)) != NULL)
        {
            UnLock(lock);
            DoMethod(self, MUIM_IC_CopyFile, inputsrcPFile, inputPFile);
        }
      localecopydone:
        ;
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : COPY CORE */

    GET(data->instc_options_main->opt_copycore, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        TEXT tmp[100];
        BOOL success = FALSE;
        CONST_STRPTR core_dirs[] = {
            "C", "C",
            "Classes", "Classes",
            "Devs", "Devs",
            "Fonts", "Fonts",
            "Libs", "Libs",
            "Locale", "Locale",
            "Prefs", "Prefs",
            "Rexxc", "Rexxc",
            "S", "S",
            "Storage", "Storage",
            "System", "System",
            "Tools", "Tools",
            "Utilities", "Utilities",
            "WBStartup", "WBStartup",
            NULL
        };
        ULONG dstLen = strlen(dest_Path) + strlen(AROS_BOOT_FILE) + 2;
        TEXT destinationPath[dstLen];

        /* Copying Core system Files */
        D(bug("[INSTALLER] Copying Core files...\n"));
        SET(data->label, MUIA_Text_Contents,
            "Copying Core System files...");
        sprintf(destinationPath, "%s:", dest_Path);
        CopyDirArray(CLASS, self, source_Path, destinationPath, core_dirs);

        /* Copy kernel files */
        strcpy(tmp, source_Path);
        AddPart(tmp, BOOT_PATH, 100);
        sprintf(destinationPath, "%s:%s", dest_Path, BOOT_PATH);
        DoMethod(self, MUIM_IC_CopyFiles, tmp, destinationPath, "#?",
            FALSE);

        /* Copy AROS.boot file */
        sprintf(tmp, "%s", source_Path);
        sprintf(destinationPath, "%s:" AROS_BOOT_FILE, dest_Path);
        AddPart(tmp, AROS_BOOT_FILE, 100);
        DoMethod(self, MUIM_IC_CopyFile, tmp, destinationPath);

        /* Make Env-Archive Writeable */
        sprintf(tmp, "Protect ADD FLAGS=W ALL QUIET %s:Prefs/Env-Archive",
            dest_Path);
        D(bug
            ("[INSTALLER] Changing Protection on Env Files (command='%s')\n",
                tmp));
        success = (BOOL) Execute(tmp, NULL, NULL);

        if (!success)
        {
            D(bug
                ("[INSTALLER] Changing Protection on Env Files failed: %d\n",
                    IOErr()));
        }
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : COPY EXTRAS */

    GET(data->instc_options_main->opt_copyextra, MUIA_Selected, &option);
    if (option && data->inst_success == MUIV_Inst_InProgress)
    {
        CONST_STRPTR extras_dirs[] = {
            "Demos", "Demos",
            "Extras", "Extras",
            NULL
        };

        TEXT extraspath[100];
        BOOL undoenabled = data->instc_copt_undoenabled;

        /* Explicitly disable undo. Some users might not have RAM for backup */
        data->instc_copt_undoenabled = FALSE;

        /* Copying Extras */
        D(bug("[INSTALLER] Copying Extras to '%s'...\n", extras_path));
        SET(data->label, MUIA_Text_Contents, "Copying Extra Software...");
        sprintf(extraspath, "%s:", extras_path);
        CopyDirArray(CLASS, self, extras_source, extraspath, extras_dirs);

        /* Set EXTRASPATH environment variable */
        AddPart(extraspath, "Extras", 100);
        create_environment_variable(dest_Path, "EXTRASPATH", extraspath);

        /* Restore undo state */
        data->instc_copt_undoenabled = undoenabled;
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : COPY DEVELOPMENT */

    GET(data->instc_options_main->opt_development, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        ULONG srcLen = strlen(source_Path);
        ULONG developerDirLen = srcLen + strlen("Development") + 2;
        TEXT developerDir[srcLen + developerDirLen];

        CopyMem(source_Path, &developerDir, srcLen + 1);
        AddPart(developerDir, "Development", srcLen + developerDirLen);

        if ((lock = Lock(developerDir, SHARED_LOCK)) != NULL)
        {
            CONST_STRPTR developer_dirs[] = {
                "Development", "Development",
                "Tests", "Tests",
                NULL
            };
            TEXT developmentpath[100];
            BOOL undoenabled = data->instc_copt_undoenabled;

            /* Explicitly disable undo. Some users might not have RAM for backup */
            data->instc_copt_undoenabled = FALSE;

            UnLock(lock);

            /* Copying Developer stuff */
            D(bug("[INSTALLER] Copying Developer Files...\n"));
            SET(data->label, MUIA_Text_Contents,
                "Copying Developer Files...");
            sprintf(developmentpath, "%s:", extras_path);
            CopyDirArray(CLASS, self, source_Path, developmentpath,
                developer_dirs);

            /* Set DEVELPATH environment variable */
            AddPart(developmentpath, "Development", 100);
            create_environment_variable(dest_Path, "DEVELPATH",
                developmentpath);

            /* Restore undo state */
            data->instc_copt_undoenabled = undoenabled;
        }
        else
            D(bug("[INSTALLER] Couldn't locate Developer Files...\n"));
    }

    if (!option && (data->inst_success == MUIV_Inst_InProgress))
    {
        /* If not installing Development, delete the package file */
        TEXT packagePath[100];
        sprintf(packagePath, "%s:Prefs/Env-Archive/SYS/Packages/Developer",
            dest_Path);
        D(bug("[INSTALLER] Deleting Developer Package...\n"));
        DeleteFile(packagePath);
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : INSTALL BOOTLOADER */

    GET(data->instc_options_main->opt_bootloader, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        int numgrubfiles = 0, file_count = 0;
        LONG part_no;
        ULONG srcLen =
            strlen(source_Path) + strlen(BOOT_PATH) + strlen(GRUB_PATH) + 4;
        ULONG dstLen =
            strlen(dest_Path) + strlen(BOOT_PATH) + strlen(GRUB_PATH) + 4;
        TEXT srcPath[srcLen];
        TEXT dstPath[dstLen];
        TEXT tmp[256];

        /* Installing GRUB */
        D(bug("[INSTALLER] Installing Grub...\n"));
        SET(data->label, MUIA_Text_Contents, "Installing Grub...");
        SET(data->pageheader, MUIA_Text_Contents, KMsgBootLoader);
        SET(data->label, MUIA_Text_Contents, "Copying BOOT files...");

        strcpy(srcPath, source_Path);
        AddPart(srcPath, BOOT_PATH, srcLen);
        sprintf(dstPath, "%s:%s", dest_Path, BOOT_PATH);
        AddPart(srcPath, GRUB_PATH, srcLen);
        AddPart(dstPath, GRUB_PATH, dstLen);
        /* Warning: do not modify srcPath or dstPath beyond this point */

        /* Get drive chosen to install GRUB bootblock to */
        GET(grub_device, MUIA_String_Contents, &option);
        strcpy(boot_Device, (STRPTR) option);
        boot_Unit = XGET(grub_unit, MUIA_String_Integer);

        switch (BootLoaderType)
        {
        case BOOTLOADER_GRUB2:

            DoMethod(self, MUIM_IC_CopyFiles, srcPath, dstPath, "#?.mod",
                FALSE);

            TEXT *grub2_files[] = {
                "boot.img", "boot.img",
                "core.img", "core.img",
                "grub.cfg", "grub.cfg",
                "splash.png", "splash.png",
                "_unicode.pf2", "_unicode.pf2",
                "command.lst", "command.lst",
                "fs.lst", "fs.lst",
                "moddep.lst", "moddep.lst",
                NULL
            };
            numgrubfiles = (sizeof(grub2_files) / sizeof(TEXT *) - 1) / 2;

            GRUB_COPY_FILE_LOOP(grub2_files);

            /* Grub 2 text/gfx mode */
            GET(data->instc_options_grub->gopt_grub2mode, MUIA_Cycle_Active,
                &option);
            if (option == 1)
            {
                /* gfx mode - copy _unicode.pf2 -> unicode.pf2 */
                ULONG newDstLen =
                    strlen(dstPath) + strlen("_unicode.pf2") + 2;
                TEXT srcFile[newDstLen];
                TEXT dstFile[newDstLen];

                sprintf(srcFile, "%s", dstPath);
                sprintf(dstFile, "%s", dstPath);
                AddPart(srcFile, "_unicode.pf2", newDstLen);
                AddPart(dstFile, "unicode.pf2", newDstLen);

                DoMethod(self, MUIM_IC_CopyFile, srcFile, dstFile);
            }
            else
            {
                /* other - delete unicode.pf2 */
                ULONG newDstLen =
                    strlen(dstPath) + strlen("unicode.pf2") + 2;
                TEXT dstFile[newDstLen];

                sprintf(dstFile, "%s", dstPath);
                AddPart(dstFile, "unicode.pf2", newDstLen);

                DeleteFile(dstFile);
            }

            /* Add entry to boot MS Windows if present */
            if ((part_no =
                    FindWindowsPartition(boot_Device, boot_Unit)) != -1
                && XGET(cycle_drivetype, MUIA_Cycle_Active) == 0)
            {
                sprintf(tmp, "%s", dstPath);
                AddPart(tmp, "grub.cfg", 256);

                BPTR menu_file = Open(tmp, MODE_READWRITE);

                if (menu_file != NULL)
                {
                    Seek(menu_file, 0, OFFSET_END);
                    FPrintf(menu_file, "\nmenuentry \"Microsoft Windows\" {\n    chainloader (hd%ld,%ld)+1\n}\n\n", 0, part_no + 1);    /* GRUB2 counts partitions from 1 */
                    Close(menu_file);
                }
                D(bug
                    ("[INSTALLER] Windows partition found. Adding Windows option to GRUB2 menu.\n"));
            }

            sprintf(tmp,
                "C:Install-grub2-i386-pc DEVICE \"%s\" UNIT %d GRUB \"%s\"",
                boot_Device, boot_Unit, dstPath);

            break;
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
                    FindWindowsPartition(boot_Device, boot_Unit)) != -1)
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
                    ("[INSTALLER] Windows partition found. Adding Windows option to GRUB menu.\n"));
            }

            sprintf(tmp,
                "C:install-i386-pc DEVICE \"%s\" UNIT %d GRUB \"%s\" FORCELBA",
                boot_Device, boot_Unit, dstPath);

            break;
            /* TODO: support more bootloaders */
        }

        D(bug("[INSTALLER] execute: %s\n", tmp));
        Execute(tmp, NULL, NULL);
        SET(data->gauge2, MUIA_Gauge_Current, 100);
    }

    SET(data->proceed, MUIA_Disabled, FALSE);

    /* STEP : PACKAGE CLEANUP */
    /* REMOVED - handled by DEVELPATH and EXTRASPATH variables */

    /* STEP : CREATE ENVIRONMENT VARIABLES THAT POINT TO INSTALLATION LOCATIONS */
    {
        TEXT varval[255];
        IPTR optcheck = 0;

        /* Volume name of installed SYS */
        sprintf(varval, "%s:", dest_Path);
        SetVar("INSTALLEDSYS", varval, strlen(varval), GVF_GLOBAL_ONLY);

        /* Volume name of installed WORK */
        GET(check_work, MUIA_Selected, &optcheck);
        if (optcheck)
        {
            sprintf(varval, "%s:", work_Path);
            SetVar("INSTALLEDWORK", varval, strlen(varval),
                GVF_GLOBAL_ONLY);
        }
        else
            SetVar("INSTALLEDWORK", "", 0, GVF_GLOBAL_ONLY);

        /* Path to Extras */
        sprintf(varval, "%s:", extras_path);
        AddPart(varval, "Extras", 255);
        SetVar("INSTALLEDEXTRAS", varval, strlen(varval), GVF_GLOBAL_ONLY);
    }


    /* STEP : EXECUTE EXTERNAL POST-INSTALL SCRIPT */
    if ((BOOL) XGET(data->instc_options_main->opt_copycore, MUIA_Selected))
    {
        BPTR scriptfile = Open(POST_INSTALL_SCRIPT, MODE_OLDFILE);
        if (scriptfile)
        {

            D(bug("[INSTALLER] Running post-install script...\n"));
            SET(data->label, MUIA_Text_Contents,
                "Running post-install script...");
            SET(data->pageheader, MUIA_Text_Contents, KMsgPostInstall);
            SET(data->gauge2, MUIA_Gauge_Current, 0);
            SET(data->actioncurrent, MUIA_Text_Contents,
                POST_INSTALL_SCRIPT);


            /* Post install script (at this momement) does not allow user interaction.
               Set SYS_Input to opened console and SYS_Background to FALSE to allow it. */

            struct TagItem tags[] = {
                {SYS_Input, (IPTR) NULL},
                {SYS_Output, (IPTR) NULL},
                {SYS_Error, (IPTR) NULL},
                {SYS_ScriptInput, (IPTR) scriptfile},
                {SYS_UserShell, TRUE},
                {TAG_DONE, 0}
            };

            D(bug("[INSTALLER] execute: %s\n", POST_INSTALL_SCRIPT));

            SystemTagList("", tags);

            /* Do not close scriptfile. It was closed by SystemTagList */

            SET(data->gauge2, MUIA_Gauge_Current, 100);
        }
        D(
            else
                bug("[INSTALLER] no post-install script\n"));
    }

    /* STEP : UNDORECORD CLEANUP */

    D(bug
        ("[INSTALLER] Reached end of Install Function - cleaning up undo logs @ %p...\n",
            &data->instc_undorecord));

    struct InstallC_UndoRecord *CurUndoNode = NULL;
    struct Node *undonode_tmp = NULL;

    ForeachNodeSafe(&data->instc_undorecord, CurUndoNode, undonode_tmp)
    {
        D(bug("[INSTALLER] Removing undo record @ %p\n", CurUndoNode));
        Remove((struct Node *)CurUndoNode);

        switch (CurUndoNode->undo_method)
        {
        case MUIM_IC_CopyFile:
            D(bug("[INSTALLER] Deleting undo file '%s'\n",
                    CurUndoNode->undo_src));
            DeleteFile(CurUndoNode->undo_src);

            FreeVec(CurUndoNode->undo_dst);
            FreeVec(CurUndoNode->undo_src);
            break;
        default:
            continue;
        }
        FreeMem(CurUndoNode, sizeof(struct InstallC_UndoRecord));
    }

    return 0;
}

IPTR Install__MUIM_RefreshWindow(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    ULONG cur_width = 0, cur_height = 0;

    GET(data->window, MUIA_Window_Width, &cur_width);
    GET(data->window, MUIA_Window_Height, &cur_height);

    if ((data->cur_width != cur_width) || (data->cur_height != cur_height))
    {
        DoMethod(data->contents, MUIM_Hide);
        DoMethod(data->contents, MUIM_Layout);
        DoMethod(data->contents, MUIM_Show);
    }
    else
        MUI_Redraw(data->contents, MADF_DRAWOBJECT);

    return 0;
}

static LONG FindWindowsPartition(STRPTR device, LONG unit)
{
    IPTR active, id;
    struct PartitionType type;
    struct PartitionHandle *root, *partition;
    LONG partition_no = -1, i = 0;

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


LONG CopyDirArray(Class * CLASS, Object * self, CONST_STRPTR sourcePath,
    CONST_STRPTR destinationPath, CONST_STRPTR directories[])
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    LONG numdirs = 0, dir_count = 0;
    BPTR lock = 0;

    while (directories[numdirs] != NULL)
        numdirs++;

    numdirs = (numdirs - 1) / 2;

    D(bug("[INSTALLER.CDA] Copying %d Dirs...\n", numdirs));

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
        DoMethod(self, MUIM_IC_CopyFiles, srcDirs, dstDirs, "#?", TRUE);

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
            lock = Lock(dstDirs, SHARED_LOCK);
            if (!lock)
            {
                UnLock(lock);
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

IPTR Install__MUIM_Format(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    char dev_nametmp[100];
    char vol_nametmp[100] = SYS_VOL_NAME;
    char fmt_nametmp[100];
    BOOL success = FALSE;
    IPTR option = FALSE;
    BPTR lock = NULL;
    char tmp[100];

    if ((BOOL) XGET(check_formatsys, MUIA_Selected))
    {
        /* Format Vol0 */
        sprintf(fmt_nametmp, "Formatting '%s'...", dest_Path);
        D(bug("[INSTALLER] %s\n", fmt_nametmp));
        SET(data->label, MUIA_Text_Contents, fmt_nametmp);
        SET(data->gauge2, MUIA_Gauge_Current, 0);

        /* Change volume name if installing to a USB drive */
        GET(grub_device, MUIA_String_Contents, &option);
        if (strcmp((char *)option, "usbscsi.device") == 0)
            strcpy(vol_nametmp, USB_SYS_VOL_NAME);

        sprintf(dev_nametmp, "%s:", dest_Path);

        /* XXX HACK
         * If partition is FFS -> it will format it for FFS
         * If partition is SFS -> it will format it for SFS
         * Correct way of doing things: read type for DH0 and DH1, apply correct
         * type when formatting
         */
        D(bug("[INSTALLER] (info) Using FormatPartition\n"));
        success =
            FormatPartition(dev_nametmp, vol_nametmp, ID_INTER_FFS_DISK);

        if (success)
            set(data->gauge2, MUIA_Gauge_Current, 100);
    }

    GET(check_work, MUIA_Selected, &option);
    if (option && XGET(check_formatwork, MUIA_Selected))
    {
        /* Format Vol1 */
        sprintf(fmt_nametmp, "Formatting '%s'...", work_Path);
        D(bug("[INSTALLER] %s\n", fmt_nametmp));
        SET(data->label, MUIA_Text_Contents, fmt_nametmp);

        SET(data->gauge2, MUIA_Gauge_Current, 0);

        /* Change volume name if installing to a USB drive */
        GET(grub_device, MUIA_String_Contents, &option);
        if (strcmp((char *)option, "usbscsi.device") == 0)
            strcpy(vol_nametmp, USB_WORK_VOL_NAME);

        sprintf(dev_nametmp, "%s:", work_Path);

        /* XXX HACK
         * If partition is FFS -> it will format it for FFS
         * If partition is SFS -> it will format it for SFS
         * Correct way of doing things: read type for DH0 and DH1, apply
         * correct type when formatting (ID_INTER_FFS_DISK or ID_SFS_BE_DISK)
         */
        D(bug("[INSTALLER] (info) Using FormatPartition\n"));
        success =
            FormatPartition(dev_nametmp, vol_nametmp, ID_INTER_FFS_DISK);

        if (success)
        {
            sprintf(tmp, "%s:", work_Path);
            SET(data->gauge2, MUIA_Gauge_Current, 100);
            lock = Lock(tmp, SHARED_LOCK);      /* check the dest dir exists */
            if (lock == 0)
            {
                D(bug
                    ("[INSTALLER] (Warning) FORMAT: Failed for chosen work partition '%s' : defaulting to sys only\n",
                        tmp));
                extras_path = dest_Path;
            }
            else
            {
                UnLock(lock);
                lock = 0;
            }
        }
    }
    if (success)
        SET(data->gauge2, MUIA_Gauge_Current, 100);

    return success;
}

BPTR RecursiveCreateDir(CONST_STRPTR dirpath)
{
    /* Will create directory even if top level directory does not exist */

    BPTR lock = NULL;
    ULONG lastdirseparator = 0;
    ULONG dirpathlen = strlen(dirpath);
    STRPTR tmpdirpath = AllocVec(dirpathlen + 2, MEMF_CLEAR | MEMF_PUBLIC);

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
        if (lock != NULL)
        {
            UnLock(lock);
            lock = NULL;
        }

        /* Check if directory exists */
        lock = Lock(tmpdirpath, SHARED_LOCK);
        if (lock == NULL)
        {
            lock = CreateDir(tmpdirpath);
            if (lock == NULL)
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
    BPTR lock = NULL, from = NULL, to = NULL;
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

    D(bug("[INSTALLER.CF] Backup '%s' @ '%s'\n", undorecord->undo_dst,
            undorecord->undo_src));

    undorecord->undo_method = MUIM_IC_CopyFile;

    /* Create backup directory */
    if ((lock = Lock(tmp, SHARED_LOCK)) != NULL)
    {
        D(bug("[INSTALLER.CF] Dir '%s' Exists - no need to create\n", tmp));
        UnLock(lock);
    }
    else
    {
        lock = RecursiveCreateDir(tmp);
        if (lock != NULL)
            UnLock(lock);
        else
        {
            D(bug("[INSTALLER.CF] Failed to create %s dir!!\n", tmp));
            FreeVec(tmp);
            return FALSE;
        }
    }

    FreeVec(tmp);

    /* Copy file */
    if ((from = Open(undorecord->undo_dst, MODE_OLDFILE)))
    {
        if ((to = Open(undorecord->undo_src, MODE_NEWFILE)))
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

IPTR Install__MUIM_IC_CopyFile
    (Class * CLASS, Object * self, struct MUIP_CopyFile * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    static TEXT buffer[kBufSize];
    struct InstallC_UndoRecord *undorecord = NULL;
    ULONG retry = 0;
    ULONG filescopied = 0;
    BPTR from = NULL, to = NULL;

    /* Display copied file name */
    SET(data->actioncurrent, MUIA_Text_Contents, strchr(message->srcFile,
            ':') + 1);

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* Check if destination file exists */
    if ((to = Open(message->dstFile, MODE_OLDFILE)))
    {
        /* File exists */
        ULONG l;

        Close(to);

        /* Do not overwrite existing icons and preferences.
           TODO: May be ask about it too? */
        l = strlen(message->dstFile);
        if ((l > 4) && (!stricmp(&message->dstFile[l - 5], ".info")))
        {
            /* Count the file as copied because otherwise installer will warn that
               not everything was copied. */
            filescopied = 1;
            goto copy_skip;
        }
        else if ((l > 5) && (!stricmp(&message->dstFile[l - 6], ".prefs")))
        {
            filescopied = 1;
            goto copy_skip;
        }
        else
        {
            switch (data->IO_Always_overwrite)
            {
            case IIO_Overwrite_Ask:
                retry =
                    AskRetry(CLASS, self,
                    "File Already Exists\nReplace %s?", message->dstFile,
                    "Yes", "Yes [Always]", "No");
                switch (retry)
                {
                case 0:        /* Yes */
                    goto copy_backup;
                case 1:        /* Always */
                    data->IO_Always_overwrite = IIO_Overwrite_Always;
                    goto copy_backup;
                default:       /* NO! */
                    goto copy_skip;
                }
            case IIO_Overwrite_Always:
                goto copy_backup;
            case IIO_Overwrite_Never:
                goto copy_skip;
            }
        }
    }
    else
        goto copy_retry;

  copy_backup:

    /* if the user has requested - backup all replaced files */

    if (data->instc_copt_undoenabled)
    {
        if ((undorecord =
                AllocMem(sizeof(struct InstallC_UndoRecord),
                    MEMF_CLEAR | MEMF_PUBLIC)) == NULL)
            DoMethod(self, MUIM_IC_QuitInstall);

        if (!BackUpFile(message->dstFile, INSTALLAROS_TMP_PATH, undorecord))
        {
            data->inst_success = MUIV_Inst_Failed;
            return 0;
        }
    }

    /* Main copy code */
  copy_retry:

    if ((from = Open(message->srcFile, MODE_OLDFILE)))
    {
        if ((to = Open(message->dstFile, MODE_NEWFILE)))
        {
            LONG s = 0;

            do
            {
                if ((s = Read(from, buffer, kBufSize)) == -1)
                {
                    D(bug("[INSTALLER.CF] Failed to read: %s [ioerr=%d]\n",
                            message->srcFile, IoErr()));

                    Close(to);
                    Close(from);

                    retry =
                        AskRetry(CLASS, self, "Couldn't Open %s",
                        message->srcFile, "Retry", "Skip", "Cancel");
                    switch (retry)
                    {
                    case 0:    /* Retry */
                        goto copy_retry;
                    case 1:    /*Skip */
                        goto copy_skip;
                    default:
                        DoMethod(self, MUIM_IC_QuitInstall);
                        goto copy_skip;
                    }
                }

                DoMethod(data->installer, MUIM_Application_InputBuffered);

                if (Write(to, buffer, s) == -1)
                {
                    D(bug
                        ("[INSTALLER.CF] Failed to write: %s  [%d bytes, ioerr=%d]\n",
                            message->dstFile, s, IoErr()));

                    if (IoErr() == 103)
                        retry =
                            AskRetry(CLASS, self,
                            "Couldn't Write to %s\nDisk Full!",
                            message->dstFile, "Retry", "Skip", "Cancel");
                    else
                        retry =
                            AskRetry(CLASS, self, "Couldn't Write to %s",
                            message->dstFile, "Retry", "Skip", "Cancel");

                    Close(to);
                    Close(from);

                    switch (retry)
                    {
                    case 0:    /* Retry */
                        goto copy_retry;
                    case 1:    /*Skip */
                        goto copy_skip;
                    default:
                        DoMethod(self, MUIM_IC_QuitInstall);
                        goto copy_skip;
                    }
                }
            }
            while ((s == kBufSize)
                && (data->inst_success == MUIV_Inst_InProgress));

            if (data->inst_success == MUIV_Inst_InProgress)
                filescopied = 1;

            Close(to);
        }
        else
        {
            D(bug
                ("[INSTALLER.CF] Failed to open '%s' for writing [ioerr=%d]\n",
                    message->dstFile, IoErr()));
            data->inst_success = MUIV_Inst_Failed;
        }
        Close(from);

      copy_skip:
        /* Add the undo record */
        if (undorecord != NULL)
        {
            if (filescopied > 0)
            {
                D(bug
                    ("[INSTALLER.CF] Adding undo record @ %x to undo list @ %x \n",
                        undorecord, &data->instc_undorecord));
                AddHead(&data->instc_undorecord, (struct Node *)undorecord);
            }
            else
            {
                D(bug("[INSTALLER.CF] Freeing undo record\n"));
                /* remove the backup file */

                DeleteFile(undorecord->undo_src);

                /* remove the undo record */
                FreeVec(undorecord->undo_dst);
                FreeVec(undorecord->undo_src);
                FreeMem(undorecord, sizeof(struct InstallC_UndoRecord));
            }
        }
    }
    else
    {
        D(bug("[INSTALLER.CF] Failed to open: %s [ioerr=%d]\n",
                message->srcFile, IoErr()));
        data->inst_success = MUIV_Inst_Failed;
    }

    return filescopied;
}

IPTR Install__MUIM_IC_UndoSteps(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    struct InstallC_UndoRecord *CurUndoNode = NULL;

    D(bug("[INSTALLER.US] Performing UNDO steps...\n"));

    /* Disbale "UNDO" mode to prevent new records */
    data->instc_copt_undoenabled = FALSE;

    ForeachNode(&data->instc_undorecord, CurUndoNode)
    {
        D(bug("[INSTALLER.US] Removing undo record @ %x\n", CurUndoNode));
        Remove((struct Node *)CurUndoNode);

        switch (CurUndoNode->undo_method)
        {
        case MUIM_IC_CopyFile:
            D(bug("[INSTALLER.US] Reverting file '%s'\n",
                    CurUndoNode->undo_dst));

            DoMethod(self, CurUndoNode->undo_method, CurUndoNode->undo_src,
                CurUndoNode->undo_dst);

            D(bug("[INSTALLER.US] Deleting undo file '%s'\n",
                    CurUndoNode->undo_src));
            DeleteFile(CurUndoNode->undo_src);

            FreeVec(CurUndoNode->undo_dst);
            FreeVec(CurUndoNode->undo_src);
            break;
        default:
            continue;
        }
        FreeMem(CurUndoNode, sizeof(struct InstallC_UndoRecord));
    }

    D(bug("[INSTALLER.US] UNDO complete\n"));

    return 0;
}

IPTR Install__MUIM_Reboot(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);

    IPTR option = FALSE;

    /* Make sure the user wants to reboot */
    GET(data->instc_options_main->opt_reboot, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        D(bug("[INSTALLER] Cold rebooting...\n"));
        ShutdownA(SD_ACTION_COLDREBOOT);
    }
    else
    {
        D(bug("[INSTALLER] Install Finished [no reboot]...\n"));
        if (data->inst_success == MUIV_Inst_InProgress)
            data->inst_success = MUIV_Inst_Completed;
        SET(data->window, MUIA_Window_CloseRequest, TRUE);
    }

    return TRUE;                /* Keep the compiler happy... */
}

IPTR Install__OM_SET(Class * CLASS, Object * self, struct opSet * message)
{
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

BOOPSI_DISPATCHER(IPTR, Install_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW:
        return Install__OM_NEW(CLASS, self, (struct opSet *)message);

    case OM_SET:
        return Install__OM_SET(CLASS, self, (struct opSet *)message);

    case MUIM_FindDrives:
        return Install__MUIM_FindDrives(CLASS, self, message);

    case MUIM_IC_NextStep:
        return Install__MUIM_IC_NextStep(CLASS, self, message);

    case MUIM_IC_PrevStep:
        return Install__MUIM_IC_PrevStep(CLASS, self, message);
        //cancel control methods
    case MUIM_IC_CancelInstall:
        return Install__MUIM_IC_CancelInstall(CLASS, self, message);

    case MUIM_IC_ContinueInstall:
        return Install__MUIM_IC_ContinueInstall(CLASS, self, message);

    case MUIM_IC_QuitInstall:
        return Install__MUIM_IC_QuitInstall(CLASS, self, message);

    case MUIM_Reboot:
        return Install__MUIM_Reboot(CLASS, self, message);

        //This should disappear
    case MUIM_RefreshWindow:
        return Install__MUIM_RefreshWindow(CLASS, self, message);

    case MUIM_IC_Install:
        return Install__MUIM_IC_Install(CLASS, self, message);

        //These will be consumed by the io task
    case MUIM_Partition:
        return Install__MUIM_Partition(CLASS, self, message);

    case MUIM_Format:
        return Install__MUIM_Format(CLASS, self, message);

    case MUIM_IC_CopyFiles:
        return Install__MUIM_IC_CopyFiles(CLASS, self,
            (struct MUIP_CopyFiles *)message);

    case MUIM_IC_CopyFile:
        return Install__MUIM_IC_CopyFile(CLASS, self,
            (struct MUIP_CopyFile *)message);

    case MUIM_IC_UndoSteps:
        return Install__MUIM_IC_UndoSteps(CLASS, self, message);

    default:
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}

BOOPSI_DISPATCHER_END void FindBootLoader(void)
{
    ULONG newSrcLen = strlen(source_Path) + BOOTLOADER_PATH_LEN;
    TEXT srcFile[newSrcLen];
    LONG i;
    BPTR lock;

    for (i = 0; BootLoaderFiles[i]; i++)
    {
        strcpy(srcFile, source_Path);
        AddPart(srcFile, BootLoaderFiles[i], newSrcLen);
        lock = Lock(srcFile, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            BootLoaderType = i;
            return;
        }
    }
    BootLoaderType = BOOTLOADER_NONE;
}

int main(int argc, char *argv[])
{
    Object *wnd = NULL;         /* installer window objects  - will get swallowed into the class eventually */
    Object *wndcontents = NULL;
    Object *page = NULL;

    Object *welcomeMsg = NULL;
    Object *LicenseMsg = NULL;
    Object *doneMsg = NULL;

    Object *pagetitle = NULL;
    Object *pageheader = NULL;
    Object *currentaction = NULL;

    Object *radio_part = NULL;

    Object *gad_back = SimpleButton("<< _Back...");
    Object *gad_proceed = SimpleButton(KMsgProceed);
    Object *gad_cancel = SimpleButton("_Cancel");

    Object *grub_drive = NULL;
    Object *grub_grub = NULL;

    Object *LicenseMandGrp = NULL;
    Object *check_license =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, End;

    Object *check_format =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, TRUE, End;
    Object *check_locale =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, End;
    Object *check_core =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, TRUE, End;
    Object *check_dev =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, End;
    Object *check_extras =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, TRUE, End;
    Object *check_bootloader;

    Object *check_reboot =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, End;

    Object *gauge1 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);
    Object *gauge2 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);
    Object *gauge3 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);

    static char *opt_drivetypes[] = {
        "IDE/SATA",
        "USB",
        NULL
    };

    Object *label = NULL;
    static char *opt_partentries[] = {
        "Only use free space",
        "Wipe disk",
        "Use existing AROS partitions (on any drive)",
        NULL
    };
    struct Install_Options *install_opts = NULL;
    struct Grub_Options *grub_opts = NULL;
    char *source_path = NULL;
    char *dest_path = NULL;
    char *work_path = NULL;

    IPTR pathend = 0;

    check_copytowork =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Disabled, TRUE, End;
    check_work =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, End;
    check_formatsys =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, TRUE, End;
    check_formatwork =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Disabled, TRUE, End;

    check_sizesys = ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, End;
    check_sizework = ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, MUIA_Disabled, TRUE,
        End;
    check_creatework = ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected, FALSE, MUIA_Disabled, TRUE,
        End;

    static char *opt_fstypes[] = {
        "FFS-Intl",
        "SFS",
        NULL
    };

    cycle_fstypework =
        CycleObject, MUIA_Cycle_Entries, opt_fstypes, MUIA_Disabled, TRUE,
        MUIA_Cycle_Active, 1, End;

    static char *opt_sizeunits[] = {
        "MB",
        "GB",
        NULL
    };

    cycle_sysunits =
        CycleObject, MUIA_Cycle_Entries, opt_sizeunits, MUIA_Disabled, TRUE,
        MUIA_Cycle_Active, 1, End;
    cycle_workunits =
        CycleObject, MUIA_Cycle_Entries, opt_sizeunits, MUIA_Disabled, TRUE,
        MUIA_Cycle_Active, 1, End;

    static char *opt_grub2mode[] = {
        "Text",
        "Graphics",
        NULL
    };

    Object *cycle_grub2mode =
        CycleObject, MUIA_Cycle_Entries, opt_grub2mode, MUIA_Disabled,
        FALSE, MUIA_Cycle_Active, 0, End;

    install_opts =
        AllocMem(sizeof(struct Install_Options), MEMF_CLEAR | MEMF_PUBLIC);
    grub_opts =
        AllocMem(sizeof(struct Grub_Options), MEMF_CLEAR | MEMF_PUBLIC);
    source_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);
    extras_source = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);

    dest_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);
    work_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);

    BPTR lock = 0;

    if (!(ExpansionBase =
            (struct ExpansionBase *)OpenLibrary("expansion.library", 0)))
        goto main_error;

    if (!NameFromLock(GetProgramDir(), source_path, 255))
    {
        D(bug("[INST-APP] Couldn't get progdir\n"));
        goto main_error;
    }
    pathend = (IPTR) FilePart(source_path);
    pathend = pathend - (IPTR) source_path;

    D(bug("[INST-APP] Path length = %d bytes\n", pathend));

    source_Path = AllocVec(pathend + 1, MEMF_CLEAR | MEMF_PUBLIC);
    CopyMem(source_path, source_Path, pathend);
    D(bug("[INST-APP] Launched from '%s'\n", source_Path));
    FreeVec(source_path);

    /* Get source location for Extras dir */
    if (read_environment_variable(source_Path, "EXTRASPATH", extras_source,
            256))
        *PathPart(extras_source) = '\0';
    else
        strcpy(extras_source, source_Path);

    dest_Path = dest_path;
    sprintf(dest_Path, "" SYS_PART_NAME);

    work_Path = work_path;
    sprintf(work_Path, "" WORK_PART_NAME);

    FindBootLoader();
    cycle_fstypesys =
        CycleObject, MUIA_Cycle_Entries, opt_fstypes, MUIA_Disabled, FALSE,
        MUIA_Cycle_Active, BootLoaderType == BOOTLOADER_GRUB1 ? 0 : 1, End;
    check_bootloader =
        ImageObject, ImageButtonFrame, MUIA_InputMode,
        MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark,
        MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack,
        MUIA_ShowSelState, FALSE, MUIA_Selected,
        BootLoaderType == BOOTLOADER_NONE ? FALSE : TRUE, MUIA_Disabled,
        BootLoaderType == BOOTLOADER_NONE ? TRUE : FALSE, End;

    lock = Lock(DEF_INSTALL_IMAGE, ACCESS_READ);
    UnLock(lock);

    LicenseMsg = MUI_NewObject(MUIC_TextEditor,
        MUIA_Background, MUII_SHINE,
        MUIA_TextEditor_ReadOnly, TRUE, TAG_DONE);

    if (!LicenseMsg)
    {
        D(bug("[INST-APP] Failed to create LicenseMsg Object\n"));
        exit(5);
    }

    Object *app = ApplicationObject,
        MUIA_Application_Title,       (IPTR) "AROS Installer",
        MUIA_Application_Version,     (IPTR) "$VER: InstallAROS 1.11 (28.11.2011)",
        MUIA_Application_Copyright,   (IPTR) "Copyright © 2003-2011, The AROS Development Team. All rights reserved.",
        MUIA_Application_Author,      (IPTR) "John \"Forgoil\" Gustafsson, Nic Andrews & Neil Cafferkey",
        MUIA_Application_Description, (IPTR) "Installs AROS on to a PC.",
        MUIA_Application_Base,        (IPTR) "INSTALLER",

        SubWindow, (IPTR) (wnd = WindowObject,
            MUIA_Window_Title, (IPTR) "AROS Installer",
            MUIA_Window_ID, MAKE_ID('f','o','r','g'),
            MUIA_Window_SizeGadget, TRUE,
            WindowContents, (IPTR) (wndcontents = VGroup,

                Child, (IPTR) VGroup,
                    Child, (IPTR) HGroup,
                        Child, (IPTR) VGroup,
                            MUIA_Background, MUII_SHADOW,    

                            Child, (IPTR) ImageObject,
                                MUIA_Frame,             MUIV_Frame_None,
                                MUIA_Image_Spec, (IPTR) "3:"DEF_INSTALL_IMAGE,
                            End,
                            Child, (IPTR) HVSpace,
                        End,

                        Child, (IPTR) ScrollgroupObject,
                            MUIA_Scrollgroup_FreeHoriz, FALSE,
                            MUIA_Scrollgroup_FreeVert, TRUE,
                            MUIA_Scrollgroup_Contents, (IPTR) (page = VGroup,
                                MUIA_Group_PageMode, TRUE,
                                ReadListFrame,

                /* each page represents an install time page... you must have one for each enumerated install progress page */

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) (welcomeMsg = FreeCLabel("")),
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) LicenseMsg,
                                        Child, (IPTR) (LicenseMandGrp = HGroup,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) check_license,
                                            Child, (IPTR) LLabel("Accept License Agreement?"),
                                            Child, (IPTR) HVSpace,
                                        End),
                                    End,
                                End,

                                /* Partitioning options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgPartitionOptions),
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (radio_part = RadioObject,
                                            GroupFrame,
                                            MUIA_Radio_Entries, (IPTR) opt_partentries,
                                        End),

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(MUIX_B "Drive:" MUIX_N),
                                        Child, (IPTR) ColGroup(6),
                                            Child, (IPTR) LLabel("Type:"),
                                            Child, (IPTR) (cycle_drivetype =
                                                CycleObject,
                                                MUIA_Cycle_Entries, (IPTR) opt_drivetypes,
                                                MUIA_Cycle_Active, 0,
                                                End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel("Device:"),
                                            Child, (IPTR) (dest_device =
                                                StringObject,
                                                MUIA_String_Contents, (IPTR) boot_Device,
                                                MUIA_String_Reject, " \"\'*",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 300,
                                                End),
                                            Child, (IPTR) LLabel("Unit:"),
                                            Child, (IPTR) (dest_unit =
                                                StringObject,
                                                MUIA_String_Integer, 0,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 20,
                                                End),
                                        End,

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(KMsgDestPartition),
                                        Child, (IPTR) ColGroup(7),
                                            Child, (IPTR) LLabel("Name:"),
                                            Child, (IPTR) (sys_devname = StringObject,
                                                MUIA_String_Contents, SYS_PART_NAME,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel("Filesystem:"),
                                            Child, (IPTR) cycle_fstypesys,
                                            Child, (IPTR) LLabel("Size:"),
                                            Child, (IPTR) (sys_size = StringObject,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) cycle_sysunits,
                                            Child, (IPTR) check_sizesys,
                                            Child, (IPTR) LLabel("Specify Size"),
                                        End,

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(KMsgWorkPartition),
                                        Child, (IPTR) ColGroup(7),
                                            Child, (IPTR) LLabel("Name:"),
                                            Child, (IPTR) (work_devname = StringObject,
                                                MUIA_String_Contents, WORK_PART_NAME,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) check_creatework,
                                            Child, (IPTR) LLabel("Create"),
                                            Child, (IPTR) LLabel("Filesystem:"),
                                            Child, (IPTR) cycle_fstypework,
                                            Child, (IPTR) LLabel("Size:"),
                                            Child, (IPTR) (work_size = StringObject,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) cycle_workunits,
                                            Child, (IPTR) check_sizework,
                                            Child, (IPTR) LLabel("Specify Size"),
                                        End,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgInstallOptions),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) check_locale,
                                            Child, (IPTR) LLabel("Choose Language Options"),
                                            Child, (IPTR) check_core,
                                            Child, (IPTR) LLabel("Install AROS Core System"),
                                            Child, (IPTR) check_extras,
                                            Child, (IPTR) LLabel("Install Extra Software"),
                                            Child, (IPTR) check_dev,
                                            Child, (IPTR) LLabel("Install Development Software"),
                                            Child, (IPTR) check_bootloader,
                                            Child, (IPTR) LLabel("Install Bootloader"),
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgDestOptions),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(KMsgDestVolume),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatsys = ColGroup(2),
                                                Child, (IPTR) check_formatsys,
                                                Child, (IPTR) LLabel("Format Partition"),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (dest_volume = StringObject,
                                            MUIA_String_Contents, (IPTR) dest_Path,
                                            MUIA_Frame, MUIV_Frame_String,
                                            End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) check_work,
                                            Child, (IPTR) LLabel("Use 'Work' Partition"),
                                            Child, (IPTR) check_copytowork,
                                            Child, (IPTR) LLabel("Use 'Work' Partition for Extras and Developer Files"),
                                        End,
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(KMsgWorkVolume),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatwork = ColGroup(2),
                                                Child, (IPTR) check_formatwork,
                                                Child, (IPTR) LLabel("Format Partition"),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (work_volume =
                                            StringObject,
                                            MUIA_String_Contents,
                                            (IPTR) work_Path,
                                            MUIA_Disabled, TRUE,
                                            MUIA_Frame, MUIV_Frame_String,
                                            End),
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                /* Bootloader options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgGrubOptions),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) LLabel(KMsgGrubGOptions),
                                        Child, (IPTR) LLabel(KMsgGrubDrive),
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) ColGroup(5),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel("Device:"),
                                            Child, (IPTR) (grub_device =
                                                StringObject,
                                                MUIA_String_Reject, " \"\'*",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 200,
                                                End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel("Unit:"),
                                            Child, (IPTR) (grub_unit =
                                                StringObject,
                                                MUIA_String_Integer, 0,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 20,
                                                End),
                                        End,

                                        Child, (IPTR) (grub_drive = TextObject, MUIA_Text_PreParse, (IPTR) "" MUIX_C, MUIA_Text_Contents, (IPTR)" ",End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) LLabel(KMsgGrubGrub),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (grub_grub = TextObject, MUIA_Text_PreParse, (IPTR) "" MUIX_C, MUIA_Text_Contents, (IPTR)" ",End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(4),
                                            Child, (IPTR) LLabel("Menu Mode:"),
                                            Child, (IPTR) cycle_grub2mode,
                                            Child, (IPTR) HVSpace,
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgPartitioning),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, gauge1, End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgPartitioning),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, (IPTR) gauge3, End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) (pagetitle = CLabel(" ")),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (pageheader = FreeCLabel(KMsgInstall)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (label = FreeLLabel("YOU SHOULD NOT SEE THIS")),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (currentaction = TextObject,MUIA_Text_Contents,(IPTR)" ",End),
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, gauge2, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                /* Completed page */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        MUIA_Group_SameHeight, FALSE,
                                        Child, (IPTR) (doneMsg = FreeCLabel(KMsgDone)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (reboot_group = ColGroup(2),
                                            MUIA_Weight,0,
                                            MUIA_ShowMe, FALSE,
                                            Child, (IPTR) check_reboot,
                                            Child, (IPTR) LLabel("Reboot AROS now"),
                                        End),
                                    End,
                                End,
                            End),
                        End,
                    End,
                End,

                Child, (IPTR) HGroup,
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) gad_back,
                    Child, (IPTR) gad_proceed,
                    Child, (IPTR) gad_cancel,
                End,
            End),
        End),

    End;

    if (!app)
    {
        D(bug("[INST-APP] Failed to create Installer GUI\n"));
        exit(5);
    }

    /* Update GUI in response to certain user actions */

    /* Notifications on partitioning action */
    DoMethod(radio_part, MUIM_Notify, (IPTR) MUIA_Radio_Active, 0,
        (IPTR) check_sizesys, 3, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(radio_part, MUIM_Notify, (IPTR) MUIA_Radio_Active, 1,
        (IPTR) check_sizesys, 3, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(radio_part, MUIM_Notify, (IPTR) MUIA_Radio_Active, 2,
        (IPTR) check_sizesys, 3, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(radio_part, MUIM_Notify, (IPTR) MUIA_Radio_Active, 2,
        (IPTR) check_sizesys, 3, MUIM_Set, MUIA_Selected, FALSE);

    /* Notifications upon selection of drive type */
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 0,
        (IPTR) dest_device, 3, MUIM_Set,
        MUIA_String_Contents, "ata.device");
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 1,
        (IPTR) dest_device, 3, MUIM_Set,
        MUIA_String_Contents, "usbscsi.device");
    DoMethod(cycle_drivetype, MUIM_Notify, MUIA_Cycle_Active,
        MUIV_EveryTime, (IPTR) dest_unit, 3, MUIM_Set, MUIA_String_Integer,
        0);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 0,
        (IPTR) sys_devname, 3, MUIM_Set, MUIA_String_Contents,
        SYS_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 1,
        (IPTR) sys_devname, 3, MUIM_Set, MUIA_String_Contents,
        USB_SYS_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 0,
        (IPTR) work_devname, 3, MUIM_Set, MUIA_String_Contents,
        WORK_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 1,
        (IPTR) work_devname, 3, MUIM_Set, MUIA_String_Contents,
        USB_WORK_PART_NAME);

    /* Notifications on change of enable status of 'enter size of sys volume'
     * (this tells us if we are using existing partitions) */
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Disabled, MUIV_EveryTime,
        (IPTR) sys_devname, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Disabled, MUIV_EveryTime,
        (IPTR) cycle_drivetype, 3, MUIM_Set,
        MUIA_Disabled, MUIV_TriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Disabled, MUIV_EveryTime,
        (IPTR) dest_device, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Disabled, MUIV_EveryTime,
        (IPTR) dest_unit, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Disabled, MUIV_EveryTime,
        (IPTR) cycle_fstypesys, 3, MUIM_Set,
        MUIA_Disabled, MUIV_TriggerValue);

    /* Notifications on change of selected status of 'enter size of sys volume' */
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_creatework, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) sys_size, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) cycle_sysunits, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_sizesys, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_creatework, 3, MUIM_Set, MUIA_Selected, FALSE);

    /* Notifications on change of selected status of 'create work volume' */
    DoMethod(check_creatework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) work_devname, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_creatework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_sizework, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_creatework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) cycle_fstypework, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_creatework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_sizework, 3, MUIM_Set, MUIA_Selected, FALSE);

    /* Notifications on change of selected status of 'enter size of work volume' */
    DoMethod(check_sizework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) work_size, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_sizework, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) cycle_workunits, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);

#if 0                           /* Notification doesn't seem to work on String gadgets */
    DoMethod(dest_volume, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
        (IPTR) dest_volume, 3, MUIM_WriteString,
        MUIV_TriggerValue, dest_Path);
#endif
    /* Notifications on installing bootloader */
    DoMethod(check_bootloader, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) cycle_grub2mode, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);


    DoMethod(check_core, MUIM_Notify, MUIA_Selected, FALSE,
        (IPTR) check_formatsys, 3, MUIM_Set, MUIA_Selected, FALSE);
    DoMethod(check_work, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_copytowork, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_work, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_copytowork, 3, MUIM_Set, MUIA_Selected, FALSE);
    DoMethod(check_work, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_formatwork, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);
    DoMethod(check_work, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) check_formatwork, 3, MUIM_Set, MUIA_Selected, FALSE);
    DoMethod(check_work, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        (IPTR) work_volume, 3, MUIM_Set,
        MUIA_Disabled, MUIV_NotTriggerValue);

    install_opts->opt_license = check_license;
    install_opts->opt_lic_box = LicenseMsg;
    install_opts->opt_lic_mgrp = LicenseMandGrp;

    install_opts->opt_partmethod = radio_part;

    install_opts->opt_format = check_format;
    install_opts->opt_locale = check_locale;
    install_opts->opt_copycore = check_core;
    install_opts->opt_copyextra = check_extras;
    install_opts->opt_development = check_dev;
    install_opts->opt_bootloader = check_bootloader;

    install_opts->opt_reboot = check_reboot;

    grub_opts->gopt_drive = grub_drive;
    grub_opts->gopt_grub = grub_grub;
    grub_opts->gopt_grub2mode = cycle_grub2mode;

    struct MUI_CustomClass *mcc =
        MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
        sizeof(struct Install_DATA), Install_Dispatcher);
    Object *installer = NewObject(mcc->mcc_Class, NULL,

        MUIA_Page, (IPTR) page,
        MUIA_Gauge1, (IPTR) gauge1,
        MUIA_Gauge2, (IPTR) gauge2,
        MUIA_Install, (IPTR) label,

        MUIA_OBJ_Installer, (IPTR) app,

        MUIA_WelcomeMsg, (IPTR) welcomeMsg,
        MUIA_FinishedMsg, (IPTR) doneMsg,

        MUIA_List_Options, (IPTR) install_opts,
        MUIA_Grub_Options, (IPTR) grub_opts,

        MUIA_OBJ_WindowContent, (IPTR) wndcontents,
        MUIA_OBJ_Window, (IPTR) wnd,

        MUIA_OBJ_PageTitle, (IPTR) pagetitle,
        MUIA_OBJ_PageHeader, (IPTR) pageheader,
        MUIA_OBJ_CActionStrng, (IPTR) currentaction,
        MUIA_OBJ_Back, (IPTR) gad_back,
        MUIA_OBJ_Proceed, (IPTR) gad_proceed,
        MUIA_OBJ_Cancel, (IPTR) gad_cancel,

        MUIA_IC_EnableUndo, TRUE,

        //        MUIA_IC_License_File,"HELP:English/license", /* License can only be viewed by highlighting text and dragging */
        MUIA_IC_License_Mandatory, TRUE,

        TAG_DONE);

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    SET(wnd, MUIA_Window_Open, TRUE);
    {
        ULONG sigs = 0;

        while (DoMethod(app, MUIM_Application_NewInput,
                &sigs) != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
            }
        }
    }

    D(bug("[INST-APP] Closing Window\n"));

    SET(wnd, MUIA_Window_Open, FALSE);

    D(bug("[INST-APP] Disposing of Installer Object\n"));

    DisposeObject(installer);

    D(bug("[INST-APP] Removing Custom Class\n"));

    MUI_DeleteCustomClass(mcc);

    D(bug("[INST-APP] Removing App Object\n"));

    MUI_DisposeObject(app);

    FreeVec(extras_source);
    FreeVec(source_Path);

  main_error:
    return 0;
}
