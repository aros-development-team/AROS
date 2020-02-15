#ifndef IA_INSTALL_H
#define IA_INSTALL_H
/*
    Copyright © 2003-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include	<exec/lists.h>
#include	<exec/nodes.h>

#include <libraries/mui.h>

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

#define localeFile_path         "Prefs/Locale\""  /* please note the suffixed \" */
#define inputFile_path          "Prefs/Input\""
#define prefssrc_path           "ENV:SYS"
#define prefs_path              "Prefs/Env-Archive/SYS"

#define locale_prfs_file        "locale.prefs"
#define input_prfs_file        "input.prefs"

#define DEF_INSTALL_IMAGE       "IMAGES:Logos/install.logo"
#define DEF_BACK_IMAGE          "IMAGES:Logos/install.logo"
#define DEF_LIGHTBACK_IMAGE     "IMAGES:Logos/install.logo"

#define POST_INSTALL_SCRIPT     "PROGDIR:InstallAROS-Post-Install"
#define AROS_BOOT_FILE          "AROS.boot"

#if 0
#define OPTION_PREPDRIVES           1
#define OPTION_FORMAT               2
#define OPTION_LANGUAGE             3
#define OPTION_CORE                 4
#define OPTION_EXTRAS               5
#define OPTION_BOOTLOADER           6
#endif

#define INSTV_TITLE                 101001
#define INSTV_LOGO                  101002
#define INSTV_PAGE                  101003

#define INSTV_TEXT                  101004
#define INSTV_SPACE                 101005
#define INSTV_BOOL                  101006
#define INSTV_RETURN                101007

#define INSTV_CURR                  101100

/* Unused strings. Kept for reference */

/*

static const char KMsgLanguage[] =
"The system prefs for your language settings\n"
"will now launch\n\n"
"Choose the settings that are relevant to you\n";

static const char KMsgNoDrives[] =
"It appears you do not have a hard\n"
"drive installed in your PC.\n\n"
"Installation of AROS can only be performed on to\n"
"a hard drive just now. Sorry.\n\nPress Proceed To Exit\n";

*/

#define MUIM_IC_NextStep                (0x00335551)
#define MUIM_IC_PrevStep                (0x00335552)
#define MUIM_IC_UndoSteps               (0x00335553)

#define MUIM_IC_Install                 (0x00335554)

#define MUIM_FindDrives                 (0x00335555)
#define MUIM_PartitionFree              (0x00335556)
#define MUIM_Partition                  (0x00335557)
#define MUIM_Format                     (0x00335558)

#define MUIM_IC_CopyFiles               (0x0033555a)
#define MUIM_IC_CopyFile                (0x0033555b)

#define MUIM_IC_CancelInstall           (0x0033556a)
#define MUIM_IC_ContinueInstall         (0x0033556b)
#define MUIM_IC_QuitInstall             (0x0033556c)

#define MUIM_Reboot                     (0x0033556d)
#define MUIM_RefreshWindow              (0x00335570)

/* to be made obsolete */

#define MUIA_Page                       (0x00335580)

#define MUIA_PartitionButton            (0x00335581)
#define MUIA_Gauge1                     (0x00335582)
#define MUIA_Gauge2                     (0x00335583)
#define MUIA_Install                    (0x00335584)
/**/
#define MUIA_WelcomeMsg                 (0x0033558a)
#define MUIA_FinishedMsg                (0x0033558b)

/* new - some/most will "vanish(tm)" */

#define MUIA_OBJ_Installer              (0x0033558d)
#define MUIA_Grub_Options               (0x0033558e)
#define MUIA_List_Options               (0x0033558f)

#define MUIA_OBJ_Window                 (0x00335590)
#define MUIA_OBJ_WindowContent          (0x00335591)

#define MUIA_OBJ_PageTitle              (0x00335592)
#define MUIA_OBJ_PageHeader             (0x00335593)
#define MUIA_OBJ_CActionStrng           (0x00335594)

#define MUIA_OBJ_Back                   (0x00335595)
#define MUIA_OBJ_Proceed                (0x00335596)
#define MUIA_OBJ_Cancel                 (0x00335597)

#define MUIA_IC_License_File            (0x00335598)
#define MUIA_IC_License_Mandatory	(0x00335599)

#define MUIA_IC_EnableUndo              (0x00335599)

/* Install Results */

#define MUIA_InstallComplete            (0x003355ff)
#define MUIA_InstallFailed              (0x003355fe)

#define MUIV_Inst_Completed             (0xff)
#define MUIV_Inst_InProgress            (0x00)
#define MUIV_Inst_Cancelled             (0x01)
#define MUIV_Inst_Failed                (0x10)

struct MUIP_CopyFiles
{
    STACKED ULONG               MethodID;
    STACKED CONST_STRPTR        srcDir;
    STACKED CONST_STRPTR        dstDir;
    STACKED struct List         *skipList;
    STACKED CONST_STRPTR        fileMask;
    STACKED ULONG               recursive;
};

struct MUIP_CopyFile
{
    STACKED ULONG               MethodID;
    STACKED CONST_STRPTR        srcFile;
    STACKED CONST_STRPTR        dstFile;
};

enum EStage
{
    EMessageStage,
    ELicenseStage,
    EPartitionOptionsStage,
    EInstallOptionsStage,
    EDestOptionsStage,
    EGrubOptionsStage,
    EInstallMessageStage,
    EPartitioningStage,
    EInstallStage,
    EDoneStage
};

struct Install_Options
{
    Object	                *opt_license;
    Object	                *opt_lic_box;
    Object	                *opt_lic_mgrp;

    Object	                *opt_partmethod;
    Object	                *opt_sysdevname;
    Object	                *opt_workdevname;

    Object	                *opt_format;
    
    Object	                *opt_locale;
    Object	                *opt_copycore;
    Object	                *opt_copyextra;
    Object	                *opt_developer;
    Object	                *opt_bootloader;

    Object	                *opt_reboot;

    BOOL	                partitioned;
    BOOL	                bootloaded;
};

struct Grub_Options
{
    Object                      *gopt_drive;
    Object                      *gopt_grub;
    Object                      *gopt_grub2mode;
    BOOL                        bootinfo;
};

struct InstallC_UndoRecord
{
    struct Node                 undo_Node;      /* Inherits from struct Node */
    ULONG                       undo_method;
    char                        *undo_src;
    char                        *undo_dst;
};

struct optionstmp
{
    IPTR	                opt_format;
    IPTR	                opt_locale;
    IPTR	                opt_copycore;
    IPTR	                opt_copyextra;
    IPTR	                opt_developer;
    IPTR	                opt_bootloader;
    IPTR	                opt_reboot;
};

enum IO_OVERWRITE_FLAGS
{
    IIO_Overwrite_Ask,
    IIO_Overwrite_Always,
    IIO_Overwrite_Never
};

struct Install_DATA
{
    /* Source and Destination Paths used in the Install stage */
    char                        *install_Source;
    char                        *install_SysTarget;
    char                        *install_WorkTarget;
    /* Bootloader Target used in the Install stage */
    char                        *bl_TargetDevice;
    ULONG                       bl_TargetUnit;
    /* File copy pattern mask */
    char                        *install_Pattern;

    /* general objects */
    Object                      *partition;
    Object                      *page;
    Object                      *gauge1;
    Object                      *gauge2;
    Object                      *label;
    /**/
    Object                      *installer;

    Object                      *welcomeMsg;
    Object                      *doneMsg;
    
    /**/
    Object                      *window;
    Object                      *contents;

    Object                      *pagetitle;
    Object                      *pageheader;
    Object                      *actioncurrent;

    Object                      *back;
    Object                      *proceed;
    Object                      *cancel;

    /**/
    ULONG                       inst_success;

    ULONG                       disable_back,
                                status_back,
                                status_proceed,
                                status_cancel;

    ULONG                       cur_width,
                                cur_height;

    char                        *instc_lic_file;
    APTR			instc_lic_buffer;

    struct List     		instc_undorecord;

    /** Option Related **/

    struct Install_Options      *instc_options_main;
    struct Grub_Options		*instc_options_grub;
    struct optionstmp		*instc_options_backup;
    enum EStage                 instc_stage_prev;
    enum EStage                 instc_stage_next;

    enum IO_OVERWRITE_FLAGS     IO_Always_overwrite;

    BOOL                        instc_default_usb;
    BOOL                        instc_cflag_driveset;
    BOOL			instc_copt_undoenabled;
    BOOL			instc_copt_licensemandatory;
};

extern void AddSkipListEntry(struct List *SkipList, char *SkipEntry);
extern void ClearSkipList(struct List *SkipList);
extern BPTR RecursiveCreateDir(CONST_STRPTR dirpath);
extern LONG CopyDirArray(Class * CLASS, Object * self, CONST_STRPTR sourcePath,
    CONST_STRPTR destinationPath, CONST_STRPTR directories[], struct List *SkipList);

BOOPSI_DISPATCHER_PROTO(IPTR, Install_Dispatcher, CLASS, self, message);

#endif /* IA_INSTALL_H */
