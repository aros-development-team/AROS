#include	<exec/lists.h>
#include	<exec/nodes.h>

static const char KMsgWelcome[] =
MUIX_B"\nWelcome to the AROS Research OS installer.\n\n" MUIX_N
"This program allows you to install AROS\n"
"on your computer's hard disk or an attached USB drive.\n\n"
"Please be aware that the stability of this software\n"
"cannot be guaranteed.\n"
"It is possible that " MUIX_B "loss of data" MUIX_N " may occur\n"
"on any drive in your system, including those\n"
"used by other operating systems.\n";

static const char KMsgInstallOptions[] =
"\nPlease choose your installation options.\n\n";

static const char KMsgDestOptions[] =
"\nPlease choose where to install AROS.\n\n"
"If you are unsure, then use the defaults";

static const char KMsgDestVolume[] =
"Destination Partition";

static const char KMsgWorkVolume[] =
"Work Partition";

static const char KMsgBeginWithPartition[] =
"OK, we are ready to begin.\n\n"
"Since you've chosen to format at least one partition,\n"
"you will no longer be able to undo changes\n"
"after this point.\n\n"
"Select Proceed to begin installation.";

static const char KMsgBeginWithoutPartition[] =
"OK, we are ready to begin...\n\n"
"We have collected enough information\n"
"to begin installation on this computer.\n\n"
"\nSelect Proceed to begin installation.";

static const char KMsgPartitionOptions[] =
"We will now create AROS partitions on the destination drive.\n\n"
"Please select how you would like to proceed.\n";

static const char KMsgDestPartition[] =
MUIX_B "System Partition:" MUIX_N;

static const char KMsgWorkPartition[] =
MUIX_B "Work Partition:" MUIX_N;

static const char KMsgPartitionTooBig[] =
"The partition sizes you have requested are too large.\n"
"The maximum size of an SFS partition is %ld GB (%ld MB),\n"
"while the maximum size of an FFS partition is %ld GB (%ld MB).\n"
"Please reduce the size of the affected partitions.\n";

static const char KMsgGrubOptions[] =
"AROS uses the GRUB bootloader.\n\n"
"In most cases, GRUB's bootblock should\n"
"be installed on the first drive in your system.\n"
"If the Installer detects a Microsoft Windows installation,\n"
"it will be included as a choice in GRUB's boot menu.\n";

static const char KMsgGrubDrive[] =
"Drive to install GRUB's bootblock on:";

static const char KMsgGrubGOptions[] =
MUIX_B "GRUB Settings" MUIX_N;

static const char KMsgGrubGrub[] =
"Path to GRUB files:";

static const char KMsgNoGrubDevice[] =
"Please enter the device name and unit number for\n"
"the drive GRUB's bootblock should be installed on.\n";

static const char KMsgPartitioning[] =
"Partitioning your drives...\n\n";

static const char KMsgInstall[] =
"Copying files to the destination drive.\n\n"
"This may take some time...\n";

static const char KMsgBootLoader[] =
"Copying the GRUB bootloader to your\n"
"destination drive, and installing...\n";

static const char KMsgPostInstall[] =
"Running the external post-install\n"
"script. Please wait.\n";

static const char KMsgDoneReboot[] =
"AROS partitions have now been created!\n"
"To continue installation, you must\n"
"reboot AROS, and re-run\n"
"this installer application\n\n"
"Select Proceed to finish\n";

static const char KMsgDoneUSB[] =
"AROS partitions have now been created!\n"
"To continue installation, you must remove\n"
"and re-attach your USB drive, and re-run\n"
"this installer application\n\n"
"Select Proceed to finish\n";

static const char KMsgDone[] =
"Congratulations, you now have AROS installed!\n\n"
"To boot AROS from the destination drive,\n"
"remove the installation media and\n"
"restart your computer using the\n"
"power switch or reset button.\n\n"
"Select Proceed to finish\n";

static const char KMsgCancelOK[] =
"Are you sure you wish to cancel\n"
"this installation?\n";

static const char KMsgCancelDanger[] =
"Irreversible changes have been made\n"
"to your system.\n\n"
"Cancelling now may leave your PC in an\n"
"unbootable state\n\n"
"Are you sure you wish to cancel\n"
"this installation?\n";

static const char KMsgProceed[] =
"_Proceed";

static const char KMsgNotAllFilesCopied[] =
"Not all files in '%s'\n"
"were copied.\n"
"Should the installation continue?\n";

static const char KMsgPartitioningFailed[] =
"Partitioning of disk failed. InstallAROS\n"
"will now quit.";

static const char KMsgGRUBNonFFSWarning[] =
"You have selected a filesystem different\n"
"than FFS-Intl for the system partition. Since\n"
"this distribution of AROS is compiled with the\n"
"original GRUB bootloader, you will not be able to\n"
"boot the system after installation.\n\n"
"Should the partitioning continue?\n";

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

#define MUIM_IC_NextStep           (0x00335551)
#define MUIM_IC_PrevStep           (0x00335552)
#define MUIM_IC_UndoSteps          (0x00335553)

#define MUIM_IC_Install            (0x00335554)

#define MUIM_FindDrives         (0x00335555)
#define MUIM_PartitionFree      (0x00335556)
#define MUIM_Partition          (0x00335557)
#define MUIM_Format             (0x00335558)

#define MUIM_IC_CopyFiles          (0x0033555a)
#define MUIM_IC_CopyFile           (0x0033555b)

#define MUIM_IC_CancelInstall      (0x0033556a)
#define MUIM_IC_ContinueInstall    (0x0033556b)
#define MUIM_IC_QuitInstall        (0x0033556c)

#define MUIM_Reboot             (0x0033556d)
#define MUIM_RefreshWindow      (0x00335570)

/* to be made obsolete */

#define MUIA_Page               (0x00335580)

#define MUIA_PartitionButton    (0x00335581)
#define MUIA_Gauge1             (0x00335582)
#define MUIA_Gauge2             (0x00335583)
#define MUIA_Install            (0x00335584)
/**/
#define MUIA_WelcomeMsg         (0x0033558a)
#define MUIA_FinishedMsg        (0x0033558b)

/* new - some/most will "vanish(tm)" */

#define MUIA_OBJ_Installer      (0x0033558d)
#define MUIA_Grub_Options       (0x0033558e)
#define MUIA_List_Options       (0x0033558f)

#define MUIA_OBJ_Window         (0x00335590)
#define MUIA_OBJ_WindowContent  (0x00335591)

#define MUIA_OBJ_PageTitle      (0x00335592)
#define MUIA_OBJ_PageHeader     (0x00335593)
#define MUIA_OBJ_CActionStrng   (0x00335594)

#define MUIA_OBJ_Back           (0x00335595)
#define MUIA_OBJ_Proceed        (0x00335596)
#define MUIA_OBJ_Cancel         (0x00335597)

#define MUIA_IC_License_File		(0x00335598)
#define MUIA_IC_License_Mandatory	(0x00335599)

#define MUIA_IC_EnableUndo			(0x00335599)

/* Install Results */

#define MUIA_InstallComplete    (0x003355ff)
#define MUIA_InstallFailed      (0x003355fe)

#define MUIV_Inst_Completed     (0xff)
#define MUIV_Inst_InProgress    (0x00)
#define MUIV_Inst_Cancelled     (0x01)
#define MUIV_Inst_Failed        (0x10)

struct MUIP_CopyFiles
{
    STACKED ULONG MethodID;
    STACKED CONST_STRPTR srcDir;
    STACKED CONST_STRPTR dstDir;
    STACKED CONST_STRPTR fileMask;
    STACKED ULONG recursive;
};

struct MUIP_CopyFile
{
    STACKED ULONG MethodID;
    STACKED CONST_STRPTR srcFile;
    STACKED CONST_STRPTR dstFile;
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
    Object	*opt_license;
    Object	*opt_lic_box;
    Object	*opt_lic_mgrp;
    Object	*opt_partmethod;
    Object	*opt_format;
    Object	*opt_locale;
    Object	*opt_copycore;
    Object	*opt_copyextra;
    Object	*opt_development;
    Object	*opt_bootloader;

    Object	*opt_reboot;

    BOOL	partitioned;
    BOOL	bootloaded;
};

struct Grub_Options
{
    Object	*gopt_drive;
    Object	*gopt_grub;
    Object  *gopt_grub2mode;
    BOOL	bootinfo;
};

struct InstallC_UndoRecord
{
	struct Node		undo_Node;      /* Inherits from struct Node */
	ULONG			undo_method;
	char			*undo_src;
	char			*undo_dst;
};

struct optionstmp
{
	IPTR	opt_format;
	IPTR	opt_locale;
	IPTR	opt_copycore;
	IPTR	opt_copyextra;
	IPTR	opt_development;
	IPTR	opt_bootloader;
	IPTR	opt_reboot;
};

enum IO_OVERWRITE_FLAGS
{
    IIO_Overwrite_Ask,
    IIO_Overwrite_Always,
    IIO_Overwrite_Never
};

struct Install_DATA
{
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
	APTR						instc_lic_buffer;

	struct List     			instc_undorecord;

/** Option Related **/

    struct Install_Options      *instc_options_main;
	struct Grub_Options			*instc_options_grub;
	struct optionstmp			*instc_options_backup;
    enum EStage                 instc_stage_prev;
    enum EStage                 instc_stage_next;

	enum IO_OVERWRITE_FLAGS     IO_Always_overwrite;

    BOOL                        instc_cflag_driveset;
	BOOL						instc_copt_undoenabled;
	BOOL						instc_copt_licensemandatory;
};

#define GRUB1 1
#define GRUB2 2
/* TODO: add more bootloaders */

struct BootloaderData
{
    UBYTE type;
    STRPTR file;
};
