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

#define MUIM_IC_BASE                    (TAG_USER + 0x00335000)
#define MUIA_IC_BASE                    (TAG_USER + 0x00336000)

#define MUIM_IC_NextStep                (MUIM_IC_BASE + 0x1)
#define MUIM_IC_PrevStep                (MUIM_IC_BASE + 0x2)
#define MUIM_IC_UndoSteps               (MUIM_IC_BASE + 0x3)

#define MUIM_IC_Install                 (MUIM_IC_BASE + 0x4)
#define MUIM_IC_SetLocalePrefs          (MUIM_IC_BASE + 0x5)

#define MUIM_FindDrives                 (MUIM_IC_BASE + 0x6)
#define MUIM_PartitionFree              (MUIM_IC_BASE + 0x7)
#define MUIM_Partition                  (MUIM_IC_BASE + 0x8)
#define MUIM_Format                     (MUIM_IC_BASE + 0x9)

#define MUIM_IC_CopyFiles               (MUIM_IC_BASE + 0xa)
#define MUIM_IC_CopyFile                (MUIM_IC_BASE + 0xb)

#define MUIM_IC_CancelInstall           (MUIM_IC_BASE + 0x1a)
#define MUIM_IC_ContinueInstall         (MUIM_IC_BASE + 0x1b)
#define MUIM_IC_QuitInstall             (MUIM_IC_BASE + 0x1c)

#define MUIM_Reboot                     (MUIM_IC_BASE + 0x1d)
#define MUIM_RefreshWindow              (MUIM_IC_BASE + 0x20)

/* to be made obsolete */

#define MUIA_Page                       (MUIA_IC_BASE + 0x80)

#define MUIA_PartitionButton            (MUIA_IC_BASE + 0x81)
#define MUIA_Gauge1                     (MUIA_IC_BASE + 0x82)
#define MUIA_Gauge2                     (MUIA_IC_BASE + 0x83)
#define MUIA_Install                    (MUIA_IC_BASE + 0x84)
/**/
#define MUIA_WelcomeMsg                 (MUIA_IC_BASE + 0x8a)
#define MUIA_FinishedMsg                (MUIA_IC_BASE + 0x8b)

/* new - some/most will "vanish(tm)" */

#define MUIA_OBJ_Installer              (MUIA_IC_BASE + 0x8d)
#define MUIA_Grub_Options               (MUIA_IC_BASE + 0x8e)
#define MUIA_List_Options               (MUIA_IC_BASE + 0x8f)

#define MUIA_OBJ_Window                 (MUIA_IC_BASE + 0x90)
#define MUIA_OBJ_WindowContent          (MUIA_IC_BASE + 0x91)

#define MUIA_OBJ_PageTitle              (MUIA_IC_BASE + 0x92)
#define MUIA_OBJ_PageHeader             (MUIA_IC_BASE + 0x93)
#define MUIA_OBJ_CActionStrng           (MUIA_IC_BASE + 0x94)

#define MUIA_OBJ_Back                   (MUIA_IC_BASE + 0x95)
#define MUIA_OBJ_Proceed                (MUIA_IC_BASE + 0x96)
#define MUIA_OBJ_Cancel                 (MUIA_IC_BASE + 0x97)

#define MUIA_IC_License_File            (MUIA_IC_BASE + 0x98)
#define MUIA_IC_License_Mandatory	(MUIA_IC_BASE + 0x99)

#define MUIA_IC_EnableUndo              (MUIA_IC_BASE + 0x99)

/* Install Results */

#define MUIA_InstallComplete            (MUIA_IC_BASE + 0xff)
#define MUIA_InstallFailed              (MUIA_IC_BASE + 0xfe)

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
