static const char* KMsgWelcome =
MUIX_L " " MUIX_B"\nWelcome to the Amiga Research OS\n"
"installer.\n\n" MUIX_N
"This program allows you to install.\n"
"the " MUIX_B "pre-alpha" MUIX_N " version of AROS\n"
"onto your computer system.\n\n"
"Since it is still in pre-alpha state, performance\n"
"may be sluggish - and stabilty isnt guarenteed\n\n";

static const char* KMsgPartitionDH0 =
"You do not have a DH0: partition.\n"
"Continue to have your master disk on\n"
"the first IDE channel turned into an\n"
"AROS disk. This step is NOT reversable!\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.\n";

static const char* KMsgInstallOptions =
"\nPlease choose your installation options.\n\n";

static const char* KMsgBeginWithPartition =
"OK We are ready to begin....\n\n"
"Since youve selcted to format the partition,\n"
"you will no longer be able to undo changes\n"
"after this point,\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.\n"
"\n Press Proceed to begin installation....";

static const char* KMsgBeginWithoutPartition =
"OK We are ready to begin....\n\n"
"We have collected enough information\n"
"to begin installion on this computer.\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.\n"
"\n Press Proceed to begin installation....";

static const char* KMsgLanguage =
"The system prefs for your language settings\n"
"will now launch\n\n"
"Choose the settings that are relevant to you\n";

static const char* KMsgPartitioning =
"Partition your drives..\n\n";

static const char* KMsgPartitioningWipe =
"Now erasing the contents of the master\n"
"disk on the first IDE channel.\n\n";

static const char* KMsgInstall =
"Copying files to the harddisk.\n\n"
"This will take quite some time...\n";

static const char* KMsgBootLoader =
"Copying the GRUB bootloader to your\n"
"hard drive, and installing ..\n";

static const char* KMsgDoneReboot =
"DH0 is now prepared for use!\n"
"to continue installation this pc\n"
"must be rebooted, and this installer\n"
"application re-run\n\n"
"Press Proceed To Finish\n";

static const char* KMsgDone =
"Congratulations, you now have AROS installed!\n\n"
"Press Proceed To Finish\n";

static const char* KMsgNoDrives =
"It appears you do not have a hard\n"
"drive installed in your PC\n\n"
"Installation of AROS can only be performed onto\n"
"a hard disk just now. Sorry.\n\nPress Proceed To Exit\n";

static const char* KMsgxxxx =
"It appears you do not have a hard\n"
"drive installed in your PC\n\n"
"Installation of AROS can only be performed onto\n"
"a hard disk just now. Sorry.\n\nPress Proceed To Exit\n";

static const char* KMsgCancelOK =
"Are you sure you wish to cancel\n"
"this instalation?\n";

static const char* KMsgCancelDanger =
"Irreversable changes have been made\n"
"to your system.\n\n"
"Canceling now may leave your pc in an\n"
"unbootable state\n\n"
"Are you sure you wish to cancel\n"
"this instalation?\n";

static const char* KMsgProceed =
"_Proceed";

#define MUIM_DH0                (0x00335560)
#define MUIM_NextStep           (0x00335561)
#define MUIM_PrevStep           (0x00335562)
#define MUIM_Partition          (0x00335563)
#define MUIM_Install            (0x00335564)

#define MUIM_Format             (0x00335565)

#define MUIM_MakeDirs           (0x00335566)
#define MUIM_CopyFiles          (0x00335567)
#define MUIM_CopyFile           (0x00335568)
#define MUIM_Reboot             (0x00335569)

#define MUIM_CancelInstall      (0x0033556a)
#define MUIM_ContinueInstall    (0x0033556b)
#define MUIM_QuitInstall        (0x0033556c)

#define MUIM_RefreshWindow      (0x0033556d)

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

#define MUIA_OBJ_Installer      (0x0033558e)
#define MUIA_List_Options       (0x0033558f)

#define MUIA_OBJ_Window         (0x00335590)
#define MUIA_OBJ_WindowContent  (0x00335591)

#define MUIA_OBJ_PageTitle      (0x00335592)
#define MUIA_OBJ_PageHeader     (0x00335593)
#define MUIA_OBJ_CActionStrng   (0x00335594)

#define MUIA_OBJ_Back           (0x00335595)
#define MUIA_OBJ_Proceed        (0x00335596)
#define MUIA_OBJ_Cancel         (0x00335597)

#define MUIA_OBJ_CWindow        (0x0033559a)
#define MUIA_OBJ_CText          (0x0033559b)
#define MUIA_OBJ_COK            (0x0033559c)
#define MUIA_OBJ_CCancel        (0x0033559d)

#define MUIA_InstallComplete    (0x0033559e)
#define MUIA_InstallFailed      (0x0033559f)

#define MUIV_Inst_Completed     (0xff)
#define MUIV_Inst_InProgress    (0x00)
#define MUIV_Inst_Cancelled     (0x01)
#define MUIV_Inst_Failed        (0x10)

/*
struct MUIP_Prepare
{
    ULONG MethodID;
    Object* prepare;
    Object* done;
};
*/
struct MUIP_Dir
{
    ULONG MethodID;
    CONST_STRPTR srcDir;
    CONST_STRPTR dstDir;
};

struct MUIP_CopyFiles
{
    ULONG MethodID;
    CONST_STRPTR srcDir;
    CONST_STRPTR dstDir;
    ULONG noOfFiles;
    ULONG currFile;
};

struct MUIP_CopyFile
{
    ULONG MethodID;
    CONST_STRPTR srcFile;
    CONST_STRPTR dstFile;
};

enum EStage
{
    EMessageStage,
    EInstallOptionsStage,
    EInstallMessageStage,
    EPartitioningStage,
    EInstallStage,
    EDoneStage
};

struct Install_Options
{
    Object *opt_partition;
    Object *opt_locale;
    Object *opt_copycore;
    Object *opt_copyextra;
    Object *opt_development;
    Object *opt_bootloader;
    Object *opt_reboot;
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
    Object                      *CWindow;
    Object                      *CText;
    Object                      *COK;
    Object                      *CCancel;

/**/
    ULONG                       inst_success;
    ULONG                       drive_set;

    ULONG                       disable_back,
                                status_back,
                                status_proceed,
                                status_cancel;

    ULONG                       cur_width,
                                cur_height;

    ULONG                       IOTask;

    struct Install_Options      *options;
    enum EStage                 nextStage;
};
