static const char* KMsgWelcome1 =
"Welcome to the AROS installer.\n\n"
"You do not have a DH0: partition.\n"
"Continue to have your master disk on\n"
"the first IDE channel turned into an\n"
"AROS disk. This step is NOT reversable!\n\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.";

static const char* KMsgWelcome2 =
"Welcome to the AROS installer.\n\n"
"Your DH0: partition will be formatted,\n"
"the necessary files will be copied, and\n"
"then the GRUB bootloader will be installed.\n"
"You are being " MUIX_B "warned" MUIX_N ", this is pre-alpha software.";

static const char* KMsgPartition =
"Now wiping the master disk in the first IDE channel.\n\n";

static const char* KMsgInstall =
"Copying files to the harddisk.\n\n"
"This will take quite some time...\n";

static const char* KMsgDone =
"Congratulations, you now have AROS installed!";

#define MUIM_DH0 (0x00335560)
#define MUIM_Continue (0x00335561)
#define MUIM_Partition (0x00335562)
#define MUIM_Install (0x00335563)

#define MUIM_Format (0x00335564)

#define MUIM_MakeDirs (0x00335565)
#define MUIM_CopyFiles (0x00335566)
#define MUIM_CopyFile (0x00335567)
#define MUIM_Reboot (0x00335568)

#define MUIA_Page (0x00335580)
#define MUIA_WelcomeMsg (0x00335581)
#define MUIA_WelcomeButton (0x00335582)
#define MUIA_PartitionButton (0x00335583)
#define MUIA_Gauge1 (0x00335584)
#define MUIA_Gauge2 (0x00335585)
#define MUIA_Install (0x00335586)

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
    EWelcomeStage,
    EPartitioningStage,
    EInstallStage,
    EDoneStage
};

struct Install_DATA
{
    Object *welcomeMsg;
    Object *welcome;
    Object *partition;
    Object *page;
    Object *gauge1;
    Object *gauge2;
    Object *label;
    enum EStage nextStage;
};
