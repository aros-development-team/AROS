#ifndef IA_INSTALL_H
#define IA_INSTALL_H
/*
    Copyright © 2003-2023, The AROS Development Team. All rights reserved.
    $Id$
*/

#include	<exec/lists.h>
#include	<exec/nodes.h>

#include <libraries/mui.h>

#define kExallBufSize          (4096)

#define SYS_PART_NAME         "DH0"
#define WORK_PART_NAME        "DH1"
#define NVME_SYS_PART_NAME    "NH0"
#define NVME_WORK_PART_NAME   "NH1"
#define USB_SYS_PART_NAME     "DU0"
#define USB_WORK_PART_NAME    "DU1"
#define EFI_VOL_NAME          "EFI"
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

#define MUIM_IC_BASE                            (TAG_USER + 0x00335000)
#define MUIA_IC_BASE                            (TAG_USER + 0x00335000)

#define MUIM_IS_BASE                            (TAG_USER + 0x00336000)
#define MUIA_IS_BASE                            (TAG_USER + 0x00336000)

#define MUIM_IO_BASE                            (TAG_USER + 0x00337000)
#define MUIA_IO_BASE                            (TAG_USER + 0x00337000)

#define MUIM_ID_BASE                            (TAG_USER + 0x00338000)
#define MUIA_ID_BASE                            (TAG_USER + 0x00338000)

#define MUIM_IV_BASE                            (TAG_USER + 0x00339000)
#define MUIA_IV_BASE                            (TAG_USER + 0x00339000)

/* ************************************************
        Main Installer Class Methods/Attribs
 * ************************************************/
#define MUIM_Install_AddOption                  (MUIM_IC_BASE + 0x1)

#define MUIA_Install_StageClass                 (MUIA_IC_BASE + 0x1)
#define MUIA_Install_OptionClass                (MUIA_IC_BASE + 0x2)

struct MUIP_Install_NewOption
{
    STACKED ULONG               MethodID;
    STACKED struct TagItem      *OptTags;
};

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

struct InstallC_UndoRecord
{
    struct Node                 undo_Node;      /* Inherits from struct Node */
    ULONG                       undo_method;
    char                        *undo_src;
    char                        *undo_dst;
};

enum IO_OVERWRITE_FLAGS
{
    IIO_Overwrite_Ask,
    IIO_Overwrite_Always,
    IIO_Overwrite_Never
};

#define Install_MakeOption(arg1, ...) \
({ \
    const IPTR Install_Option_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\
    struct MUIP_Install_NewOption Install_Option_msg[] = \
    { \
        MUIM_Install_AddOption, \
        (struct TagItem *)(Install_Option_args)  \
    }; \
    (Object *)DoMethodA((arg1), (Install_Option_msg)); \
})

#endif /* IA_INSTALL_H */
