#include	<exec/lists.h>
#include	<exec/nodes.h>

static const char* KMsgWelcome =
MUIX_L " " MUIX_B"\nWelcome to the AROS Research OS\n"
"installer.\n\n" MUIX_N
"This program allows you to install\n"
"the " MUIX_B "pre-alpha" MUIX_N " version of AROS\n"
"on to your computer's hard drive.\n\n"
"Since it is still in pre-alpha state, performance\n"
"may be sluggish - and stability isn't guaranteed\n\n";

static const char* KMsgInstallOptions =
"\nPlease choose your installation options.\n\n";

static const char* KMsgDestOptions =
"\nPlease choose where to install AROS.\n\n"
"If you are unsure, then use the defaults";

static const char* KMsgDestVolume =
"Destination Partition";

static const char* KMsgWorkVolume =
"Work Partition";

static const char* KMsgBeginWithPartition =
"OK, we are ready to begin.\n\n"
"Since you've selected to format the partition,\n"
"you will no longer be able to undo changes\n"
"after this point.\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.\n\n"
"Select Proceed to begin installation.";

static const char* KMsgBeginWithoutPartition =
"OK, we are ready to begin...\n\n"
"We have collected enough information\n"
"to begin installation on this computer.\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.\n"
"\n Press Proceed to begin installation...";

static const char* KMsgPartitionOptions =
"We will now create AROS partitions on your hard drive\n\n"
"Please select how you would like to proceed...\n";

static const char* KMsgDestPartition =
"System Partition (DH0)";

static const char* KMsgWorkPartition =
"Work Partition (DH1)";

static const char* KMsgGrubOptions =
"AROS uses the GRUB bootloader.\n\n"
"The Installer will install it to the first\n"
"drive on your system, and configure it\n"
"to boot AROS.\n";

static const char* KMsgGrubDrive =
"Drive GRUB will install on:";

static const char* KMsgGrubGOptions =
MUIX_B "GRUB Settings" MUIX_N;

static const char* KMsgGrubGrub =
"Path to GRUB files:";

static const char* KMsgPartitioning =
"Partition your drives...\n\n";

static const char* KMsgInstall =
"Copying files to the hard disk.\n\n"
"This may take some time...\n";

static const char* KMsgBootLoader =
"Copying the GRUB bootloader to your\n"
"hard drive, and installing...\n";

static const char* KMsgPostInstall =
"Running the external post-install\n"
"script. Please wait.\n";

static const char* KMsgDoneReboot =
"AROS partitions have now been created!\n"
"To continue installation, you must\n"
"reboot AROS, and re-run\n"
"this installer application\n\n"
"Select Proceed to finish\n";

static const char* KMsgDone =
"Congratulations, you now have AROS installed!\n\n"
"To boot AROS from the hard drive,\n"
"remove the installation media and\n"
"restart your computer using the\n"
"power switch or reset button.\n\n"
"Select Proceed to finish\n";

static const char* KMsgCancelOK =
"Are you sure you wish to cancel\n"
"this installation?\n";

static const char* KMsgCancelDanger =
"Irreversible changes have been made\n"
"to your system.\n\n"
"Cancelling now may leave your PC in an\n"
"unbootable state\n\n"
"Are you sure you wish to cancel\n"
"this installation?\n";

static const char* KMsgProceed =
"_Proceed";

#if GRUB == 1
static const char* KMsgGRUBNonFFSWarning =
"You have selected a filesystem different\n"
"than FFS-Intl for the DH0 partition. Since\n"
"this distribution of AROS is compiled  with\n"
"GRUB bootloader, you will not be able to\n"
"boot the system after installation\n\n"
"Should the partitioning continue?\n";
#endif

/* Unused strings. Kept for reference */

/*

static const char* KMsgLanguage =
"The system prefs for your language settings\n"
"will now launch\n\n"
"Choose the settings that are relevant to you\n";

static const char* KMsgNoDrives =
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
/* IO Retry Window */

#define MUIA_OBJ_IO_RWindow     (0x0033559a)
#define MUIA_OBJ_IO_RText       (0x0033559b)
#define MUIA_OBJ_IO_ROpt1       (0x0033559c)
#define MUIA_OBJ_IO_ROpt2		(0x0033559d)
#define MUIA_OBJ_IO_ROpt3		(0x0033559e)

#define MUIA_IIO_Flag			(0x0033559f)

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
    STACKED BOOL recursive;
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
/** Start IO Related **/

    APTR                        IO_IOTask;

    Object                      *IO_RWindow;
    Object                      *IO_RText;
    Object                      *IO_ROpt1;
    Object                      *IO_ROpt2;
    Object                      *IO_ROpt3;

	IPTR						IO_Flags;

#define IIO_Selected_Opt1		1
#define IIO_Selected_Opt2		2
#define IIO_Selected_Opt3		3

/** END IO Related **/
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
