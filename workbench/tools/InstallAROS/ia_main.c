/*
    Copyright © 2003-2020, The AROS Development Team. All rights reserved.
    $Id$
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
#include "ia_bootloader.h"

struct ExpansionBase *ExpansionBase = NULL;

char *boot_Device = "ahci.device";
ULONG boot_Unit = 0;

char *source_Path = NULL;       /* full path to source "tree" */
char *extras_source = NULL;

char *dest_Path = NULL;         /* DOS DEVICE NAME of part used to store "aros" */
char *work_Path = NULL;         /* DOS DEVICE NAME of part used to store "work" */

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
Object *cycle_grub2mode = NULL;

Object *reboot_group = NULL;

#define BUTTONCOMMON \
            ImageButtonFrame, \
            MUIA_CycleChain, 1, \
            MUIA_InputMode, MUIV_InputMode_Toggle, \
            MUIA_Image_Spec, MUII_CheckMark, \
            MUIA_Image_FreeVert, TRUE, \
            MUIA_Background, MUII_ButtonBack, \
            MUIA_ShowSelState, FALSE,

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

    Object *label = NULL;

    Object *gad_back;
    Object *gad_proceed;
    Object *gad_cancel;

    Object *grub_drive = NULL;
    Object *grub_grub = NULL;

    Object *LicenseMandGrp = NULL;
    Object *check_license = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
	End;

    Object *check_format = ImageObject, BUTTONCOMMON
            MUIA_Selected, TRUE,
	End;

    Object *check_locale = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
	End;

    Object *check_core = ImageObject, BUTTONCOMMON
            MUIA_Selected, TRUE,
        End;

    Object *check_dev = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
        End;

    Object *check_extras = ImageObject, BUTTONCOMMON
            MUIA_Selected, TRUE,
        End;

    Object *check_bootloader;

    Object *check_reboot = ImageObject, BUTTONCOMMON
        End;

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
        "AHCI/SATA",
        "IDE",
        "USB",
        NULL
    };

    static char *opt_partentries[4] = {
        NULL
    };
    
    static char *opt_fstypes[] = {
        "FFS-Intl",
        "SFS",
        NULL
    };

    static char *opt_sizeunits[] = {
        "MB",
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

    IPTR pathend = 0;
    BPTR lock = BNULL;

    if (!Locale_Initialize())
        return 20;

    opt_partentries[0] = _(MSG_USEFREE);
    opt_partentries[1] = _(MSG_WIPEDISK);
    opt_partentries[2] = _(MSG_USEEXISTING);

    opt_grub2mode[0] = _(MSG_TEXT);
    opt_grub2mode[1] = _(MSG_GFX);

    gad_back = SimpleButton(_(MSG_BACK));
    gad_proceed = SimpleButton(_(MSG_PROCEED));
    gad_cancel = SimpleButton(_(MSG_CANCEL2));

    check_copytowork = ImageObject, BUTTONCOMMON
            MUIA_Disabled, TRUE,
        End;

    check_work = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
        End;

    check_formatsys = ImageObject, BUTTONCOMMON
            MUIA_Selected, TRUE,
        End;

    check_formatwork = ImageObject, BUTTONCOMMON
            MUIA_Disabled, TRUE,
        End;

    check_sizesys = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
        End;

    check_sizework = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
            MUIA_Disabled, TRUE,
        End;

    check_creatework = ImageObject, BUTTONCOMMON
            MUIA_Selected, FALSE,
            MUIA_Disabled, TRUE,
        End;

    cycle_fstypework = CycleObject,
            MUIA_CycleChain, 1,
            MUIA_Cycle_Entries, opt_fstypes,
            MUIA_Disabled, TRUE,
            MUIA_Cycle_Active, 1,
        End;

    cycle_sysunits = CycleObject,
            MUIA_CycleChain, 1,
            MUIA_Cycle_Entries, opt_sizeunits,
            MUIA_Disabled, TRUE,
            MUIA_Cycle_Active, 1,
        End;

    cycle_workunits = CycleObject,
            MUIA_CycleChain, 1,
            MUIA_Cycle_Entries, opt_sizeunits,
            MUIA_Disabled, TRUE,
            MUIA_Cycle_Active, 1,
        End;

    cycle_grub2mode = CycleObject,
            MUIA_CycleChain, 1,
            MUIA_Cycle_Entries, opt_grub2mode,
            MUIA_Disabled, FALSE,
            MUIA_Cycle_Active, 0,
        End;

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
    if (read_environment_variable(source_Path, "EXTRASPATH", extras_source,
            256))
        *PathPart(extras_source) = '\0';
    else
        strcpy(extras_source, source_Path);

    dest_Path = dest_path;
    work_Path = work_path;

    BOOTLOADER_InitSupport();
    cycle_fstypesys = CycleObject,
            MUIA_CycleChain, 1,
            MUIA_Cycle_Entries, opt_fstypes,
            MUIA_Disabled, FALSE,
            MUIA_Cycle_Active,
#if defined(BOOTLOADER_GRUB1)
                BootLoaderType == BOOTLOADER_GRUB1 ? 0 : 1,
#else
                1,
#endif
        End;

    check_bootloader = ImageObject, BUTTONCOMMON
            MUIA_Selected, BootLoaderType == BOOTLOADER_NONE ? FALSE : TRUE,
            MUIA_Disabled, BootLoaderType == BOOTLOADER_NONE ? TRUE : FALSE,
        End;

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
        MUIA_Application_Version,     (IPTR) "$VER: InstallAROS 1.20 (30.08.2018)",
        MUIA_Application_Copyright,   (IPTR) "Copyright © 2003-2014, The AROS Development Team. All rights reserved.",
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
                                            Child, (IPTR) check_license,
                                            Child, (IPTR) LLabel(_(MSG_ACCEPT)),
                                            Child, (IPTR) HVSpace,
                                        End),
                                    End,
                                End,

                                /* Partitioning options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(_(MSG_PARTITIONOPTIONS)),
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (radio_part = RadioObject,
                                            GroupFrame,
                                            MUIA_CycleChain, 1,
                                            MUIA_Radio_Entries, (IPTR) opt_partentries,
                                        End),

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(_(MSG_DRIVE)),
                                        Child, (IPTR) ColGroup(6),
                                            Child, (IPTR) LLabel(_(MSG_TYPE)),
                                            Child, (IPTR) (cycle_drivetype =
                                                CycleObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_Cycle_Entries, (IPTR) opt_drivetypes,
                                                MUIA_Cycle_Active, 0,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel(_(MSG_DEVICE)),
                                            Child, (IPTR) (dest_device =
                                                StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Contents, (IPTR) boot_Device,
                                                MUIA_String_Reject, " \"\'*",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 300,
                                            End),
                                            Child, (IPTR) LLabel(_(MSG_UNIT)),
                                            Child, (IPTR) (dest_unit =
                                                StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Integer, 0,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_Frame, MUIV_Frame_String,
                                                MUIA_HorizWeight, 20,
                                            End),
                                        End,

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(_(MSG_DESTPARTITION)),
                                        Child, (IPTR) ColGroup(7),
                                            Child, (IPTR) LLabel(_(MSG_NAME)),
                                            Child, (IPTR) (sys_devname = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Contents, SYS_PART_NAME,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) LLabel(_(MSG_FILESYSTEM)),
                                            Child, (IPTR) cycle_fstypesys,
                                            Child, (IPTR) LLabel(_(MSG_SIZE)),
                                            Child, (IPTR) (sys_size = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) cycle_sysunits,
                                            Child, (IPTR) check_sizesys,
                                            Child, (IPTR) LLabel(_(MSG_SPECIFYSIZE)),
                                        End,

                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) LLabel(_(MSG_WORKPARTITION)),
                                        Child, (IPTR) ColGroup(7),
                                            Child, (IPTR) LLabel(_(MSG_NAME)),
                                            Child, (IPTR) (work_devname = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Contents, WORK_PART_NAME,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) HVSpace,
                                            Child, (IPTR) check_creatework,
                                            Child, (IPTR) LLabel(_(MSG_CREATE)),
                                            Child, (IPTR) LLabel(_(MSG_FILESYSTEM)),
                                            Child, (IPTR) cycle_fstypework,
                                            Child, (IPTR) LLabel(_(MSG_SIZE)),
                                            Child, (IPTR) (work_size = StringObject,
                                                MUIA_CycleChain, 1,
                                                MUIA_String_Accept, "0123456789",
                                                MUIA_String_Integer, 0,
                                                MUIA_Disabled, TRUE,
                                                MUIA_Frame, MUIV_Frame_String,
                                            End),
                                            Child, (IPTR) cycle_workunits,
                                            Child, (IPTR) check_sizework,
                                            Child, (IPTR) LLabel(_(MSG_SPECIFYSIZE)),
                                        End,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(_(MSG_INSTALLOPTIONS)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) check_locale,
                                            Child, (IPTR) LLabel(_(MSG_CHOOSELANG)),
                                            Child, (IPTR) check_core,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLCORE)),
                                            Child, (IPTR) check_extras,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLEXTRA)),
                                            Child, (IPTR) check_dev,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLDEVEL)),
                                            Child, (IPTR) check_bootloader,
                                            Child, (IPTR) LLabel(_(MSG_INSTALLBOOT)),
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(_(MSG_DESTOPTIONS)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(_(MSG_DESTVOLUME)),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatsys = ColGroup(2),
                                                Child, (IPTR) check_formatsys,
                                                Child, (IPTR) LLabel(_(MSG_FORMATPART)),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (dest_volume = StringObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Frame, MUIV_Frame_String,
                                        End),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) check_work,
                                            Child, (IPTR) LLabel(_(MSG_USEWORK)),
                                            Child, (IPTR) check_copytowork,
                                            Child, (IPTR) LLabel(_(MSG_USEWORKFOR)),
                                        End,
                                        Child, (IPTR) HVSpace,

                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) ColGroup(2),
                                                Child, (IPTR) LLabel(_(MSG_WORKVOLUME)),
                                                Child, (IPTR) HVSpace,
                                            End,
                                            Child, (IPTR) (show_formatwork = ColGroup(2),
                                                Child, (IPTR) check_formatwork,
                                                Child, (IPTR) LLabel(_(MSG_FORMATPART)),
                                            End),
                                        End,
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (work_volume = StringObject,
                                            MUIA_CycleChain, 1,
                                            MUIA_Disabled, TRUE,
                                            MUIA_Frame, MUIV_Frame_String,
                                        End),
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                /* Bootloader options */
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
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
                                            Child, (IPTR) cycle_grub2mode,
                                            Child, (IPTR) HVSpace,
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
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
                                            Child, (IPTR) check_reboot,
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

    /* Update GUI in response to certain user actions */

    /* Notifications upon selection of drive type */
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 0,
        (IPTR) dest_device, 3, MUIM_Set,
        MUIA_String_Contents, "ahci.device");
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 1,
        (IPTR) dest_device, 3, MUIM_Set,
        MUIA_String_Contents, "ata.device");
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 2,
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
        SYS_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 2,
        (IPTR) sys_devname, 3, MUIM_Set, MUIA_String_Contents,
        USB_SYS_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 0,
        (IPTR) work_devname, 3, MUIM_Set, MUIA_String_Contents,
        WORK_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 1,
        (IPTR) work_devname, 3, MUIM_Set, MUIA_String_Contents,
        WORK_PART_NAME);
    DoMethod(cycle_drivetype, MUIM_Notify, (IPTR) MUIA_Cycle_Active, 2,
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
    install_opts->opt_sysdevname = sys_devname;
    install_opts->opt_workdevname = work_devname;

    install_opts->opt_format = check_format;
    install_opts->opt_locale = check_locale;
    install_opts->opt_copycore = check_core;
    install_opts->opt_copyextra = check_extras;
    install_opts->opt_developer = check_dev;
    install_opts->opt_bootloader = check_bootloader;

    install_opts->opt_reboot = check_reboot;

    grub_opts->gopt_drive = grub_drive;
    grub_opts->gopt_grub = grub_grub;
    grub_opts->gopt_grub2mode = cycle_grub2mode;

    SET(pagetitle, MUIA_Font, MUIV_Font_Big);

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
    Locale_Deinitialize();
    return 0;
}
