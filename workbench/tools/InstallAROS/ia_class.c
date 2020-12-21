/*
    Copyright © 2018-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

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
#include "ia_packages.h"
#include "ia_bootloader.h"

extern struct ExpansionBase *ExpansionBase;

extern char *source_Path;       /* full path to source "tree" */
extern char *extras_source;

extern char *dest_Path;         /* DOS DEVICE NAME of part used to store "aros" */
extern char *work_Path;         /* DOS DEVICE NAME of part used to store "work" */

extern char *boot_Device;
extern ULONG boot_Unit;

extern Object *check_copytowork;
extern Object *check_work;
extern Object *show_formatsys;
extern Object *show_formatwork;
extern Object *check_formatsys;
extern Object *check_formatwork;
extern Object *cycle_fstypesys;
extern Object *cycle_fstypework;

extern Object *dest_volume;
extern Object *work_volume;

extern Object *dest_device;
extern Object *cycle_drivetype;
extern Object *dest_unit;

extern Object *check_sizesys;
extern Object *check_sizework;
extern Object *check_creatework;

extern Object *sys_devname;
extern Object *work_devname;

extern Object *grub_device;
extern Object *grub_unit;

extern Object *reboot_group;

TEXT            *extras_path = NULL;       /* DOS DEVICE NAME of part used to store extras */
struct List     SKIPLIST;

AROS_UFH3S(BOOL, partradioHookFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, installer, A2),
    AROS_UFHA(IPTR *, params, A1))
{
    AROS_USERFUNC_INIT

    Class *CLASS = (Class *)h->h_Data;

    D(bug("[InstallAROS] %s()\n", __func__));
    D(bug("[InstallAROS] %s: val = %d\n", __func__, *params));

    switch (*params)
    {
    case 0:
        SET(check_sizesys, MUIA_Disabled, FALSE);
        break;
    case 1:
        SET(check_sizesys, MUIA_Disabled, FALSE);
        break;
    case 2:
        {
            struct Install_DATA *data = INST_DATA(CLASS, installer);
            char *sysName, *workName;
            IPTR option = 0;

            if (data->instc_default_usb)
            {
                sysName = USB_SYS_VOL_NAME;
                workName = USB_WORK_VOL_NAME;
            }
            else
            {
                sysName = SYS_VOL_NAME;
                workName = WORK_VOL_NAME;
            }
            SET(check_sizesys, MUIA_Disabled, TRUE);
            SET(check_sizesys, MUIA_Selected, FALSE);

            /* Get the sys drives device name */
            SET(sys_devname, MUIA_String_Contents, GetDevNameForVolume(sysName));

            /* Get the work drives device name */
            SET(work_devname, MUIA_String_Contents, GetDevNameForVolume(workName));

            break;
        }
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

struct Hook partradioHook =
{
    .h_Entry = partradioHookFunc,
};

IPTR Install__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);

    struct Install_DATA *data = INST_DATA(CLASS, self);
    BPTR lock = BNULL;
    char sys_path[100];

    /* We will generate this info shortly */

    /* IO Related */

    data->IO_Always_overwrite = IIO_Overwrite_Ask;
    data->install_Pattern = "#?";

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

    partradioHook.h_Data = CLASS;
    DoMethod(data->instc_options_main->opt_partmethod, MUIM_Notify, MUIA_Radio_Active, MUIV_EveryTime,
             self, 3, MUIM_CallHook, &partradioHook, MUIV_TriggerValue);
        
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

    SET(data->welcomeMsg, MUIA_Text_Contents, __(MSG_WELCOME));
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
        data->instc_default_usb = TRUE;
        SET(cycle_drivetype, MUIA_Cycle_Active, 2);
    }
    else
        data->instc_default_usb = FALSE;
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
        BPTR from = BNULL;
        LONG s = 0;

        lock = (BPTR) Lock(data->instc_lic_file, SHARED_LOCK);
        if (lock != BNULL)
        {
            fib = (void *)AllocVec(sizeof(*fib), MEMF_PUBLIC);
            Examine(lock, fib);
        }

        if ((from = Open(data->instc_lic_file, MODE_OLDFILE)))
        {
            D(bug
                ("[InstallAROS.i] Allocating buffer [%d] for license file '%s'!",
                    fib->fib_Size, data->instc_lic_file));
            data->instc_lic_buffer =
                AllocVec(fib->fib_Size + 1, MEMF_CLEAR | MEMF_PUBLIC);
            if ((s = Read(from, data->instc_lic_buffer,
                        fib->fib_Size)) == -1)
            {
                D(bug("[InstallAROS.i] Error processing license file!"));
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
        lock = BNULL;
        NEWLIST((struct List *)&data->instc_undorecord);
        D(bug("[InstallAROS.i] Prepared UNDO list @ %p\n",
                &data->instc_undorecord));

        if ((lock = Lock(INSTALLAROS_TMP_PATH, ACCESS_READ)) != BNULL)
        {
            D(bug("[InstallAROS.i] Dir '%s' Exists - no nead to create\n",
                    INSTALLAROS_TMP_PATH));
            UnLock(lock);
        }
        else
        {
            lock = RecursiveCreateDir(INSTALLAROS_TMP_PATH);
            if (lock != NULL)
                UnLock(lock);
            else
            {
                D(bug("[InstallAROS.i] Failed to create dir '%s'!!\n",
                        INSTALLAROS_TMP_PATH));
                data->inst_success = MUIV_Inst_Failed;
                return 0;
            }
        }
    }

    PACKAGES_InitSupport();

    return (IPTR) self;
}

IPTR Install__OM_SET(Class * CLASS, Object * self, struct opSet * message)
{
    return DoSuperMethodA(CLASS, self, (Msg) message);
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
        _(MSG_IOERROR), finaloptions, finalmessage, NULL);
    FreeVec(finalmessage);
    FreeVec(finaloptions);

    return result - 1;
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
        D(bug("[InstallAROS] %s: Drive found [%s unit %d]\n",
                __func__, StartMess->fssm_Device, StartMess->fssm_Unit));

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
                        ("[InstallAROS] %s: First DRIVE found [%s unit %d]...\n",
                            __func__, StartMess->fssm_Device, StartMess->fssm_Unit));
                    founddisk = TRUE;
                    //                    boot_Device = StartMess->fssm_Device;
                    //                    boot_Unit = StartMess->fssm_Unit;
                }

                if (OpenPartitionTable(root) == 0)
                {
                    result = FindPartition(root);
                    D(bug("[InstallAROS] %s: Partition '%s'\n", __func__, result));
                    ClosePartitionTable(root);
                }
                CloseRootPartition(root);
            }
        }
    }

    return (IPTR) result;
}

IPTR Install__MUIM_IC_NextStep(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR this_page = 0, next_stage = 0, option = 0;

    D(bug("[InstallAROS] %s()\n", __func__));

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
        SET(data->welcomeMsg, MUIA_Text_Contents, __(MSG_INSTALLOPTIONS));
        data->instc_stage_next = EDestOptionsStage;
        next_stage = EInstallOptionsStage;
        break;

    case EDestOptionsStage:
        {
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
        }
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

                    GET(sys_devname, MUIA_String_Contents, &option);
                    sprintf(tmp_grub, "%s:"ARCHBOOTDIR"/%s",
                        (CONST_STRPTR) option, BootLoaderData[BootLoaderType].path);

                    /* Guess the best disk to install GRUB's bootblock to */
                    fssm = getDiskFSSM(tmp_grub);
                    if (fssm != NULL)
                    {
                        boot_Device = fssm->fssm_Device;
                        if (strcmp(fssm->fssm_Device, "ahci.device") != 0
                            && strcmp(fssm->fssm_Device, "ata.device") != 0)
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
                MUI_RequestA(data->installer, data->window, 0, _(MSG_ERROR),
                    _(MSG_OK), _(MSG_NOGRUBDEVICE), NULL);
                data->instc_stage_next = EInstallMessageStage;
                next_stage = EGrubOptionsStage;
                data->instc_stage_prev = EInstallOptionsStage;
                return 0;
            }
        }

        if (XGET(check_formatsys, MUIA_Selected)
            || XGET(check_formatwork, MUIA_Selected))
            SET(data->welcomeMsg, MUIA_Text_Contents,
                __(MSG_BEGINWITHPARTITION));
        else
            SET(data->welcomeMsg, MUIA_Text_Contents,
                __(MSG_BEGINWITHOUTPARTITION));
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
                TEXT msg[strlen(_(MSG_PARTITIONTOOBIG)) + 40];
                sprintf(msg, _(MSG_PARTITIONTOOBIG),
                    MAX_SFS_SIZE / 1024, MAX_SFS_SIZE,
                    MAX_FFS_SIZE / 1024, MAX_FFS_SIZE);
                MUI_RequestA(data->installer, data->window, 0, _(MSG_ERROR),
                    _(MSG_OK), msg, NULL);
                return 0;
            }

            BOOTLOADER_PartFixUp(data, systype);
        }
        data->disable_back = TRUE;

        SET(data->page, MUIA_Group_ActivePage, EPartitioningStage);

        switch (option)
        {
        case 0:
        case 1:
            if (DoMethod(self, MUIM_Partition) != RETURN_OK)
            {
                D(bug("[InstallAROS] %s: Partitioning FAILED!!!!\n", __func__));
                data->disable_back = FALSE;
                SET(data->page, MUIA_Group_ActivePage,
                    EInstallMessageStage);
                data->instc_stage_next = EPartitioningStage;
                data->instc_options_main->partitioned = FALSE;
                MUI_RequestA(data->installer, data->window, 0, _(MSG_ERROR),
                    _(MSG_QUIT), _(MSG_PARTITIONINGFAILED), NULL);
                DoMethod(self, MUIM_IC_QuitInstall);
                return 0;
            }
            data->instc_options_main->partitioned = TRUE;
            next_stage = EDoneStage;
            DoMethod(data->page, MUIM_Group_InitChange);
            if (XGET(cycle_drivetype, MUIA_Cycle_Active) != 2)
                SET(data->doneMsg, MUIA_Text_Contents, __(MSG_DONEREBOOT));
            else
                SET(data->doneMsg, MUIA_Text_Contents, __(MSG_DONEUSB));
            SET(reboot_group, MUIA_ShowMe, TRUE);
            if (XGET(cycle_drivetype, MUIA_Cycle_Active) != 2)
                SET(data->instc_options_main->opt_reboot, MUIA_Selected,
                    TRUE);
            DoMethod(data->page, MUIM_Group_ExitChange);
            SET(data->back, MUIA_Disabled, TRUE);
            SET(data->cancel, MUIA_Disabled, TRUE);
            data->instc_stage_next = EDoneStage;
            break;
        case 2:
            {
                TEXT nametmp[100];
                char *optname = NULL, *volname;
                GET(data->instc_options_main->opt_sysdevname, MUIA_String_Contents, (IPTR *)&optname);
                if (!GetVolumeForDevName(optname, nametmp))
                {
                    if (data->instc_default_usb)
                        volname = USB_SYS_VOL_NAME;
                    else
                        volname = SYS_VOL_NAME;
                }
                else
                    volname = nametmp;
                SET(dest_volume, MUIA_String_Contents, volname);

                GET(data->instc_options_main->opt_workdevname, MUIA_String_Contents, (IPTR *)&optname);
                if (!GetVolumeForDevName(optname, nametmp))
                {
                    if (data->instc_default_usb)
                        volname = USB_SYS_VOL_NAME;
                    else 
                        volname = WORK_VOL_NAME;
                }
                else
                    volname = nametmp;
                SET(work_volume, MUIA_String_Contents, volname);

                data->disable_back = FALSE;
                data->instc_options_main->partitioned = TRUE;
                data->instc_stage_next = EDestOptionsStage;
                next_stage = EInstallOptionsStage;
                break;
            }
        default:
            D(bug("[InstallAROS] %s: Launching QuickPart...\n", __func__));
            Execute("SYS:Tools/QuickPart", NULL, NULL);
            break;
        }
        break;

    case EInstallStage:
        data->disable_back = TRUE;
        SET(data->page, MUIA_Group_ActivePage, EInstallStage);

        /* Set the paths used for the actual install */
        GET(sys_devname, MUIA_String_Contents, &data->install_SysTarget);
        GET(work_devname, MUIA_String_Contents, &data->install_WorkTarget);

        /* Set the bootloader device and unit */
        GET(grub_device, MUIA_String_Contents, &data->bl_TargetDevice);
        data->bl_TargetUnit = XGET(grub_unit, MUIA_String_Integer);

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
                __(MSG_BEGINWITHPARTITION));
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
        SET(data->welcomeMsg, MUIA_Text_Contents, __(MSG_WELCOME));
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
        cancelmessage = _(MSG_CANCELDANGER);
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
        GET(data->instc_options_main->opt_developer, MUIA_Disabled,
            &backupOptions->opt_developer);
        GET(data->instc_options_main->opt_bootloader, MUIA_Disabled,
            &backupOptions->opt_bootloader);
        GET(data->instc_options_main->opt_reboot, MUIA_Disabled,
            &backupOptions->opt_reboot);

        SET(data->instc_options_main->opt_format, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_locale, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_copycore, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_copyextra, MUIA_Disabled, TRUE);
        SET(data->instc_options_main->opt_developer, MUIA_Disabled, TRUE);
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
        cancelmessage = _(MSG_CANCELOK);
        break;
    }

    SET(data->back, MUIA_Selected, FALSE);
    SET(data->back, MUIA_Disabled, TRUE);

    SET(data->proceed, MUIA_Selected, FALSE);
    SET(data->proceed, MUIA_Disabled, TRUE);

    SET(data->cancel, MUIA_Selected, FALSE);
    SET(data->cancel, MUIA_Disabled, TRUE);

    if (!MUI_RequestA(data->installer, data->window, 0,
            _(MSG_CANCELINSTALL), _(MSG_CONTINUECANCELINST),
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
        SET(data->instc_options_main->opt_developer, MUIA_Disabled,
            (BOOL) backupOptions->opt_developer);
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
            D(bug("[InstallAROS] %s: Partitioning EVERYTHING! MUAHAHAHA...\n", __func__));
            strcat(tmpcmd, " WIPE");
        }
        D(else
                bug("[InstallAROS] %s: Partitioning Free Space...\n", __func__);
        )

        D(bug("[InstallAROS] %s: ### Executing '%s'\n", __func__, &tmpcmd));
        tmp = SystemTagList(tmpcmd, NULL);

        SET(data->proceed, MUIA_Disabled, FALSE);
    }

    return tmp;
}

IPTR Install__MUIM_IC_CopyFiles
    (Class * CLASS, Object * self, struct MUIP_CopyFiles * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    LONG totalFiles = -1, totalFilesCopied = 0;

    D(bug("[InstallAROS] %s: Entry, src: %s, dst: %s, mask: %s\n",
            __func__, message->srcDir, message->dstDir, message->fileMask));

    /* Check entry condition */
    if (data->inst_success != MUIV_Inst_InProgress)
    {
        D(bug("[InstallAROS] %s: Installation failed\n", __func__));
        return totalFilesCopied;
    }

    SET(data->gauge2, MUIA_Gauge_Current, 0);

    /* Get file count */
    do
    {
        totalFiles =
            CountFiles(message->srcDir, message->skipList, message->fileMask,
            message->recursive);
        D(bug("[InstallAROS] %s: Found %ld files in %s\n", __func__, totalFiles,
                message->srcDir));

        if (totalFiles < 0)
        {
            ULONG retry =
                AskRetry(CLASS, self, _(MSG_SCANRETRY),
                    message->srcDir, _(MSG_YES), _(MSG_SKIP), _(MSG_QUIT));

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
        InternalCopyFiles(CLASS, self, message->srcDir, message->dstDir, message->skipList,
        message->fileMask, message->recursive, totalFiles,
        totalFilesCopied);

    /* Check final condition */
    if ((data->inst_success == MUIV_Inst_InProgress)
        && (totalFiles != totalFilesCopied))
    {
        TEXT msg[strlen(_(MSG_NOTALLFILESCOPIED)) +
                strlen(message->srcDir) + 3];
        sprintf(msg, _(MSG_NOTALLFILESCOPIED), message->srcDir);

        if (MUI_RequestA(data->installer, data->window, 0, _(MSG_ERROR),
                _(MSG_CONTINUEQUIT), msg, NULL) != 1)
            DoMethod(self, MUIM_IC_QuitInstall);

        return totalFilesCopied;
    }

    return totalFilesCopied;
}

IPTR Install__MUIM_IC_SetLocalePrefs(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    BPTR lock = BNULL;

    D(bug("[InstallAROS] %s: Launching Locale Prefs...\n", __func__));

    ULONG srcLen = strlen(source_Path), dstLen =
        (strlen(data->install_SysTarget) + 1);
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

    sprintf(envDstDir, "%s:", data->install_SysTarget);
    sprintf(localeFile, "\"%s", source_Path);
    CopyMem(prefssrc_path, localesrcPFile, envsrcLen + 1);
    sprintf(localePFile, "%s:", data->install_SysTarget);
    sprintf(inputFile, "\"%s", source_Path);
    CopyMem(prefssrc_path, inputsrcPFile, envsrcLen + 1);
    sprintf(inputPFile, "%s:", data->install_SysTarget);

    AddPart(localeFile, inputFile_path, localeFileLen);

    AddPart(localesrcPFile, locale_prfs_file, localePFileLen);

    AddPart(localePFile, prefs_path, localePFileLen);
    AddPart(localePFile, locale_prfs_file, localePFileLen);

    AddPart(inputFile, localeFile_path, inputFileLen);

    AddPart(inputsrcPFile, input_prfs_file, inputPFileLen);

    AddPart(inputPFile, prefs_path, inputPFileLen);
    AddPart(inputPFile, input_prfs_file, inputPFileLen);

    D(bug("[InstallAROS] %s: Excecuting '%s'...\n", __func__, localeFile));

    Execute(localeFile, NULL, NULL);

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    D(bug("[InstallAROS] %s: Excecuting '%s'...\n", __func__, inputFile));

    Execute(inputFile, NULL, NULL);

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    D(bug("[InstallAROS] %s: Copying Locale Settings...\n", __func__));

    //create the dirs "Prefs","Prefs/Env-Archive" and "Prefs/Env-Archive/SYS"
    AddPart(envDstDir, "Prefs", dstLen + envdstLen);
    AddPart(envDstDir, "Env-Archive", envdstdirLen);
    AddPart(envDstDir, "SYS", envdstdirLen);
    D(bug("[InstallAROS] %s: Create Dir '%s' \n", __func__, envDstDir));
    if ((lock = Lock(envDstDir, ACCESS_READ)) != BNULL)
    {
        D(bug("[InstallAROS] %s: Dir '%s' Exists - no nead to create\n",
                __func__, envDstDir));
        UnLock(lock);
    }
    else
    {
        lock = RecursiveCreateDir(envDstDir);
        if (lock != NULL)
            UnLock(lock);
        else
        {
          createdirfaild:
            D(bug("[InstallAROS] %s: Failed to create %s dir!!\n",
                    envDstDir, __func__));
            /* TODO: Should prompt on failure to try again/continue anyhow/exit */
            goto localecopydone;
            //data->inst_success = MUIV_Inst_Failed;
            //return 0;
        }
    }
    
    lock = BNULL;

    D(bug("[InstallAROS] %s: Copying files\n", __func__));

    if ((lock = Lock(localesrcPFile, ACCESS_READ)) != BNULL)
    {
        UnLock(lock);
        DoMethod(self, MUIM_IC_CopyFile, localesrcPFile, localePFile);
    }

    lock = BNULL;

    if ((lock = Lock(inputsrcPFile, ACCESS_READ)) != BNULL)
    {
        UnLock(lock);
        DoMethod(self, MUIM_IC_CopyFile, inputsrcPFile, inputPFile);
    }
  localecopydone:
    ;
}

IPTR Install__MUIM_IC_Install(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    BPTR lock = BNULL;
    IPTR option = FALSE;

    NEWLIST(&SKIPLIST);

    D(
        bug("[InstallAROS] %s: Dest Sys Path = '%s'\n", __func__, data->install_SysTarget);
        bug("[InstallAROS] %s: Dest Work Path = '%s'\n", __func__, data->install_WorkTarget);
    )

    SET(data->back, MUIA_Disabled, TRUE);
    SET(data->proceed, MUIA_Disabled, TRUE);

    SET(data->pagetitle, MUIA_Text_Contents, __(MSG_INSTALLING));

    /* set up destination Work name to use */

    GET(check_copytowork, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
        extras_path = data->install_WorkTarget;
    else
        extras_path = data->install_SysTarget;

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
        sprintf(tmp, "%s:", data->install_WorkTarget);
        D(bug
            ("[InstallAROS] %s: Checking validity of work partition '%s' ..", __func__, tmp));
        if ((lock = Lock(tmp, SHARED_LOCK)) != BNULL)    /* check the dest dir exists */
        {
            D(bug("OK!\n"));
            UnLock(lock);
        }
        else
        {
            D(bug
                ("FAILED!\n[InstallAROS] %s: (Warning) INSTALL - Failed to locate chosen work partition : defaulting to '%s'\n",
                    __func__, data->install_SysTarget));
            extras_path = data->install_SysTarget;
        }
        lock = BNULL;
    }
    else
    {
        D(bug("[InstallAROS] %s: Using SYS partition only (%s)\n",
                __func__, data->install_SysTarget));
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : LOCALE */

    GET(data->instc_options_main->opt_locale, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        DoMethod(self, MUIM_IC_SetLocalePrefs);
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : COPY CORE */

    GET(data->instc_options_main->opt_copycore, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        CONST_STRPTR core_dirs[] = {
            "boot", "boot",
            "C", "C",
            "Classes", "Classes",
            "Devs", "Devs",
            "Fonts", "Fonts",
            "L", "L",
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
        ULONG dstLen = strlen(data->install_SysTarget) + strlen(AROS_BOOT_FILE) + 2;
        ULONG skipPathLen = strlen(source_Path) + 100;
        TEXT destinationPath[dstLen];
        TEXT skipPath[skipPathLen];
        TEXT tmp[100];
        BOOL success = FALSE;

        /* Copying Core system Files */
        D(bug("[InstallAROS] %s: Copying Core files...\n", __func__));
        SET(data->label, MUIA_Text_Contents, __(MSG_COPYCORE));
        sprintf(destinationPath, "%s:", data->install_SysTarget);

        /* Make sure the EXTRASPATH/DEVELPATH vars arent copied */
        strcpy(skipPath, source_Path);
        AddPart(skipPath, "Prefs/Env-Archive/EXTRASPATH", skipPathLen);
        AddSkipListEntry(&SKIPLIST, skipPath);
        strcpy(skipPath, source_Path);
        AddPart(skipPath, "Prefs/Env-Archive/DEVELPATH", skipPathLen);
        AddSkipListEntry(&SKIPLIST, skipPath);

        /* Remove paths handled by other stages of the install */
        BOOTLOADER_AddCoreSkipPaths(&SKIPLIST);
        PACKAGES_AddCoreSkipPaths(&SKIPLIST);

        /* Copy the core AROS system files ... */
        CopyDirArray(CLASS, self, source_Path, destinationPath, core_dirs, &SKIPLIST);

        /* Copy AROS.boot file ... */
        sprintf(tmp, "%s", source_Path);
        sprintf(destinationPath, "%s:" AROS_BOOT_FILE, data->install_SysTarget);
        AddPart(tmp, AROS_BOOT_FILE, 100);
        DoMethod(self, MUIM_IC_CopyFile, tmp, destinationPath);

        /* Make Env-Archive Writeable */
        sprintf(tmp, "Protect ADD FLAGS=W ALL QUIET %s:Prefs/Env-Archive",
            data->install_SysTarget);
        D(bug
            ("[InstallAROS] %s: Changing Protection on Env Files (command='%s')\n",
                __func__, tmp));
        success = (BOOL) Execute(tmp, NULL, NULL);

        if (!success)
        {
            D(bug
                ("[InstallAROS] %s: Changing Protection on Env Files failed: %d\n",
                    __func__, IoErr()));
        }
        ClearSkipList(&SKIPLIST);
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
        D(bug("[InstallAROS] %s: Copying Extras to '%s'...\n", __func__, extras_path));
        SET(data->label, MUIA_Text_Contents, __(MSG_COPYEXTRA));
        sprintf(extraspath, "%s:", extras_path);
        CopyDirArray(CLASS, self, extras_source, extraspath, extras_dirs, &SKIPLIST);

        /* Set EXTRASPATH environment variable */
        AddPart(extraspath, "Extras", 100);
        create_environment_variable(data->install_SysTarget, "EXTRASPATH", extraspath);

        /* Restore undo state */
        data->instc_copt_undoenabled = undoenabled;
        ClearSkipList(&SKIPLIST);
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : COPY DEVELOPER FILES */

    GET(data->instc_options_main->opt_developer, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        ULONG srcLen = strlen(source_Path);
        ULONG developerDirLen = srcLen + strlen("Developer") + 2;
        TEXT developerDir[srcLen + developerDirLen];

        CopyMem(source_Path, &developerDir, srcLen + 1);
        AddPart(developerDir, "Developer", srcLen + developerDirLen);

        if ((lock = Lock(developerDir, SHARED_LOCK)) != BNULL)
        {
            CONST_STRPTR developer_dirs[] = {
                "Developer", "Developer",
                NULL
            };
            TEXT developerpath[100];
            BOOL undoenabled = data->instc_copt_undoenabled;

            /* Explicitly disable undo. Some users might not have RAM for backup */
            data->instc_copt_undoenabled = FALSE;

            UnLock(lock);

            /* Copying Developer stuff */
            D(bug("[InstallAROS] %s: Copying Developer Files...\n", __func__));
            SET(data->label, MUIA_Text_Contents, __(MSG_COPYDEVEL));
            sprintf(developerpath, "%s:", extras_path);
            CopyDirArray(CLASS, self, source_Path, developerpath,
                developer_dirs, &SKIPLIST);

            /* Set DEVELPATH environment variable */
            AddPart(developerpath, "Developer", 100);
            create_environment_variable(data->install_SysTarget, "DEVELPATH",
                developerpath);

            /* Set Developer package var */
            create_environment_variable(data->install_SysTarget, "SYS/Packages/Developer",
                developerpath);

            /* Restore undo state */
            data->instc_copt_undoenabled = undoenabled;
        }
        D(else
           bug("[InstallAROS] %s: Couldn't locate Developer Files...\n", __func__);
        )

        ClearSkipList(&SKIPLIST);
    }

    DoMethod(data->installer, MUIM_Application_InputBuffered);

    /* STEP : INSTALL BOOTLOADER */

    GET(data->instc_options_main->opt_bootloader, MUIA_Selected, &option);
    if (option && (data->inst_success == MUIV_Inst_InProgress))
    {
        BOOTLOADER_DoInstall(CLASS, self);
    }

    SET(data->proceed, MUIA_Disabled, FALSE);

    /* STEP : CREATE ENVIRONMENT VARIABLES THAT POINT TO INSTALLATION LOCATIONS */
    {
        TEXT varval[255];
        IPTR optcheck = 0;

        /* Volume name of installed SYS */
        sprintf(varval, "%s:", data->install_SysTarget);
        SetVar("INSTALLEDSYS", varval, strlen(varval), GVF_GLOBAL_ONLY);

        /* Volume name of installed WORK */
        GET(check_work, MUIA_Selected, &optcheck);
        if (optcheck)
        {
            sprintf(varval, "%s:", data->install_WorkTarget);
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

            D(bug("[InstallAROS] Running post-install script...\n");)
            SET(data->label, MUIA_Text_Contents, __(MSG_POSTINSTALL));
            SET(data->pageheader, MUIA_Text_Contents, __(MSG_POSTINSTALL2));
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

            D(bug("[InstallAROS] execute: %s\n", POST_INSTALL_SCRIPT);)

            SystemTagList("", tags);

            /* Do not close scriptfile. It was closed by SystemTagList */

            SET(data->gauge2, MUIA_Gauge_Current, 100);
        }
        D(else
                bug("[InstallAROS] no post-install script\n");
        )
    }

    /* STEP : UNDORECORD CLEANUP */

    D(bug
        ("[InstallAROS] Reached end of Install Function - cleaning up undo logs @ %p...\n",
            &data->instc_undorecord);)

    struct InstallC_UndoRecord *CurUndoNode = NULL;
    struct Node *undonode_tmp = NULL;

    ForeachNodeSafe(&data->instc_undorecord, CurUndoNode, undonode_tmp)
    {
        D(bug("[InstallAROS] Removing undo record @ %p\n", CurUndoNode);)
        Remove((struct Node *)CurUndoNode);

        switch (CurUndoNode->undo_method)
        {
        case MUIM_IC_CopyFile:
            D(bug("[InstallAROS] Deleting undo file '%s'\n",
                    CurUndoNode->undo_src);)
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

IPTR Install__MUIM_Format(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    char *dev_nametmp[100];
    char vol_nametmp[100];
    char fmt_nametmp[100];
    char *name_tmp = NULL;
    BOOL success = FALSE;
    IPTR option = FALSE;
    BPTR lock = BNULL;
    char tmp[100];

    if ((BOOL) XGET(check_formatsys, MUIA_Selected))
    {
        /* Format Vol0 */
        GET(sys_devname, MUIA_String_Contents, &name_tmp);
        sprintf(dev_nametmp, "%s:", name_tmp);

        sprintf(fmt_nametmp, _(MSG_FORMATTING), dev_nametmp);
        D(bug("[InstallAROS] %s\n", fmt_nametmp);)
        SET(data->label, MUIA_Text_Contents, fmt_nametmp);
        SET(data->gauge2, MUIA_Gauge_Current, 0);

        GET(dest_volume, MUIA_String_Contents, &name_tmp);
        strcpy(vol_nametmp, name_tmp);

        /* XXX HACK
         * If partition is FFS -> it will format it for FFS
         * If partition is SFS -> it will format it for SFS
         * Correct way of doing things: read type for DH0 and DH1, apply correct
         * type when formatting
         */
        D(bug("[InstallAROS] (info) Using FormatPartition\n");)
        success =
            FormatPartition(dev_nametmp, vol_nametmp, ID_INTER_FFS_DISK);

        if (success)
            set(data->gauge2, MUIA_Gauge_Current, 100);
    }

    GET(check_work, MUIA_Selected, &option);
    if (option && XGET(check_formatwork, MUIA_Selected))
    {
        /* Format Vol1 */
        GET(work_devname, MUIA_String_Contents, &name_tmp);
        sprintf(dev_nametmp, "%s:", name_tmp);

        sprintf(fmt_nametmp, _(MSG_FORMATTING), dev_nametmp);
        D(bug("[InstallAROS] %s\n", fmt_nametmp);)
        SET(data->label, MUIA_Text_Contents, fmt_nametmp);
        SET(data->gauge2, MUIA_Gauge_Current, 0);

        GET(work_volume, MUIA_String_Contents, &name_tmp);
        strcpy(vol_nametmp, name_tmp);

        /* XXX HACK
         * If partition is FFS -> it will format it for FFS
         * If partition is SFS -> it will format it for SFS
         * Correct way of doing things: read type for DH0 and DH1, apply
         * correct type when formatting (ID_INTER_FFS_DISK or ID_SFS_BE_DISK)
         */
        D(bug("[InstallAROS] (info) Using FormatPartition\n");)
        success =
            FormatPartition(dev_nametmp, vol_nametmp, ID_INTER_FFS_DISK);

        if (success)
        {
            SET(data->gauge2, MUIA_Gauge_Current, 100);
            lock = Lock(dev_nametmp, SHARED_LOCK);      /* check the dest dir exists */
            if (lock == BNULL)
            {
                bug
                    ("[InstallAROS] (Warning) FORMAT: Failed for chosen work partition '%s' : defaulting to sys only\n",
                        dev_nametmp);
                extras_path = dest_Path;
            }
            else
            {
                UnLock(lock);
                lock = BNULL;
            }
        }
    }
    if (success)
        SET(data->gauge2, MUIA_Gauge_Current, 100);

    return success;
}

IPTR Install__MUIM_IC_CopyFile
    (Class * CLASS, Object * self, struct MUIP_CopyFile * message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    static TEXT buffer[kBufSize];
    struct InstallC_UndoRecord *undorecord = NULL;
    ULONG retry = 0;
    ULONG filescopied = 0;
    BPTR from = BNULL, to = BNULL;

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
                    _(MSG_EXISTSRETRY), message->dstFile,
                    _(MSG_YES), _(MSG_YESALWAYS), _(MSG_NO));
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
                    D(bug("[InstallAROS.CF] Failed to read: %s [ioerr=%d]\n",
                            message->srcFile, IoErr()));

                    Close(to);
                    Close(from);

                    retry =
                        AskRetry(CLASS, self, _(MSG_COULDNTOPEN),
                        message->srcFile, _(MSG_RETRY), _(MSG_SKIP), _(MSG_CANCEL));
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
                        ("[InstallAROS.CF] Failed to write: %s  [%d bytes, ioerr=%d]\n",
                            message->dstFile, s, IoErr()));

                    if (IoErr() == 103)
                        retry =
                            AskRetry(CLASS, self,
                            _(MSG_DISKFULL),
                            message->dstFile, _(MSG_RETRY), _(MSG_SKIP), _(MSG_CANCEL));
                    else
                        retry =
                            AskRetry(CLASS, self, _(MSG_COULDNTWRITE),
                            message->dstFile, _(MSG_RETRY), _(MSG_SKIP), _(MSG_CANCEL));

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
                ("[InstallAROS.CF] Failed to open '%s' for writing [ioerr=%d]\n",
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
                    ("[InstallAROS.CF] Adding undo record @ %x to undo list @ %x \n",
                        undorecord, &data->instc_undorecord));
                AddHead(&data->instc_undorecord, (struct Node *)undorecord);
            }
            else
            {
                D(bug("[InstallAROS.CF] Freeing undo record\n"));
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
        D(bug("[InstallAROS.CF] Failed to open: %s [ioerr=%d]\n",
                message->srcFile, IoErr()));
        data->inst_success = MUIV_Inst_Failed;
    }

    return filescopied;
}

IPTR Install__MUIM_IC_UndoSteps(Class * CLASS, Object * self, Msg message)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    struct InstallC_UndoRecord *CurUndoNode = NULL;

    D(bug("[InstallAROS.US] Performing UNDO steps...\n"));

    /* Disbale "UNDO" mode to prevent new records */
    data->instc_copt_undoenabled = FALSE;

    ForeachNode(&data->instc_undorecord, CurUndoNode)
    {
        D(bug("[InstallAROS.US] Removing undo record @ %x\n", CurUndoNode));
        Remove((struct Node *)CurUndoNode);

        switch (CurUndoNode->undo_method)
        {
        case MUIM_IC_CopyFile:
            D(bug("[InstallAROS.US] Reverting file '%s'\n",
                    CurUndoNode->undo_dst));

            DoMethod(self, CurUndoNode->undo_method, CurUndoNode->undo_src,
                CurUndoNode->undo_dst);

            D(bug("[InstallAROS.US] Deleting undo file '%s'\n",
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

    D(bug("[InstallAROS.US] UNDO complete\n"));

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
        D(bug("[InstallAROS] Cold rebooting...\n"));
        ShutdownA(SD_ACTION_COLDREBOOT);
    }
    else
    {
        D(bug("[InstallAROS] Install Finished [no reboot]...\n"));
        if (data->inst_success == MUIV_Inst_InProgress)
            data->inst_success = MUIV_Inst_Completed;
        SET(data->window, MUIA_Window_CloseRequest, TRUE);
    }

    return TRUE;                /* Keep the compiler happy... */
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

    case MUIM_IC_SetLocalePrefs:
        return Install__MUIM_IC_SetLocalePrefs(CLASS, self, message);

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
BOOPSI_DISPATCHER_END
