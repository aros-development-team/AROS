/*
    Copyright © 2003-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#include   <aros/debug.h>

#include   <proto/exec.h>
#include   <proto/expansion.h>
#include   <proto/dos.h>
#include   <proto/oop.h>
#include   <proto/partition.h>
#include   <proto/intuition.h>
#include   <proto/muimaster.h>
#include   <proto/locale.h>
#include   <proto/utility.h>

#include   <proto/alib.h>

#include   <libraries/configvars.h>
#include   <libraries/expansionbase.h>
#include   <libraries/partition.h>
#include   <libraries/mui.h>

#include   <devices/trackdisk.h>
#include   <devices/scsidisk.h>

#include   <dos/dos.h>
#include   <dos/filehandler.h>

#include <utility/tagitem.h>

#include   <exec/memory.h>
#include   <exec/execbase.h>
#include   <exec/lists.h>
#include   <exec/nodes.h>
#include   <exec/types.h>
#include   <exec/ports.h>

#include <hidd/storage.h>

#include   <zune/systemprefswindow.h>
#include   <zune/prefseditor.h>
#include   <zune/aboutwindow.h>

#include   <stdlib.h>
#include   <stdio.h>
#include   <strings.h>

#define _QP_MAIN_C

#include "QP_Intern.h"

#include "QP_ccApp.h"
#include "QP_ccOperation.h"
#include "QP_ccOpGrp.h"
#include "QP_ccTxt.h"
#include "QP_ccDisk.h"
#include "QP_ccFree.h"
#include "QP_ccPartition.h"
#include "QP_ccPartitionContainer.h"
#include "QP_locale.h"

//Temp
extern Object *QPart_AppWin_Legend;

extern int FindDisks();

/**/

struct ListNode
{
    struct Node          ln;
    struct List             list;
    struct List             history;
    struct ListNode          *parent;
    ULONG             flags;              /* see below */
    UWORD             change_count;       /* number of changes on this node or children*/
};

struct HDTBPartition
{
    struct ListNode         listnode;
    struct HDTBPartition      *root;
    struct PartitionHandle       *ph;
    struct DriveGeometry       dg;
    struct DosEnvec          de;
    struct PartitionType       type;
    struct PartitionTable       *table;
    ULONG                flags;
    ULONG                pos;

    struct   Hook            AQPart_Part_Hook;

    int                  AQPart_Part_Size;
    int                  AQPart_Part_Free;               /* only used by "root/extended" atm - but could also show partitions used status? */

    IPTR                  AQPart_Part_Type;

    struct   HDNode         *AQPart_Part_HardDisk;
      
    Object                *AQPart_Partition;               /* NicJA: Additions - our interface objects for this disk*/   
    Object                *AQPart_txt_PartName;
    Object                *AQPart_Part_Content;   
};

#define PNF_ACTIVE        (1<<0)
#define PNF_BOOTABLE      (1<<1)
#define PNF_AUTOMOUNT     (1<<2)

struct HDNode
{
    struct ListNode         listnode;
    struct HDTBPartition      root_partition;
    LONG                unit;

    struct   Hook         AQPart_HardDisk_Hook;

    int                  AQPart_HardDisk_Size;

    Object                *AQPart_HardDisk;               /* NicJA: Additions - our interface objects for this disk*/   

    Object                *AQPart_txt_DriveName;
    Object                *AQPart_txt_DriveCapacity;
    Object                *AQPart_txt_DriveUnit;

    Object                *AQPart_HardDisk_Content;
    Object                *AQPart_HardDisk_Root;
    Object                *AQPart_HardDisk_Weight;
};

struct PartitionTable
{
    struct ListNode         listnode;
    struct   PartitionAttribute   *tattrlist; /* supported partition table attributes */
    struct   PartitionAttribute   *pattrlist; /* supported partition attributes */
    ULONG            reserved;
    ULONG            max_partitions;
    ULONG            type;
};

/**/

//struct   ExpansionBase      *ExpansionBase=NULL;
//struct   Library         *MUIMasterBase=NULL;

OOP_AttrBase HiddStorageUnitAB;

const struct OOP_ABDescr storage_abd[] =
{
    {IID_Hidd_StorageUnit ,  &HiddStorageUnitAB },
    {NULL            ,  NULL          }
};

Object                  *AQPart_App        = NULL,  /*  */

                        *AQPart_gge_Progress = NULL,

                        *AQPart_win_Main   = NULL,

                        *AQPart_VOID       = NULL,  /*  */

                        *AQPart_grp_Main  = NULL,
                        *AQPart_grp_Drives = NULL,  /*  */
                        *AQPart_panel_Pending = NULL,
                        *AQPart_grp_Pending  = NULL,
                        *AQPart_grp_Info = NULL,
                        *AQPart_txt_Info = NULL,  /*  */
                        *AQPart_spcr_Info = NULL,

                        *AQPart_chk_Scale  = NULL,

                        *AQPart_but_Help   = NULL,
                        *AQPart_but_Legend = NULL,
                        *AQPart_but_Create = NULL,
                        *AQPart_but_Delete = NULL;

struct   List           AQPart_driveList;
struct   List           AQPart_partList;
   
struct   HDNode         *AQPart_selectedDrive=NULL;
struct   HDTBPartition  *AQPart_selectedPart=NULL;
   
int                     AQPart_HardDisk_Largest = 0;   /* The size of the biggest drive */
BOOL                    AQPart_help_SHOW =  FALSE;

BYTE                    AQPart_signal_PART=0;
BYTE                    AQPart_signal_SAVE=0;

struct   MUI_CustomClass      *mcc_oldpartition = NULL;

struct   MUI_CustomClass      *mcc_qpapp = NULL;
struct   MUI_CustomClass      *mcc_qptxt = NULL;
struct   MUI_CustomClass      *mcc_qpdisk = NULL;
struct   MUI_CustomClass      *mcc_qpfree = NULL;
struct   MUI_CustomClass      *mcc_qpop = NULL;
struct   MUI_CustomClass      *mcc_qpopgrp = NULL;
struct   MUI_CustomClass      *mcc_qppartition = NULL;
struct   MUI_CustomClass      *mcc_qppartitioncontainer = NULL;

char   * sizename[8] =
{
    "B",
    "KB",
    "MB",
    "GB",
    "TB",
    "PB",
    "EB",
    NULL
};

/***********

 ***********/

int   fs_fat32_letter   = 0;

/***********

 ***********/

struct QPoldPartition_DATA
{
    ULONG      qpt_parttype;
    ULONG      qpt_partsize;
#define   MUIA_QPart_PartSize         12345670
#define   MUIA_QPart_PartType         12345671
#define   MUIV_QPart_TypeFree         1
#define   MUIV_QPart_TypeRoot         2
#define   MUIV_QPart_TypeExtended         3
#define   MUIV_QPart_TypePartition         4
    IPTR         qpt_partcolor;
   
    ULONG      qpt_start;
#define   MUIA_QPart_PartStart         12345672
    ULONG      qpt_end;
#define   MUIA_QPart_PartEnd         12345673
   
   
    Object      *qpt_fgroup;            /* object this "partitions" frame is drawn using */
   
    Object      *qpt_partcontainergrp;   
#define   MUIA_QPart_PartContainerGroup   12345674
    Object      *qpt_partcontainer;
#define   MUIA_QPart_PartContainer      12345675
    Object      *qpt_partspacer;
#define   MUIA_QPart_CSStore         12345676
    Object      *qpt_partname;
#define   MUIA_QPart_Name         12345677
};


AROS_UFH3(void, switchBubbleHelp,
    AROS_UFHA(struct Hook *, this_hook, A0),
    AROS_UFHA(void *, unused, A2),
    AROS_UFHA(char *, c, A1))
{
    AROS_USERFUNC_INIT

    struct HDNode         *CurDisk;

    if (AQPart_help_SHOW ==  TRUE)
    {
        AQPart_help_SHOW=  FALSE;

        set(AQPart_but_Legend, MUIA_ShortHelp, NULL);
        set(AQPart_but_Create, MUIA_ShortHelp, NULL);
        set(AQPart_but_Delete, MUIA_ShortHelp, NULL);

        set(AQPart_but_Help, MUIA_Background, (IPTR)MUII_ButtonBack);

        ForeachNode(&AQPart_driveList, CurDisk)
        {
            set(CurDisk->AQPart_HardDisk, MUIA_ShortHelp, NULL);
            set(CurDisk->AQPart_txt_DriveName, MUIA_ShortHelp, NULL);
            set(CurDisk->AQPart_txt_DriveCapacity, MUIA_ShortHelp, NULL);
            set(CurDisk->AQPart_txt_DriveUnit, MUIA_ShortHelp, NULL);
            set(CurDisk->AQPart_HardDisk_Content, MUIA_ShortHelp, NULL);
        }
    }
    else
    {
        AQPart_help_SHOW =  TRUE;

        set(AQPart_but_Legend, MUIA_ShortHelp, __(MSG_DESCRIPTION_LEGEND));
        set(AQPart_but_Create, MUIA_ShortHelp, __(MSG_DESCRIPTION_CREATE));
        set(AQPart_but_Delete, MUIA_ShortHelp, __(MSG_DESCRIPTION_DELETE));

        set(AQPart_but_Help, MUIA_Background, (IPTR)MUII_SHINE);

        ForeachNode(&AQPart_driveList, CurDisk)
        {
            set(CurDisk->AQPart_HardDisk, MUIA_ShortHelp, __(MSG_DESCRIPTION_DISK));
            set(CurDisk->AQPart_txt_DriveName, MUIA_ShortHelp, __(MSG_DESCRIPTION_DISKNAME));
            set(CurDisk->AQPart_txt_DriveCapacity, MUIA_ShortHelp, __(MSG_DESCRIPTION_DISKSIZE));
            set(CurDisk->AQPart_txt_DriveUnit, MUIA_ShortHelp, __(MSG_DESCRIPTION_DISKDEVICE));
            set(CurDisk->AQPart_HardDisk_Content, MUIA_ShortHelp, __(MSG_DESCRIPTION_PARTITIONS));
        }
    }

    AROS_USERFUNC_EXIT
}

/**/

/**/

BOOL   AQPart_legend_SHOW = FALSE;

struct   Hook   SwitchHelphook;
struct Library *UUIDBase = NULL;

/********** PROGRAM ENTRY ****************/

int main(int argc,char *argv[])
{
    struct BootNode             *CurBootNode=NULL;
    struct DevInfo              *devnode=NULL;
    struct FileSysStartupMsg    *StartMess=NULL;
    struct DosEnvec             *DriveEnv=NULL;
    int                         nodeCount=0, nodeCurrent=0;
    struct HDNode               *CurDisk=NULL;

    struct InputHNode           *CurEvent=NULL;

    Object                      *AQPart_win_Progress = NULL,
                                *AQPart_win_About = NULL,
                                *AQPart_win_Welcome = NULL,

                                *AQPart_txt_Progress = NULL,

                                *AQPart_but_Use = NULL,
                                *AQPart_but_Cancel = NULL,
                                *AQPart_but_proceed = NULL,
                                *AQPart_but_Cancel2 = NULL,

                                *AQPart_VOID0 = NULL,
                                *AQPart_VOID1 = NULL,
                                *AQPart_VOID2 = NULL,
                                *AQPart_VOID3 = NULL;

    int                         error = RETURN_OK;
    IPTR                        helpHeight = 0;

    AQPart_signal_PART = AllocSignal(-1);

    UUIDBase = OpenLibrary("uuid.library",0);

    AQPart_gge_Progress = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);

    AQPart_but_Help = ImageButton(NULL, "THEME:Images/Gadgets/Help");
    GET(AQPart_but_Help, MUIA_Height, &helpHeight);

//    SimpleButton("_?");
    if ((ExpansionBase = OpenLibrary("expansion.library", 0)))
    {
        OOP_ObtainAttrBases(storage_abd);

        if ((mcc_qpapp = MUI_CreateCustomClass(NULL, MUIC_Application, NULL, sizeof(struct QPApp_DATA), QPApp_Dispatcher)) &&
            (mcc_qpopgrp = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPOpGrp_DATA), QPOpGrp_Dispatcher)) &&
            (mcc_qpop = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPOp_DATA), QPOp_Dispatcher)) &&
            (mcc_qptxt = MUI_CreateCustomClass(NULL, MUIC_Text, NULL, sizeof(struct QPTxt_DATA), QPTxt_Dispatcher)) &&
            (mcc_qpfree = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPFree_DATA), QPFree_Dispatcher)) &&
            ((mcc_qpdisk = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPDisk_DATA), QPDisk_Dispatcher))) &&
           ((mcc_qppartition = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPPartition_DATA), QPPartition_Dispatcher))) &&
           ((mcc_qppartitioncontainer = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPPartitionContainer_DATA), QPPartitionContainer_Dispatcher))))
        {
//            D(
                bug("[Qp.Main] Partition Class @ 0x%p <0x%p>\n", mcc_qppartition, mcc_qppartition->mcc_Class);
//            )
            AQPart_but_Legend = HGroup,
                    ButtonFrame,
                    MUIA_InputMode    , MUIV_InputMode_RelVerify,
                    MUIA_Background   , MUII_ButtonBack,
                    MUIA_FixHeight, helpHeight,
                    MUIA_CycleChain,    1,
                    Child, HVSpace,
                    Child, VGroup,
                        Child, HVSpace,
                        Child, (IPTR)(TextObject,
                            MUIA_Font, MUIV_Font_Button,
                            MUIA_Text_HiCharIdx, '_',
                            MUIA_Text_Contents, __(WORD_Legend),
                            MUIA_Text_PreParse, "\33c",
                        End),
                        Child, HVSpace,
                    End,
                    Child, HVSpace,
                End;

            AQPart_but_Create = HGroup,
                    ButtonFrame,
                    MUIA_InputMode    , MUIV_InputMode_RelVerify,
                    MUIA_Background   , MUII_ButtonBack,
                    MUIA_FixHeight, helpHeight,
                    MUIA_CycleChain,    1,
                    Child, HVSpace,
                    Child, VGroup,
                        Child, HVSpace,
                        Child, (IPTR)(TextObject,
                            MUIA_Font, MUIV_Font_Button,
                            MUIA_Text_HiCharIdx, '_',
                            MUIA_Text_Contents, __(WORD_Create),
                            MUIA_Text_PreParse, "\33c",
                        End),
                        Child, HVSpace,
                    End,
                    Child, HVSpace,
                End;

            AQPart_but_Delete = HGroup,
                    ButtonFrame,
                    MUIA_InputMode    , MUIV_InputMode_RelVerify,
                    MUIA_Background   , MUII_ButtonBack,
                    MUIA_FixHeight, helpHeight,
                    MUIA_CycleChain,    1,
                    Child, HVSpace,
                    Child, VGroup,
                        Child, HVSpace,
                        Child, (IPTR)(TextObject,
                            MUIA_Font, MUIV_Font_Button,
                            MUIA_Text_HiCharIdx, '_',
                            MUIA_Text_Contents, __(WORD_Delete),
                            MUIA_Text_PreParse, "\33c",
                        End),
                        Child, HVSpace,
                    End,
                    Child, HVSpace,
                End;

            AQPart_chk_Scale = MUI_MakeObject(MUIO_Checkmark, __(MSG_SCALE_DRIVES));

#if (0)
            AQPart_txt_Info = NewObject(mcc_qptxt->mcc_Class, NULL,
                                        MUIA_Text_SetMin, FALSE,
                                        MUIA_Text_SetMax, FALSE,
                                    TAG_DONE);
#else
            AQPart_txt_Info = FloattextObject,
                                        MUIA_Background, MUII_WindowBack,
                                        MUIA_Listview_ScrollerPos, MUIV_Listview_ScrollerPos_None,
                                        MUIA_Listview_Input, FALSE,
                                        MUIA_Text_SetMin, FALSE,
                                        MUIA_Text_SetMax, FALSE,
                                    End;
#endif

            AQPart_App = NewObject(mcc_qpapp->mcc_Class, NULL,
                MUIA_Application_Title,       (IPTR)"AROS Quick Part",
                MUIA_Application_Version,     (IPTR)"$VER: QuickPart 0.61 (27.05.23)",
                MUIA_Application_Copyright,   (IPTR)"Copyright © 2003-2023, The AROS Development Team. All rights reserved.",
                MUIA_Application_Author,      (IPTR)"Nick \"Kalamatee\" Andrews",
                MUIA_Application_Description, __(MSG_DESCRIPTION),
                MUIA_Application_Base,        (IPTR)"AQPART",
                SubWindow, (IPTR) (AQPart_win_Main = WindowObject,
                    MUIA_PrefsEditor_CanTest, FALSE,
                    MUIA_PrefsEditor_CanSave, FALSE,
                    MUIA_Window_Title, (IPTR)"AROS Quick Part [v0.61] ..",
                    MUIA_Window_Activate, TRUE,
                    MUIA_Window_Width, MUIV_Window_Width_Visible(80),
                    MUIA_Window_Height, MUIV_Window_Height_Visible(80),
                    WindowContents, (IPTR) (AQPart_grp_Main = VGroup,
                        Child, (IPTR)(HGroup,
                            MUIA_Weight, 100,
                            Child, (IPTR)(VGroup,
                                VirtualFrame,
                                Child,(ScrollgroupObject,
                                    MUIA_Background, (IPTR)DEF_DRIVEPANE_BACK,
                                    MUIA_Scrollgroup_FreeHoriz, FALSE,
                                    MUIA_Scrollgroup_FreeVert, TRUE,
                                    MUIA_Scrollgroup_Contents,(IPTR)(VirtgroupObject,
                                        MUIA_Background, (IPTR)DEF_DRIVEPANE_BACK,
                                        Child, HGroup,
                                            Child,(IPTR)(RectangleObject, 
                                                MUIA_FixWidth, 10,
                                            End),
                                            Child, (IPTR)(AQPart_grp_Drives = VGroupV,
                                                MUIA_Group_SameWidth, FALSE,
                                                Child, (IPTR)(RectangleObject, 
                                                    MUIA_FixHeight, 10,
                                                End),
                                                Child, (IPTR)(AQPart_VOID = HVSpace),
                                            End),
                                            Child, (IPTR)(RectangleObject, 
                                                MUIA_FixWidth, 10,
                                            End),
                                        End,
                                    End),
                                End),
                                Child, (IPTR)(AQPart_panel_Pending = HGroup,
                                    MUIA_ShowMe, FALSE,
                                    Child, (IPTR)(VGroup,
                                        Child, (IPTR)(HGroup,
                                            MUIA_FixHeight, 10,
                                            MUIA_InnerLeft, 1,
                                            MUIA_InnerTop, 1,
                                            Child, (IPTR)(NewObject(mcc_qptxt->mcc_Class, NULL,
                                                MUIA_Font, MUIV_Font_Tiny,
                                                MUIA_Text_SetMin, FALSE,
                                                MUIA_Text_SetMax, FALSE,
                                                MUIA_Text_Contents, (IPTR)"Pending operations..",
                                            TAG_DONE)),
                                            Child, RectangleObject,
                                            End,
                                        End),
										Child,(ScrollgroupObject,
											MUIA_Scrollgroup_FreeHoriz, TRUE,
											MUIA_Scrollgroup_FreeVert, FALSE,
											MUIA_FixHeight, 60,
											MUIA_Scrollgroup_Contents,(IPTR)(VirtgroupObject,
												Child, (IPTR)(AQPart_grp_Pending = NewObject(mcc_qpopgrp->mcc_Class, NULL,
												TAG_DONE)),
											End),
										End),
                                    End),
                                    Child, (IPTR)(VGroup,
                                        MUIA_Weight, 0,
                                        MUIA_Group_SameWidth, TRUE,
                                        Child, (IPTR) (AQPart_but_Use    = ImageButton((IPTR) "Apply", "THEME:Images/Gadgets/Use")),
                                        Child, (IPTR) (ImageButton((IPTR) "Undo", "THEME:Images/Gadgets/Revert")),
                                        Child, (IPTR) (AQPart_but_Cancel = ImageButton(__(WORD_Cancel), "THEME:Images/Gadgets/Cancel")),
                                        Child, HVSpace,
                                    End),
                                End),
                            End),

                            Child, BalanceObject, End,

                            Child, (IPTR)(VGroup,
                                MUIA_Weight, 0,
                                MUIA_Group_SameHeight, FALSE,
                                Child, (IPTR)(VGroup,
                                    MUIA_VertWeight, 0,
                                    Child, (IPTR)(ColGroup(2),
                                        Child, (IPTR) AQPart_but_Help,
                                        Child, (IPTR) AQPart_but_Legend,
                                    End),
                                    Child, (IPTR)(RectangleObject, 
                                        MUIA_Rectangle_HBar, TRUE,
                                    End),
                                End),
                                Child, (IPTR)(RectangleObject, 
                                    MUIA_VertWeight, 0,
                                End),
                                Child, (IPTR)(HGroup,
                                    MUIA_VertWeight, 0,
                                    Child, (IPTR) AQPart_but_Create,
                                    Child, (IPTR) AQPart_but_Delete,
                                End),
                                Child, (IPTR)(AQPart_grp_Info = VGroup,
                                    MUIA_VertWeight, 100,
                                    MUIA_Group_SameHeight, FALSE,
                                    Child, (IPTR)AQPart_txt_Info,
                                    Child, (IPTR)(AQPart_spcr_Info = RectangleObject, 
                                        MUIA_VertWeight, 100,
                                    End),
                                End),
                                Child, (IPTR)(HGroup,
                                    MUIA_VertWeight, 0,
                                    Child, (IPTR) LLabel( _(MSG_SCALE_DRIVES)),
                                    Child, (IPTR) AQPart_chk_Scale,
                                End),
                            End),
                        End),
                    End),
                End),

                SubWindow, (IPTR)(AQPart_win_Progress = WindowObject,
                    MUIA_Window_Title, __(WORD_ProgressTitle),
                    MUIA_Window_CloseGadget, FALSE,
                    MUIA_Window_Activate, FALSE,
                    WindowContents, (IPTR)(VGroup,
                        Child, (IPTR) (AQPart_txt_Progress = LLabel("...")),
                        Child, (IPTR) HVSpace,
                        Child, (IPTR)(VGroup, GaugeFrame, Child, AQPart_gge_Progress, End),
                        Child, (IPTR) ScaleObject, End,
                        Child, (IPTR) HVSpace,
                    End),
                End),
                          
                /*		SubWindow, (IPTR)(AQPart_win_Welcome = WindowObject,
                                        MUIA_Window_Title, __(MSG_WelcomeTitle),
                                        MUIA_Window_CloseGadget, FALSE,
                                        MUIA_Window_Activate, FALSE,
                                        WindowContents, (IPTR)(VGroup,
                                                Child, (IPTR)(HGroup,
                                                        Child, (IPTR)(VGroup,
                                                                MUIA_Background, MUII_SHADOW,    
                                                
                                                                Child, (IPTR)(ImageObject,
                                                                        MUIA_Frame,             MUIV_Frame_None,
                                                                        MUIA_Image_Spec, (IPTR) "3:"DEF_LOGO_IMAGE,
                                                                End),
                                                                Child, (IPTR) HVSpace,
                                                        End),
                                                        Child, (IPTR)(VGroup,
                                                                Child, (IPTR)(TextObject,
                                                                        MUIA_Text_PreParse, (IPTR)"" MUIX_B MUIX_C,
                                                                        MUIA_Text_Contents, __(MSG_WelcomeTitle),
                                                                End),
                                                                Child, (IPTR)HVSpace,
                                                                Child, (IPTR)LLabel(_(MSG_WelcomeText)),
                                                                Child, (IPTR)HVSpace,
                                                        End),
                                                End),
                                                Child, (IPTR)HGroup,
                                                Child, (IPTR)HVSpace,
                                                Child, (IPTR)(AQPart_but_proceed = ImageButton(__(WORD_Proceed), "THEME:Images/Gadgets/Prefs/Use")),
                                                Child, (IPTR)(AQPart_but_Cancel2 = ImageButton(__(WORD_Cancel), "THEME:Images/Gadgets/Prefs/Cancel")),
                                        End),
                                End),*/

                SubWindow, (IPTR)(AQPart_win_About = AboutWindowObject,
                    MUIA_AboutWindow_Authors, (IPTR) TAGLIST
                    (
                        SECTION
                        (
                            SID_PROGRAMMING,
                            NAME("Nick \"Kalamatee\" Andrews")
                        )
                    ),
                End),


            TAG_DONE);

            if (AQPart_App)
            {
                ULONG   sigs = 0;

                SwitchHelphook.h_Entry = (APTR)switchBubbleHelp;

                D(bug("[Qp.Main] Window Open\n"));
                /*
                    Setup defaults and open our display
                */
                /* set up our hooks */

                //DoMethod(AQPart_but_proceed, MUIM_Notify, MUIA_Pressed, TRUE, (IPTR) AQPart_win_Welcome, 3, MUIM_Set, MUIA_Window_Open, FALSE);
//                DoMethod(AQPart_but_proceed, MUIM_Notify, MUIA_Pressed, TRUE, (IPTR) AQPart_win_Main, 3, MUIA_Window_Sleep, TRUE);
//                DoMethod(AQPart_but_Cancel2, MUIM_Notify, MUIA_Pressed, TRUE, (IPTR) AQPart_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                DoMethod(AQPart_but_Legend, MUIM_Notify, MUIA_Pressed, FALSE, QPart_AppWin_Legend, 3, MUIM_Set, MUIA_Window_Open, TRUE);

                DoMethod(AQPart_but_Help, MUIM_Notify, MUIA_Pressed, TRUE, AQPart_but_Help, 3, MUIM_CallHook, &SwitchHelphook, MUIV_TriggerValue);

                /* Bubble help toggle always has help */
                set(AQPart_but_Help, MUIA_ShortHelp, __(MSG_BUBBLEHELP));

                set(AQPart_win_Main, MUIA_Window_Open, TRUE);

                DoMethod(AQPart_but_Cancel, MUIM_Notify, MUIA_Pressed, TRUE, (IPTR) AQPart_win_Main, 3, MUIM_Set, MUIA_Window_CloseRequest, TRUE);

                /* CHANGE THIS LINE!! - This should cause our update method - forcing partitions to be written to disk! */
                //DoMethod(AQPart_but_Use, MUIM_Notify, MUIA_Pressed, TRUE, (IPTR) AQPart_win_Main, 3, MUIM_Set, MUIA_Window_CloseRequest, TRUE);

                set(AQPart_but_Create, MUIA_Disabled, TRUE);
                DoMethod(AQPart_but_Delete, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) AQPart_App, 1, MUIM_QPart_ccApp_DeleteActive);
                set(AQPart_but_Delete, MUIA_Disabled, TRUE);

                DoMethod(AQPart_win_Main, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,  AQPart_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                /*
                    Now lets detect available drives
                */

                set(AQPart_txt_Progress, MUIA_Text_Contents, __(MSG_SEARCHING));
                set(AQPart_gge_Progress, MUIA_Gauge_Current, 0);
                set(AQPart_win_Progress, MUIA_Window_Open, TRUE);

                set(AQPart_win_Main, MUIA_Window_Sleep, FALSE);

                NEWLIST((struct List *)&AQPart_driveList);
                NEWLIST((struct List *)&AQPart_partList);

                struct VolumeResource__StorageDevice_RecordCommon *VRM_Device = NULL;

                int diskCount;
                if ((diskCount = FindDisks()) > 0)
                {
                    if (DoMethod(AQPart_grp_Drives, MUIM_Group_InitChange))
                    {
                        AQPart_VOID = HVSpace;

                        DoMethod(AQPart_grp_Drives, OM_ADDMEMBER, AQPart_VOID);

                        DoMethod(AQPart_grp_Drives, MUIM_Group_ExitChange);
                    }
                    set(AQPart_VOID, MUIA_HorizDisappear, 1);
                }
                /* available drives detected */
                set(AQPart_gge_Progress, MUIA_Gauge_Current, 100);
                set(AQPart_win_Main, MUIA_Window_Sleep, TRUE);
                D(bug("[Qp.main] Found %d Nodes\n",nodeCount));

                set(AQPart_chk_Scale, MUIA_CycleChain, 1);
                set(AQPart_chk_Scale, MUIA_Selected,FALSE);

                if ( diskCount < 2 ) set(AQPart_chk_Scale, MUIA_Disabled, TRUE);

                set(AQPart_win_Progress, MUIA_Window_Open, FALSE);

                //set( AQPart_win_Welcome, MUIA_Window_Open, TRUE);
                set(AQPart_win_Main, MUIA_Window_Sleep, FALSE);

                while (DoMethod(AQPart_App, MUIM_Application_NewInput, (IPTR) &sigs) != MUIV_Application_ReturnID_Quit)
                {
                    if (sigs)
                    {
                        sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

                        if (sigs & SIGBREAKF_CTRL_C)
                            break;
                        if (sigs & SIGBREAKF_CTRL_D)
                            break;
                    }
                }
                MUI_DisposeObject(AQPart_App);
            }
            else
            {
                printf( _(MSG_GUIFAILED));
                error = RETURN_ERROR;
            }
        }
        CloseLibrary(MUIMasterBase);

        CloseLibrary(ExpansionBase);
    }

    FreeSignal(AQPart_signal_PART);

    return error;
}
