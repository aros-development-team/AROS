/*
    Copyright (C) 2003-2023, The AROS Development Team. All rights reserved.
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

#include <mui/TextEditor_mcc.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_config.h"
#include "ia_stage.h"
#include "ia_option.h"
#include "ia_driveselect.h"
#include "ia_driveselect_intern.h"
#include "ia_volume.h"
#include "ia_volume_intern.h"
#include "ia_bootloader.h"

struct ExpansionBase *ExpansionBase = NULL;

char *source_Path = NULL;       /* full path to source "tree" */
char *extras_source = NULL;

char *dest_Path = NULL;         /* DOS DEVICE NAME of part used to store "aros" */
char *work_Path = NULL;         /* DOS DEVICE NAME of part used to store "work" */

Object *driveObjSelect = NULL;
struct DriveSelect_Global driveGlobalData;

Object *optObjCyclePartScheme = NULL;

Object *optObjCheckCopyToWork = NULL;
Object *optObjCheckWork = NULL;
Object *show_formatsys = NULL;
Object *show_formatwork = NULL;
Object *optObjCheckFormatSys = NULL;
Object *optObjCheckFormatWork = NULL;
Object *optObjCycleFSTypeSys = NULL;
Object *optObjCycleFSTypeWork = NULL;
Object *optObjCycleSysUnits = NULL;
Object *optObjCycleWorkUnits = NULL;

Object *optObjDestVolLabel = NULL;
Object *optObjWorkDestLabel = NULL;

Object *show_sizesys = NULL;
Object *show_sizework = NULL;
Object *optObjCheckSysSize = NULL;
Object *optObjCheckSizeWork = NULL;
Object *optObjCheckCreateWork = NULL;
Object *sys_size = NULL;
Object *work_size = NULL;
Object *optObjCheckEFI = NULL;
Object *optObjEFIVolumeName = NULL;
Object *optObjDestVolumeName = NULL;
Object *sys_devname = NULL, *work_devname = NULL;

Object *grub_device = NULL;
Object *grub_unit = NULL;
Object *optObjCycleGrub2Mode = NULL;

Object *reboot_group = NULL;
Object *efi_group = NULL;
Object *sys_group = NULL;
Object *work_group = NULL;

#define CHECKBUTTONCOMMON \
            ImageButtonFrame, \
            MUIA_CycleChain, 1, \
            MUIA_InputMode, MUIV_InputMode_Toggle, \
            MUIA_Image_Spec, MUII_CheckMark, \
            MUIA_Image_FreeVert, TRUE, \
            MUIA_Background, MUII_ButtonBack, \
            MUIA_ShowSelState, FALSE,

BOOL read_environment_variable(CONST_STRPTR envarchiveDisk, CONST_STRPTR name, STRPTR buffer, ULONG size);

void SetOptObjNotificationFromOptObj(Object *optobjSrc, Object *optobjTgt, IPTR trigTag, IPTR trigVal, IPTR setTag, IPTR setVal)
{
    Object *optObjS, *optObjT;
    GET(optobjSrc, MUIA_InstallOption_Obj, &optObjS);
    GET(optobjTgt, MUIA_InstallOption_Obj, &optObjT);
    DoMethod(optObjS, MUIM_Notify, trigTag, trigVal,
        (IPTR) optObjT, 3, MUIM_Set,
        setTag, setVal);
}

void SetOptObjNotificationFromObj(Object *objSrc, Object *optobjTgt, IPTR trigTag, IPTR trigVal, IPTR setTag, IPTR setVal)
{
    Object *optObjT;
    GET(optobjTgt, MUIA_InstallOption_Obj, &optObjT);
    DoMethod(objSrc, MUIM_Notify, trigTag, trigVal,
        (IPTR) optObjT, 3, MUIM_Set,
        setTag, setVal);
}

void SetObjNotificationFromOptObj(Object *optobjSrc, Object *objTgt, IPTR trigTag, IPTR trigVal, IPTR setTag, IPTR setVal)
{
    Object *optObjS;
    GET(optobjSrc, MUIA_InstallOption_Obj, &optObjS);
    DoMethod(optObjS, MUIM_Notify, trigTag, trigVal,
        (IPTR) objTgt, 3, MUIM_Set,
        setTag, setVal);
}

int main(int argc, char *argv[])
{
    Object *InstallObj = NULL;

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
    Object *label = NULL;
    Object *gad_back;
    Object *gad_proceed;
    Object *gad_cancel;
    Object *grub_drive = NULL;
    Object *grub_grub = NULL;
    Object *LicenseMandGrp = NULL;
    Object *optObjCheckLicense;
    Object *optObjCheckFormat;
    Object *optObjCheckLocale;
    Object *optObjCheckCore;
    Object *optObjCheckDev;
    Object *optObjCheckExtras;
    Object *optObjCheckBootloader;
    Object *optObjCheckReboot;

    Object *gauge1 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);

    Object *gauge2 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);

    Object *gauge3 =
        (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE,
        MUIA_Gauge_Current, 0, End);

    static char *opt_partentries[4] = {
        NULL
    };

    static char *opt_ptypes[] = {
        "RDB",
        "MBR/EBR",
        "GPT",
        NULL
    };
    
    static char *opt_fstypes[] = {
        "FFS-Intl",
        "SFS",
        NULL
    };

    static char *opt_sizeunits[] = {
#define OPT_SIZEUNIT_MB 0
        "MB",
#define OPT_SIZEUNIT_GB 1
        "GB",
        NULL
    };

    static char *opt_grub2mode[3] = {
        NULL
    };

    struct Install_Options *install_opts = NULL;
    struct Grub_Options *grub_opts = NULL;
    char *source_path = NULL;
    char *dest_path = NULL;
    char *work_path = NULL;

    char *labPad;
    char *labDrive;
    char *labScheme;
    char *labESP;
    char *labSysPart;
    char *labWrkPart;

    IPTR pathend = 0;
    BPTR lock = BNULL;

    if (!Locale_Initialize())
        return 20;

    labDrive = _(MSG_DRIVE);
    labPad = labDrive;

    labScheme = _(MSG_PARTSCHEME);
    if (strlen(labScheme) > strlen(labPad))
        labPad = labScheme;
    labESP = _(MSG_EFIESP);
    if (strlen(labESP) > strlen(labPad))
        labPad = labESP;
    labSysPart = _(MSG_DESTPARTITION);
    if (strlen(labSysPart) > strlen(labPad))
        labPad = labSysPart;
    labWrkPart = _(MSG_WORKPARTITION);
    if (strlen(labWrkPart) > strlen(labPad))
        labPad = labWrkPart;

    struct MUI_CustomClass *icMcc =
        MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
        sizeof(struct Install_DATA), Install__Dispatcher);
    if (!icMcc)
    {
        return 20;
    }

    InstallObj = NewObject(icMcc->mcc_Class, NULL,
        TAG_DONE);

    bug("[INST-APP] Base Install Object @ 0x%p\n", InstallObj);

    struct IClass  *stageClassPtr = NULL;
    GET(InstallObj, MUIA_Install_StageClass, &stageClassPtr);

    struct MUI_CustomClass *driveselMcc =
        MUI_CreateCustomClass(NULL, MUIC_Group, NULL,
        sizeof(struct DriveSelect_Data), DriveSelect__Dispatcher);
    if (!driveselMcc)
    {
        DisposeObject(InstallObj);
        MUI_DeleteCustomClass(icMcc);
        return 20;
    }

    struct MUI_CustomClass *volMcc =
        MUI_CreateCustomClass(NULL, MUIC_Group, NULL,
        sizeof(struct Volume_Data), Volume__Dispatcher);
    if (!volMcc)
    {
        MUI_DeleteCustomClass(driveselMcc);
        DisposeObject(InstallObj);
        MUI_DeleteCustomClass(icMcc);
        return 20;
    }
    
    optObjCheckLicense = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"license",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);
    optObjCheckFormat = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"format",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, TRUE,
            End),
        TAG_DONE);
    optObjCheckLocale = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"localize",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);
    optObjCheckCore = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"core",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, TRUE,
            End),
        TAG_DONE);
    optObjCheckDev = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"developer",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);
    optObjCheckExtras = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"extras",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, TRUE,
            End),
        TAG_DONE);
    optObjCheckReboot = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"reboot",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
            End),
        TAG_DONE);
    
    opt_partentries[0] = _(MSG_USEFREE);
    opt_partentries[1] = _(MSG_WIPEDISK);
    opt_partentries[2] = _(MSG_USEEXISTING);

    opt_grub2mode[0] = _(MSG_TEXT);
    opt_grub2mode[1] = _(MSG_GFX);

    gad_back = ImageButton(_(MSG_BACK), "THEME:Images/Gadgets/Previous");
    gad_proceed = ImageButton(_(MSG_PROCEED), "THEME:Images/Gadgets/Next");
    gad_cancel = ImageButton(_(MSG_CANCEL2), "THEME:Images/Gadgets/Cancel");

    driveObjSelect = NewObject(driveselMcc->mcc_Class, NULL,
            MUIA_DriveSelect_InstallInstance, (IPTR)InstallObj,
            MUIA_DriveSelect_GlobalData, (IPTR)&driveGlobalData,
            MUIA_DriveSelect_SysObjPtr,  (IPTR)&sys_devname,
            MUIA_DriveSelect_WorkObjPtr,  (IPTR)&work_devname,
        TAG_DONE);

    optObjCheckEFI = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"efi",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_ShortHelp, __(MSG_HELP_EFIESP),
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);

    optObjCheckCopyToWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"copytowork",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Disabled, TRUE,
            End),
        TAG_DONE);

    optObjCheckWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"work",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);

    optObjCheckFormatSys = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"sysfmt",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, TRUE,
            End),
        TAG_DONE);

    optObjCheckFormatWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"workfmt",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Disabled, TRUE,
            End),
        TAG_DONE);

    optObjCheckSysSize = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"syssize",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
            End),
        TAG_DONE);

    optObjCycleFSTypeSys = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"sysfstype",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, opt_fstypes,
                MUIA_Disabled, FALSE,
                MUIA_Cycle_Active,
#if defined(BOOTLOADER_GRUB1)
                    BootLoaderType == BOOTLOADER_GRUB1 ? 0 : 1,
#else
                    1,
#endif
            End),
        TAG_DONE);

    optObjCycleSysUnits = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"sysunits",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, opt_sizeunits,
                MUIA_Disabled, TRUE,
                MUIA_Cycle_Active, 1,
            End),
        TAG_DONE);

    optObjCheckSizeWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"worksize",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
                MUIA_Disabled, TRUE,
            End),
        TAG_DONE);

    optObjCheckCreateWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"creatework",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, FALSE,
                MUIA_Disabled, TRUE,
            End),
        TAG_DONE);

    optObjCycleFSTypeWork = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"workfstype",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, opt_fstypes,
                MUIA_Disabled, TRUE,
                MUIA_Cycle_Active, 1,
            End),
        TAG_DONE);

    optObjCycleWorkUnits = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"workunits",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, opt_sizeunits,
                MUIA_Disabled, TRUE,
                MUIA_Cycle_Active, 1,
            End),
        TAG_DONE);

    optObjCycleGrub2Mode = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"grub2mode",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, opt_grub2mode,
                MUIA_Disabled, FALSE,
                MUIA_Cycle_Active, 0,
            End),
        TAG_DONE);

    optObjEFIVolumeName = Install_MakeOption(InstallObj, 
                                                                MUIA_InstallOption_ID, (IPTR)"efivol",
                                                                MUIA_InstallOption_ValueTag, MUIA_String_Contents,
                                                                MUIA_InstallOption_Obj, (IPTR)(StringObject,
                                                                    MUIA_CycleChain, 1,
                                                                    MUIA_String_Contents, EFI_VOL_NAME,
                                                                    MUIA_FixWidthTxt, "xxxxxx",
                                                                    MUIA_Disabled, TRUE,
                                                                    MUIA_Frame, MUIV_Frame_String,
                                                                End),
                                                            TAG_DONE);

    Object *bootVolFSObj = TextObject,
                MUIA_Text_Contents, (IPTR)"FAT32",
            End;
    Object *bootVolSizeObj = HGroup,
                Child, TextObject,
                    MUIA_Text_Contents, (IPTR)"100",
                End,
                Child, TextObject,
                    MUIA_Text_Contents, (IPTR)opt_sizeunits[OPT_SIZEUNIT_MB],
                End,
                Child, HVSpace,
            End;
    Object *bootVolObj = NewObject(volMcc->mcc_Class, NULL,
            MUIA_Volume_InstallInstance, (IPTR)InstallObj,
            MUIA_Volume_Informative, TRUE,
            MUIA_Volume_CreateObj,  (IPTR)optObjCheckEFI,
            MUIA_Volume_PopPadTxt,  (IPTR)labPad,
            MUIA_Volume_NameObj,  (IPTR)optObjEFIVolumeName,
            MUIA_Volume_FSObj,  (IPTR)bootVolFSObj,
            MUIA_Volume_SizeObj,  (IPTR)bootVolSizeObj,
        TAG_DONE);

    bug("[INST-APP] Boot Vol FS Obj @ 0x%p, SizeObj @ 0x%p, VolumeObj @ 0x%p\n", bootVolFSObj, bootVolSizeObj, bootVolObj);
    optObjDestVolumeName = Install_MakeOption(InstallObj, 
                                                                MUIA_InstallOption_ID, (IPTR)"tgtvol",
                                                                MUIA_InstallOption_ValueTag, MUIA_String_Contents,
                                                                MUIA_InstallOption_Obj, (IPTR)(sys_devname = StringObject,
                                                                    MUIA_CycleChain, 1,
                                                                    MUIA_FixWidthTxt, "xxxxxx",
                                                                    MUIA_Frame, MUIV_Frame_String,
                                                                End),
                                                            TAG_DONE);
    Object *sysVolSizeObj = ColGroup(7),
                                            Child, (IPTR) (sys_size = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) optObjCycleSysUnits,
                                            Child, (IPTR) optObjCheckSysSize,
                                            Child, (IPTR) LLabel(_(MSG_SPECIFYSIZE)),
                                        End;
    Object *sysVolObj = NewObject(volMcc->mcc_Class, NULL,
            MUIA_Volume_InstallInstance, (IPTR)InstallObj,
            MUIA_Volume_Help, (IPTR)__(MSG_HELP_PARTITION),
            MUIA_Volume_NameObj,  (IPTR)optObjDestVolumeName,
            MUIA_Volume_FSObj,  (IPTR)optObjCycleFSTypeSys,
            MUIA_Volume_SizeObj,  (IPTR)sysVolSizeObj,
        TAG_DONE);

    bug("[INST-APP] Sys Vol Name Obj @ 0x%p, SizeObj @ 0x%p, VolumeObj @ 0x%p\n", optObjDestVolumeName, sysVolSizeObj, sysVolObj);

    work_devname = StringObject,
                                MUIA_CycleChain, 1,
                                MUIA_FixWidthTxt, "xxxxxx",
                                MUIA_Frame, MUIV_Frame_String,
                            End;
    Object *workVolSizeObj = ColGroup(7),
                                            Child, (IPTR) (work_size = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) optObjCycleWorkUnits,
                                            Child, (IPTR) optObjCheckSizeWork,
                                            Child, (IPTR) LLabel(_(MSG_SPECIFYSIZE)),
                                        End;
    Object *workVolObj = NewObject(volMcc->mcc_Class, NULL,
            MUIA_Volume_InstallInstance, (IPTR)InstallObj,
            MUIA_Volume_Help, (IPTR)__(MSG_HELP_PARTITION),
            MUIA_Volume_CreateObj,  (IPTR)optObjCheckCreateWork,
            MUIA_Volume_NameObj,  (IPTR)work_devname,
            MUIA_Volume_FSObj,  (IPTR)optObjCycleFSTypeWork,
            MUIA_Volume_SizeObj,  (IPTR)workVolSizeObj,
        TAG_DONE);


    bug("[INST-APP] Work Vol Name Obj @ 0x%p, SizeObj @ 0x%p, VolumeObj @ 0x%p\n", work_devname, workVolSizeObj, workVolObj);

    install_opts =
        AllocMem(sizeof(struct Install_Options), MEMF_CLEAR | MEMF_PUBLIC);

    grub_opts =
        AllocMem(sizeof(struct Grub_Options), MEMF_CLEAR | MEMF_PUBLIC);

    source_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);
    extras_source = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);

    dest_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);
    work_path = AllocVec(256, MEMF_CLEAR | MEMF_PUBLIC);

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
    if (read_environment_variable(source_Path, "EXTRASPATH", extras_source, 256))
        *PathPart(extras_source) = '\0';
    else
        strcpy(extras_source, source_Path);

    dest_Path = dest_path;
    work_Path = work_path;

    BOOTLOADER_InitSupport();

#ifdef __mc68000__
    IPTR fsHelp = __(MSG_HELP_PARTRDB);
#else
    IPTR fsHelp = __(MSG_HELP_PARTNORDB);
#endif
    optObjCyclePartScheme = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"sysfstype",
            MUIA_InstallOption_ValueTag, MUIA_Cycle_Active,
            MUIA_InstallOption_Obj, (IPTR)(CycleObject,
                MUIA_CycleChain, 1,
    			MUIA_ShortHelp, fsHelp,
                MUIA_Cycle_Entries, opt_ptypes,
                MUIA_Disabled, FALSE,
                MUIA_Cycle_Active,
#ifdef __mc68000__
                    0,
#else
                    1,
#endif
            End),
        TAG_DONE);

    optObjCheckBootloader = Install_MakeOption(InstallObj, 
            MUIA_InstallOption_ID, (IPTR)"reboot",
            MUIA_InstallOption_ValueTag, MUIA_Selected,
            MUIA_InstallOption_Obj, (IPTR)(ImageObject, CHECKBUTTONCOMMON
                MUIA_Selected, BootLoaderType == BOOTLOADER_NONE ? FALSE : TRUE,
                MUIA_Disabled, BootLoaderType == BOOTLOADER_NONE ? TRUE : FALSE,
            End),
        TAG_DONE);

    lock = Lock(DEF_INSTALL_IMAGE, ACCESS_READ);
    UnLock(lock);

    LicenseMsg = MUI_NewObject(MUIC_TextEditor,
        MUIA_CycleChain, 1,
        MUIA_Background, MUII_SHINE,
        MUIA_TextEditor_ReadOnly, TRUE, TAG_DONE);

    if (!LicenseMsg)
    {
        D(bug("[INST-APP] Failed to create LicenseMsg Object\n"));
        Locale_Deinitialize();
        exit(5);
    }

    Object *app = ApplicationObject,
        MUIA_Application_Title,       __(MSG_TITLE),
        MUIA_Application_Version,     (IPTR) "$VER: InstallAROS 1.30 (21.05.2023)",
        MUIA_Application_Copyright,   (IPTR) "Copyright © 2003-2023, The AROS Development Team. All rights reserved.",
        MUIA_Application_Author,      (IPTR) "John \"Forgoil\" Gustafsson, Nick Andrews & Neil Cafferkey",
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Base,        (IPTR) "INSTALLER",

        SubWindow, (IPTR) (wnd = WindowObject,
            MUIA_Window_Title, __(MSG_TITLE),
            MUIA_Window_ID, MAKE_ID('I','N','S','T'),
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
                                            Child, (IPTR) optObjCheckLicense,
                                            Child, (IPTR) LLabel(_(MSG_ACCEPT)),
                                            Child, (IPTR) HVSpace,
                                        End),
                                    End,
                                End,

                                /* Partitioning options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) CLabel(_(MSG_PARTITIONOPTIONS)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) VGroup,
                                            GroupFrame,
                                            Child, (IPTR) HGroup,
                                                Child, VGroup,
                                                    Child, (IPTR) LLabel(labDrive),
                                                    Child, (IPTR) RectangleObject,
                                                        MUIA_FixWidthTxt, labPad,
                                                    End,
                                                End,
                                                Child, (IPTR) driveObjSelect,
                                            End,

                                            Child, (IPTR) HGroup,
                                                Child, (IPTR) HVSpace,
                                                Child, (IPTR) (radio_part = RadioObject,
                                                    MUIA_CycleChain, 1,
                                                    MUIA_Radio_Entries, (IPTR) opt_partentries,
                                                End),
                                                Child, (IPTR) HVSpace,
                                            End,
                                        End,
                                        Child, (IPTR) HVSpace,

                                        Child, ScrollgroupObject,
                                            MUIA_Scrollgroup_FreeHoriz, FALSE,
                                            MUIA_Scrollgroup_FreeVert, TRUE,
                                            MUIA_Scrollgroup_Contents, (IPTR)(VGroup,
                                                Child, (IPTR) VGroup,
                                                    Child, (IPTR) LLabel(labScheme),
                                                    Child, HGroup,
                                                        Child, (IPTR) RectangleObject,
                                                            MUIA_FixWidthTxt, labPad,
                                                        End,
                                                        Child, (IPTR) optObjCyclePartScheme,
                                                    End,
                                                    Child, (IPTR) RectangleObject,
                                                        MUIA_Rectangle_HBar, TRUE,
                                                        MUIA_FixHeight,      2,
                                                    End,
                                                    Child, (IPTR) HVSpace,
                                                End,

                                                Child, (IPTR) (efi_group = VGroup,
                                                    MUIA_ShowMe, FALSE,
                                                    Child, (IPTR) HGroup,
                                                        Child, (IPTR) LLabel(labESP),
                                                        Child, (IPTR) bootVolObj,
                                                    End,
                                                    Child, (IPTR) RectangleObject,
                                                        MUIA_Rectangle_HBar, TRUE,
                                                        MUIA_FixHeight,      2,
                                                    End,
                                                    Child, (IPTR) HVSpace,
                                                End),

                                                Child, (IPTR)(sys_group = VGroup,
                                                    Child, (IPTR) HGroup,
                                                        Child, (IPTR) LLabel(labSysPart),
                                                        Child, (IPTR) sysVolObj,
                                                    End,
                                                End),

                                                Child, (IPTR) HVSpace,

                                                Child, (IPTR)(work_group = VGroup,
                                                    Child, (IPTR) HGroup,
                                                        Child, (IPTR) LLabel(labWrkPart),
                                                        Child, (IPTR) workVolObj,
                                                    End,
                                                End),

                                                Child, (IPTR) HVSpace,
                                            End),
                                        End,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) VGroup,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) CLabel(_(MSG_INSTALLOPTIONS)),
                                            Child, (IPTR) HVSpace,
                                        End,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) optObjCheckLocale,
                                            Child, (IPTR) LLabel(_(MSG_CHOOSELANG)),
                                            Child, (IPTR) optObjCheckCore,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLCORE)),
                                            Child, (IPTR) optObjCheckExtras,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLEXTRA)),
                                            Child, (IPTR) optObjCheckDev,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLDEVEL)),
                                            Child, (IPTR) optObjCheckBootloader,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLBOOT)),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) CLabel(_(MSG_DESTOPTIONS)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(_(MSG_DESTVOLUME)),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatsys = ColGroup(2),
                                                Child, (IPTR) optObjCheckFormatSys,
                                                Child, (IPTR) LLabel(_(MSG_FORMATPART)),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (optObjDestVolLabel = Install_MakeOption(InstallObj, 
                                            MUIA_InstallOption_ID, (IPTR)MUIV_InstallOptionID_Dest,
                                        TAG_DONE)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) optObjCheckWork,
                                            Child, (IPTR) LLabel(_(MSG_USEWORK)),
                                            Child, (IPTR) optObjCheckCopyToWork,
                                            Child, (IPTR) LLabel(_(MSG_USEWORKFOR)),
                                        End,
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(_(MSG_WORKVOLUME)),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatwork = ColGroup(2),
                                                Child, (IPTR) optObjCheckFormatWork,
                                                Child, (IPTR) LLabel(_(MSG_FORMATPART)),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (optObjWorkDestLabel = Install_MakeOption(InstallObj, 
                                            MUIA_InstallOption_ID, (IPTR)"wrklabel",
                                            MUIA_InstallOption_ValueTag, MUIA_String_Contents,
                                            MUIA_InstallOption_Obj, (IPTR)(StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                        TAG_DONE)),
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                /* Bootloader options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) CLabel(_(MSG_GRUBOPTIONS)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) LLabel(_(MSG_GRUBGOPTIONS)),
                                        Child, (IPTR) LLabel(_(MSG_GRUBDRIVE)),
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) ColGroup(5),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel(_(MSG_DEVICE)),
                                            Child, (IPTR) (grub_device = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Reject, " \"\'*",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 200,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel(_(MSG_UNIT)),
                                            Child, (IPTR) (grub_unit = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Integer, 0,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 20,
                                            End),
                                        End,

                                        Child, (IPTR) (grub_drive = TextObject,
                                            MUIA_Text_PreParse, (IPTR) "" MUIX_C,
                                            MUIA_Text_Contents, (IPTR)" ",
                                        End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) LLabel(_(MSG_GRUBGRUB)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (grub_grub = TextObject,
                                            MUIA_Text_PreParse, (IPTR) "" MUIX_C,
                                            MUIA_Text_Contents, (IPTR)" ",
                                        End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(4),
                                            Child, (IPTR) LLabel(_(MSG_MENUMODE)),
                                            Child, (IPTR) optObjCycleGrub2Mode,
                                            Child, (IPTR) HVSpace,
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) HVSpace,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(_(MSG_PARTITIONING)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup,
                                            GaugeFrame,
                                            MUIA_Background, MUII_HSHINEBACK,
                                            Child, gauge1,
                                        End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) HVSpace,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(_(MSG_PARTITIONING)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup,
                                            GaugeFrame,
                                            MUIA_Background, MUII_HSHINEBACK,
                                            Child, (IPTR) gauge3,
                                        End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) (HGroup,
                                            Child, (IPTR) (RectangleObject,
                                                MUIA_HorizDisappear, 0,
                                            End),
                                            Child, (IPTR) (VGroup,
                                                Child, (IPTR) (RectangleObject,
                                                    MUIA_VertDisappear, 0,
                                                End),
                                                Child, (IPTR) (pagetitle = CLabel(" ")),
                                                Child, (IPTR) HVSpace,
                                                Child, (IPTR) (pageheader = FreeCLabel(_(MSG_INSTALL))),
                                                Child, (IPTR) HVSpace,
                                            End),
                                            Child, (IPTR) (RectangleObject,
                                                MUIA_HorizDisappear, 0,
                                            End),
                                        End),
                                        Child, (IPTR) (label = FreeLLabel("YOU SHOULD NOT SEE THIS")),
                                        Child, (IPTR) (RectangleObject,
                                            MUIA_FixHeightTxt, "\n",
                                        End),
                                        Child, (IPTR) (currentaction = TextObject,
                                            MUIA_Text_Contents,(IPTR)" ",
                                        End),
                                        Child, (IPTR) VGroup,
                                            GaugeFrame,
                                            MUIA_Background, MUII_HSHINEBACK,
                                            Child, gauge2,
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                /* Completed page */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        MUIA_Group_SameHeight, FALSE,
                                        Child, (IPTR) (doneMsg = FreeCLabel(_(MSG_DONE))),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (reboot_group = ColGroup(2),
                                            MUIA_Weight,0,
                                            MUIA_ShowMe, FALSE,
                                            Child, (IPTR) optObjCheckReboot,
                                            Child, (IPTR) LLabel(_(MSG_REBOOT)),
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
        Locale_Deinitialize();
        exit(5);
    }

    if ((FindResident("efi.resource")))
    {
        SET(efi_group, MUIA_ShowMe, TRUE);
        SET(bootVolObj, MUIA_Volume_ParentGrpObj, efi_group);
    }
    SET(sysVolObj, MUIA_Volume_ParentGrpObj, sys_group);
    SET(workVolObj, MUIA_Volume_ParentGrpObj, work_group);

    DoMethod(driveObjSelect, MUIM_Notify, MUIA_DriveSelect_DevProbed, TRUE,
                    (IPTR)radio_part, 3, MUIM_Set, MUIA_Radio_Active, 2);

    D(bug("[INST-APP] App @ 0x%p\n", app));
    /* Update GUI in response to certain user actions */

    /* Notifications on change of enable status of 'enter size of sys volume'
     * (this tells us if we are using existing partitions) */
#if (0)
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjDestVolumeName,
                                                MUIA_Disabled, MUIV_EveryTime, MUIA_Disabled, MUIV_TriggerValue);
#endif
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCycleFSTypeSys,
                                                MUIA_Disabled, MUIV_EveryTime, MUIA_Disabled, MUIV_TriggerValue);

    /* Notifications on change of selected status of 'enter size of sys volume' */
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCyclePartScheme,
                                                MUIA_Disabled, MUIV_EveryTime, MUIA_Disabled, MUIV_TriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCheckEFI,
                                                MUIA_Disabled, MUIV_EveryTime, MUIA_Disabled, MUIV_TriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCheckCreateWork,
                                                MUIA_Disabled, MUIV_EveryTime, MUIA_Disabled, MUIV_TriggerValue);
    SetObjNotificationFromOptObj(optObjCheckSysSize, sys_size,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCycleSysUnits,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
#if (0)
    SetOptObjNotificationFromOptObj(optObjCheckSysSize, optObjCheckCreateWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Selected, FALSE);
#endif

    /* Notifications on change of selected status of 'create work volume' */
#if (0)
    SetObjNotificationFromOptObj(optObjCheckCreateWork, work_devname,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
#endif
    SetOptObjNotificationFromOptObj(optObjCheckCreateWork, optObjCheckSizeWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckCreateWork, optObjCycleFSTypeWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckCreateWork, optObjCheckSizeWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Selected, FALSE);

    /* Notifications on change of selected status of 'enter size of work volume' */
    SetObjNotificationFromOptObj(optObjCheckSizeWork, work_size,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckSizeWork, optObjCycleWorkUnits,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);

    /* Notifications on installing bootloader */
    SetOptObjNotificationFromOptObj(optObjCheckBootloader, optObjCycleGrub2Mode,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);

    SetOptObjNotificationFromOptObj(optObjCheckCore, optObjCheckFormatSys,
                                                MUIA_Selected, FALSE, MUIA_Selected, FALSE);
    SetOptObjNotificationFromOptObj(optObjCheckWork, optObjCheckCopyToWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckWork, optObjCheckCopyToWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Selected, FALSE);
    SetOptObjNotificationFromOptObj(optObjCheckWork, optObjCheckFormatWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);
    SetOptObjNotificationFromOptObj(optObjCheckWork, optObjCheckFormatWork,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Selected, FALSE);
    SetOptObjNotificationFromOptObj(optObjCheckWork, optObjWorkDestLabel,
                                                MUIA_Selected, MUIV_EveryTime, MUIA_Disabled, MUIV_NotTriggerValue);

    D(bug("[INST-APP] Notifications configured\n"));

    /* Update GUI in response to certain user actions */

    GET(optObjCheckLicense, MUIA_InstallOption_Obj, &install_opts->opt_license);
    install_opts->opt_lic_box = LicenseMsg;
    install_opts->opt_lic_mgrp = LicenseMandGrp;

    install_opts->opt_partmethod = radio_part;
    GET(optObjDestVolumeName, MUIA_InstallOption_Obj, &install_opts->opt_sysdevname);
    install_opts->opt_workdevname = work_devname;

    GET(optObjCheckFormat, MUIA_InstallOption_Obj, &install_opts->opt_format);
    GET(optObjCheckLocale, MUIA_InstallOption_Obj, &install_opts->opt_locale);
    GET(optObjCheckCore, MUIA_InstallOption_Obj, &install_opts->opt_copycore);
    GET(optObjCheckExtras, MUIA_InstallOption_Obj, &install_opts->opt_copyextra);
    GET(optObjCheckDev, MUIA_InstallOption_Obj, &install_opts->opt_developer);
    GET(optObjCheckBootloader, MUIA_InstallOption_Obj, &install_opts->opt_bootloader);
    GET(optObjCheckReboot, MUIA_InstallOption_Obj, &install_opts->opt_reboot);

    grub_opts->gopt_drive = grub_drive;
    grub_opts->gopt_grub = grub_grub;
    GET(optObjCycleGrub2Mode, MUIA_InstallOption_Obj, &grub_opts->gopt_grub2mode);

    SET(pagetitle, MUIA_Font, MUIV_Font_Big);

    D(bug("[INST-APP] Creating Installer Stage Object using class @ 0x%p\n", stageClassPtr));

    Object *installer = NewObject(stageClassPtr, NULL,

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

    D(bug("[INST-APP] Installer Instance @ 0x%p\n", installer));

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    D(bug("[INST-APP] Installer notifications configured\n"));

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

    D(bug("[INST-APP] Removing App Object\n"));

    MUI_DisposeObject(app);

    D(bug("[INST-APP] Removing Custom Classes\n"));

    MUI_DeleteCustomClass(driveselMcc);
    MUI_DeleteCustomClass(icMcc);

    FreeVec(extras_source);
    FreeVec(source_Path);

  main_error:
    Locale_Deinitialize();
    return 0;
}
