/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#define DEBUG 1

#include <aros/debug.h>

#include <libraries/mui.h>

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <gadgets/colorwheel.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "install.h"

#define kBufSize  1024
#define kSrcDir  "SYS:"
#define kDstDir  "DH0:"
#define kDstDev  "DH0"
#define kDstName "AROS"

#define DEF_INSTALL_IMAGE           "IMAGES:Logos/install.logo"
#define DEF_BACK_IMAGE              "IMAGES:Logos/install.logo"
#define DEF_LIGHTBACK_IMAGE         "IMAGES:Logos/install.logo"

//extern ULONG InitTask(void);
int CopyDirArray( Object *self, struct Install_DATA* data, TEXT *copy_files[]);
int CreateDestDIR( Class *CLASS, Object *self, TEXT *dest_dir);

IPTR Install__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);

    struct Install_DATA* data = INST_DATA(CLASS, self);

    /**/
    //data->IOTask       = InitTask(); /** LAUNCH THE IO TASK! **/
    
/**/
/* We will generate this info shortly */
    data->welcomeMsg    = (APTR) GetTagData (MUIA_WelcomeMsg,           (ULONG) NULL, message->ops_AttrList);
    data->doneMsg       = (APTR) GetTagData (MUIA_FinishedMsg,           (ULONG) NULL, message->ops_AttrList);

    data->page          = (APTR) GetTagData (MUIA_Page,                 (ULONG) NULL, message->ops_AttrList);
    data->gauge1        = (APTR) GetTagData (MUIA_Gauge1,               (ULONG) NULL, message->ops_AttrList);
    data->gauge2        = (APTR) GetTagData (MUIA_Gauge2,               (ULONG) NULL, message->ops_AttrList);
    data->label         = (APTR) GetTagData (MUIA_Install,              (ULONG) NULL, message->ops_AttrList);

    data->installer     = (APTR) GetTagData (MUIA_OBJ_Installer,        (ULONG) NULL, message->ops_AttrList);

    data->window        = (APTR) GetTagData (MUIA_OBJ_Window,           (ULONG) NULL, message->ops_AttrList);
    data->contents      = (APTR) GetTagData (MUIA_OBJ_WindowContent,    (ULONG) NULL, message->ops_AttrList);

    data->pagetitle     = (APTR) GetTagData (MUIA_OBJ_PageTitle,        (ULONG) NULL, message->ops_AttrList);
    data->pageheader    = (APTR) GetTagData (MUIA_OBJ_PageHeader,       (ULONG) NULL, message->ops_AttrList);

    data->actioncurrent = (APTR) GetTagData (MUIA_OBJ_CActionStrng,     (ULONG) NULL, message->ops_AttrList);
    data->back          = (APTR) GetTagData (MUIA_OBJ_Back,             (ULONG) NULL, message->ops_AttrList);
    data->proceed       = (APTR) GetTagData (MUIA_OBJ_Proceed,          (ULONG) NULL, message->ops_AttrList);
    data->cancel        = (APTR) GetTagData (MUIA_OBJ_Cancel,           (ULONG) NULL, message->ops_AttrList);
/**/
    data->CWindow       = (APTR) GetTagData (MUIA_OBJ_CWindow,          (ULONG) NULL, message->ops_AttrList);
    data->CText         = (APTR) GetTagData (MUIA_OBJ_CText,            (ULONG) NULL, message->ops_AttrList);
    data->COK           = (APTR) GetTagData (MUIA_OBJ_COK,              (ULONG) NULL, message->ops_AttrList);
    data->CCancel       = (APTR) GetTagData (MUIA_OBJ_CCancel,          (ULONG) NULL, message->ops_AttrList);
/**/
    data->options       = (APTR) GetTagData (MUIA_List_Options,         (ULONG) NULL, message->ops_AttrList);

/****/
    get( data->window, MUIA_Window_Width, &data->cur_width);
    get( data->window, MUIA_Window_Height, &data->cur_height);

    set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
    set(data->back, MUIA_Disabled, TRUE);

    data->nextStage     = EInstallOptionsStage;

    data->inst_success  = FALSE;
    data->disable_back  = FALSE;
    
    data->drive_set     = (BOOL)DoMethod(self, MUIM_DH0);

    DoMethod(data->proceed, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1, MUIM_NextStep);
    DoMethod(data->back, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1, MUIM_PrevStep);
    DoMethod(data->cancel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) self, 1, MUIM_CancelInstall);

    DoMethod(self, MUIM_Notify, MUIA_InstallComplete, TRUE, (IPTR) self, 1, MUIM_Reboot);
    return (IPTR) self;
}

/* make page */
/**/

BOOL FindPartition(struct PartitionHandle *root, CONST_STRPTR nameToFind)
{
    struct PartitionHandle *partition = NULL;
    BOOL   success = FALSE;
    
    ForeachNode(&root->table->list, partition)
    {
        if (OpenPartitionTable(partition) == 0)
        {
            success = FindPartition(partition, nameToFind);
            ClosePartitionTable(partition);
        
            if (success) break;
        }
        else
        {
            TEXT name[32] = {0};
            
            GetPartitionAttrsTags
            (
                partition, PT_NAME, (IPTR) name, TAG_DONE
            );
            
            if (stricmp(name, nameToFind) == 0)
            {
                success = TRUE;
                break;
            }
        }
    }
    
    return success;
}

IPTR Install__MUIM_DH0
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct PartitionHandle *root;
    BOOL                    result = FALSE;
    
    if((root = OpenRootPartition("ide.device", 0)) != NULL)
    {
        if (OpenPartitionTable(root) == 0)
        {
            result = FindPartition(root, kDstDev);            
            ClosePartitionTable(root);
        }
        
        CloseRootPartition(root);
    }
    
    return result;
}

IPTR Install__MUIM_NextStep
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA*    data = INST_DATA(CLASS, self);
    IPTR                    this_page,next_stage;

    get(data->page,MUIA_Group_ActivePage, &this_page);

    if ( this_page == data->nextStage ) set(self, MUIA_InstallComplete, TRUE);  //ALL DONE!!

    set(data->back, MUIA_Disabled, (BOOL)data->disable_back);

    next_stage = data->nextStage;

    set(data->back, MUIA_Selected, FALSE);
    set(data->proceed, MUIA_Selected, FALSE);
    set(data->cancel, MUIA_Selected, FALSE);

    switch(data->nextStage)
    {
    case EInstallOptionsStage:
        if(data->drive_set)
        {
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgInstallOptions);
            data->nextStage = EInstallMessageStage;
        }
        else
        {
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgPartitionDH0);
            next_stage = EMessageStage;
            data->nextStage = EPartitioningStage;
        }
        break;

    case EInstallMessageStage:
        set(data->welcomeMsg, MUIA_Text_Contents, KMsgBeginWithPartition);
        data->nextStage = EInstallStage;
        next_stage =  EMessageStage;
        break;

    case EPartitioningStage:
        data->disable_back = TRUE;
        set(data->page,MUIA_Group_ActivePage, EPartitioningStage);

        if(DoMethod(self, MUIM_Partition) != RETURN_OK)
        {
            exit(RETURN_FAIL);
        }

        next_stage = EDoneStage;
        set(data->doneMsg,MUIA_Text_Contents,KMsgDoneReboot);
        set(data->back, MUIA_Disabled, TRUE);
        set(data->cancel, MUIA_Disabled, TRUE);
        data->nextStage = EDoneStage;
        break;

    case EInstallStage:
        data->disable_back = TRUE;
        set(data->page,MUIA_Group_ActivePage, EInstallStage);

        DoMethod(self, MUIM_Install);

        next_stage = EDoneStage;
        set(data->back, MUIA_Disabled, TRUE);
        set(data->cancel, MUIA_Disabled, TRUE);
        data->nextStage = EDoneStage;
        break;

    default:
        break;
    }

    //DoMethod(data->contents, MUIM_Group_InitChange);
    //DoMethod(data->contents, MUIM_Group_ExitChange);

    set(data->page,MUIA_Group_ActivePage, next_stage);

    return 0;
}

IPTR Install__MUIM_PrevStep
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    IPTR    this_page;

    get(data->page,MUIA_Group_ActivePage, &this_page);
    set(data->back, MUIA_Selected, FALSE);
    set(data->proceed, MUIA_Selected, FALSE);
    set(data->cancel, MUIA_Selected, FALSE);

    set(data->back, MUIA_Disabled, (BOOL)data->disable_back);
    data->nextStage = this_page;

    //DoMethod(data->contents, MUIM_Group_InitChange);

    switch(this_page)
    {
        case EInstallOptionsStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
            set(data->page,MUIA_Group_ActivePage, EMessageStage);
            set(data->back, MUIA_Disabled, TRUE);
            break;
        case EInstallMessageStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgInstallOptions);
            set(data->page,MUIA_Group_ActivePage, EInstallOptionsStage);
            break;
        case EPartitioningStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgInstallOptions);
            set(data->page,MUIA_Group_ActivePage, EInstallOptionsStage);
            break;
        case EInstallStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgInstallOptions);
            set(data->page,MUIA_Group_ActivePage, EInstallOptionsStage);
            break;
        case EDoneStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
            set(data->page,MUIA_Group_ActivePage, EInstallStage);
            break;
        case EMessageStage:
            set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome);
            set(data->page,MUIA_Group_ActivePage, EMessageStage);
            set(data->back, MUIA_Disabled, TRUE);
            data->nextStage = EInstallOptionsStage;
            break;
        default:
            break;
    }

    //DoMethod(data->contents, MUIM_Group_ExitChange);
    //DoMethod(data->installer,MUIM_Application_InputBuffered);
    return TRUE;
}

IPTR Install__MUIM_CancelInstall
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR         this_page;
    CONST_STRPTR cancelmessage;

    get(data->page,MUIA_Group_ActivePage, &this_page);
    
    get(data->back, MUIA_Disabled, &data->status_back);
    get(data->proceed, MUIA_Disabled, &data->status_proceed);
    get(data->cancel, MUIA_Disabled, &data->status_cancel);

    set(data->back, MUIA_Selected, FALSE);
    set(data->back, MUIA_Disabled, TRUE);

    set(data->proceed, MUIA_Selected, FALSE);
    set(data->proceed, MUIA_Disabled, TRUE);

    set(data->cancel, MUIA_Selected, FALSE);
    set(data->cancel, MUIA_Disabled, TRUE);

    if (this_page==EInstallOptionsStage)
    {
        set(data->options->opt_partition,MUIA_Disabled,TRUE);
        set(data->options->opt_locale,MUIA_Disabled,TRUE);
        set(data->options->opt_copycore,MUIA_Disabled,TRUE);
        set(data->options->opt_copyextra,MUIA_Disabled,TRUE);
        set(data->options->opt_development,MUIA_Disabled,TRUE);
        set(data->options->opt_bootloader,MUIA_Disabled,TRUE);
        set(data->options->opt_reboot,MUIA_Disabled,TRUE);
    }

    switch(this_page)
    {
    case EInstallOptionsStage:
    case EMessageStage:    
    case EInstallMessageStage:
        cancelmessage = KMsgCancelOK;
        break;
    case EPartitioningStage:
    case EInstallStage:
    case EDoneStage:
        cancelmessage = KMsgCancelDanger;
        break;
    default:
        break;
    }

    if
    (
        !MUI_RequestA
        (
            data->installer, data->window, 0, "Cancel Installation..", 
            "*Continue Install|Cancel Install", cancelmessage, NULL
        )
    )
    {
        DoMethod(self, MUIM_QuitInstall);
    }
    else
    {
        DoMethod(self, MUIM_ContinueInstall);
    }

    return 0;
}

IPTR Install__MUIM_ContinueInstall
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    IPTR    this_page;

    get(data->page,MUIA_Group_ActivePage, &this_page);

    if (!(BOOL)data->disable_back) set(data->back, MUIA_Disabled, data->status_back);
    else set(data->back, MUIA_Disabled, TRUE);
    set(data->back, MUIA_Selected, FALSE);

    set(data->proceed, MUIA_Disabled, data->status_proceed);
    set(data->proceed, MUIA_Selected, FALSE);

    set(data->cancel, MUIA_Disabled, data->status_cancel);
    set(data->cancel, MUIA_Selected, FALSE);

    if (this_page==EInstallOptionsStage)
    {
        set(data->options->opt_partition,MUIA_Disabled,FALSE);
        set(data->options->opt_locale,MUIA_Disabled,FALSE);
        set(data->options->opt_copycore,MUIA_Disabled,FALSE);
        set(data->options->opt_copyextra,MUIA_Disabled,FALSE);
        set(data->options->opt_development,MUIA_Disabled,FALSE);
        set(data->options->opt_bootloader,MUIA_Disabled,FALSE);
        set(data->options->opt_reboot,MUIA_Disabled,FALSE);
    }

    return 0;
}

IPTR Install__MUIM_QuitInstall
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);

    if ( data->inst_success ==  MUIV_Inst_InProgress)
    {
        data->inst_success = MUIV_Inst_Cancelled;

        DoMethod(self,MUIM_Reboot);
    }
    
    return 0;
}

/* ****** FUNCTION IS CALLED BY THE PROCEDURE PROCESSOR

            IT LAUNCHES THE NECESSARY FUNCTION TO PERFORM WHATEVER IS BEING ASKED TO DO
*/

IPTR Install__MUIM_DispatchInstallProcedure
(     
    Class *CLASS, Object *self, Msg message 
)
{
    // struct Install_DATA* data = INST_DATA(CLASS, self);

    return 0;
}

IPTR Install__MUIM_Partition
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);
    IPTR   tmp = 0;

    if ( data->inst_success ==  MUIV_Inst_InProgress)
    {
        set(data->back, MUIA_Disabled, TRUE);
        set(data->proceed, MUIA_Disabled, TRUE);

        tmp = SystemTagList("C:Partition FORCE QUIET", NULL);

        set(data->proceed, MUIA_Disabled, FALSE);
    }
    
    return tmp;
}

IPTR Install__MUIM_Install
(     
 Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA *data = INST_DATA(CLASS, self);

    BPTR lock   = NULL;
    IPTR option = FALSE;

    set(data->back, MUIA_Disabled, TRUE);
    set(data->proceed, MUIA_Disabled, TRUE);

    set(data->pagetitle,MUIA_Text_Contents, "Installing AROS ...");

    get(data->options->opt_partition,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        // Formatting
        D(bug("[INSTALLER] Formatting...\n"));
        DoMethod(self, MUIM_Format);
    }

    DoMethod(data->installer,MUIM_Application_InputBuffered);

    get(data->options->opt_locale,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        D(bug("[INSTALLER] Launching Locale Prefs...\n"));

#define localeFile_path     "Prefs/Locale"
#define inputFile_path      "Prefs/Input"
#define prefssrc_path       "ENV:SYS"
#define prefs_path          "Prefs/Env-Archive/SYS"

        ULONG srcLen = strlen(kSrcDir);
        ULONG dstLen = strlen(kDstDir);
        ULONG envsrcLen = strlen(prefssrc_path);
        ULONG envdstLen = strlen(prefs_path);

        ULONG localeFileLen = srcLen + strlen(localeFile_path) + 2;
        ULONG inputFileLen = srcLen + strlen(inputFile_path) + 2;

        //ULONG localesrcPFileLen = envsrcLen + strlen("locale.prefs") + 2;
        ULONG localePFileLen = dstLen + envdstLen + strlen("locale.prefs") + 4;

        //ULONG inputsrcPFileLen = envsrcLen + strlen("input.prefs") + 2;
        ULONG inputPFileLen = dstLen + envdstLen + strlen("input.prefs") + 4;

        TEXT envDstDir[dstLen + envdstLen];

        TEXT localeFile[localeFileLen];
        TEXT localesrcPFile[localePFileLen];
        TEXT localePFile[localePFileLen];
        TEXT inputFile[inputFileLen];
        TEXT inputsrcPFile[inputPFileLen];
        TEXT inputPFile[inputPFileLen];

        CopyMem(kDstDir,    envDstDir,         dstLen + 1);
        CopyMem(kSrcDir,    localeFile,        srcLen + 1);
        CopyMem(prefssrc_path,  localesrcPFile,    envsrcLen + 1);
        CopyMem(kDstDir,    localePFile,       dstLen + 1);
        CopyMem(kSrcDir,    inputFile,         srcLen + 1);
        CopyMem(prefssrc_path,  inputsrcPFile,     envsrcLen + 1);
        CopyMem(kDstDir,    inputPFile,        dstLen + 1);

        AddPart(localeFile, inputFile_path, inputFileLen);

        AddPart(localesrcPFile, "locale.prefs", envsrcLen + sizeof("locale.prefs") + 2);

        AddPart(localePFile, prefs_path, dstLen + envdstLen + 2);
        AddPart(localePFile, "locale.prefs", dstLen + envdstLen + sizeof("locale.prefs") + 4);

        AddPart(inputFile, localeFile_path, localeFileLen);

        AddPart(inputsrcPFile, "input.prefs", envsrcLen + sizeof("input.prefs") + 2);

        AddPart(inputPFile, prefs_path,  dstLen + envdstLen + 2);
        AddPart(inputPFile, "input.prefs", dstLen + envdstLen + sizeof("input.prefs") + 4);

        Execute(localeFile, NULL, NULL);

        DoMethod(data->installer,MUIM_Application_InputBuffered);

        Execute(inputFile, NULL, NULL);

        DoMethod(data->installer,MUIM_Application_InputBuffered);

        D(bug("[INSTALLER] Copying Locale Settings...\n"));

        //create the dirs "Prefs","Prefs/Env-Archive" and "Prefs/Env-Archive/SYS"
        AddPart(envDstDir, "Prefs", dstLen + sizeof("Prefs") +2);
        BPTR bootDirLock = CreateDir(envDstDir);
        if(bootDirLock != NULL) UnLock(bootDirLock);
        else
        {
createfaild:
            D(bug("[INSTALLER] Failed to create %s dir!!\n",envDstDir));
            data->inst_success = MUIV_Inst_Failed;
            return 0;
        }

        AddPart(envDstDir, "Env-Archive", dstLen + sizeof("Prefs") + sizeof("Env-Archive") + 4);
        bootDirLock = CreateDir(envDstDir);
        if(bootDirLock != NULL) UnLock(bootDirLock);
        else goto createfaild;

        AddPart(envDstDir, "SYS", dstLen + sizeof("Prefs") + sizeof("Env-Archive") + sizeof("SYS") + 6);
        bootDirLock = CreateDir(envDstDir);
        if(bootDirLock != NULL) UnLock(bootDirLock);
        else goto createfaild;

        //copy files "Prefs/Env-Archive/SYS/locale.prefs" "Prefs/Env-Archive/SYS/input.prefs"
        
        if ((lock = Lock(localesrcPFile, ACCESS_READ))!=NULL)
        {
            UnLock(lock);
            DoMethod(self, MUIM_CopyFile, localesrcPFile, localePFile);
        }
        
        if ((lock = Lock(inputsrcPFile, ACCESS_READ))!=NULL)
        {
            UnLock(lock);
            DoMethod(self, MUIM_CopyFile, inputsrcPFile, inputPFile);
        }
    }

    DoMethod(data->installer,MUIM_Application_InputBuffered);

    get(data->options->opt_copycore,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        TEXT     *core_dirs[((11+1)*2)] = 
        {
            "C",
            "C",

            "Classes",
            "Classes",

            "Devs",
            "Devs",

            "Fonts",
            "Fonts",

            "Libs",
            "Libs",

            "Locale",
            "Locale",

            "Prefs",
            "Prefs",
            
            "S",
            "S",

            "System",
            "System",

            "Tools",
            "Tools",

            "Utilities",
            "Utilities",

            NULL
        };

        // Copying Core system Files
        D(bug("[INSTALLER] Copying Core files...\n"));
        set(data->label, MUIA_Text_Contents, "Copying Core System files...");

        CopyDirArray( self, data, core_dirs);
    }

    DoMethod(data->installer,MUIM_Application_InputBuffered);

    get(data->options->opt_copyextra,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        TEXT     *extras_dirs[((2+1)*2)] = 
        {
            "Demos",
            "Demos",

            "Extras",
            "Extras",
            
            NULL
        };

        // Copying Extras
        D(bug("[INSTALLER] Copying Extras...\n"));
        set(data->label, MUIA_Text_Contents, "Copying Extra Software...");

        CopyDirArray( self, data, extras_dirs);
    }

    DoMethod(data->installer,MUIM_Application_InputBuffered);

    get(data->options->opt_development,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        ULONG srcLen = strlen(kSrcDir);
        ULONG developerDirLen = srcLen + strlen("Development") + 2;
        TEXT developerDir[srcLen + developerDirLen];

        CopyMem(kSrcDir, developerDir, srcLen + 1);
        AddPart(developerDir, "Development", strlen("Development")+1);

        if ((lock = Lock(developerDir, ACCESS_READ)) != NULL)
        {
            UnLock(lock);
            TEXT     *developer_dirs[((2+1)*2)] = 
            {
                "Development",
                "Development",
              
                "Tests",
                "Tests",

                NULL
            };

            // Copying Developer stuff
            D(bug("[INSTALLER] Copying Developer Files...\n"));
            set(data->label, MUIA_Text_Contents, "Copying Developer Files...");

            CopyDirArray( self, data, developer_dirs);
        }
        else D(bug("[INSTALLER] Couldnt locate Developer Files...\n"));
    }

    DoMethod(data->installer,MUIM_Application_InputBuffered);

    get(data->options->opt_bootloader,MUIA_Selected,&option);
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        int numgrubfiles = 3,file_count = 0;
        ULONG srcLen = strlen(kSrcDir);
        ULONG dstLen = strlen(kDstDir);

        TEXT     *grub_files[(3+1)*2] = 
        {
            "boot/aros-pc-i386.gz",
            "boot/aros-pc-i386.gz",

            "boot/grub/stage1",
            "boot/grub/stage1",

            "boot/grub/stage2_hdisk",
            "boot/grub/stage2",
            
            "boot/grub/menu.lst.DH0",
            "boot/grub/menu.lst",

            NULL
        };

        CreateDestDIR( CLASS, self, "boot" );
        CreateDestDIR( CLASS, self, "boot/grub" );

        // Installing GRUB
        D(bug("[INSTALLER] Installing Grub...\n"));
        set(data->label, MUIA_Text_Contents, "Installing Grub...");
        set(data->pageheader, MUIA_Text_Contents, KMsgBootLoader);

        set(data->gauge2, MUIA_Gauge_Current, 0);

        set(data->label, MUIA_Text_Contents, "Copying BOOT files...");

        while (grub_files[file_count]!=NULL)
        {
            ULONG newSrcLen = srcLen + strlen(grub_files[file_count]) + 2;
            ULONG newDstLen = dstLen + strlen(grub_files[file_count+1]) + 2;
            
            TEXT srcFile[newSrcLen];
            TEXT dstFile[newDstLen];
            
            CopyMem(kSrcDir, srcFile, srcLen + 1);
            CopyMem(kDstDir, dstFile, dstLen + 1);
            AddPart(srcFile, grub_files[file_count], newSrcLen);
            AddPart(dstFile, grub_files[file_count+1], newDstLen);

            set(data->actioncurrent, MUIA_Text_Contents, srcFile);
            DoMethod(data->installer,MUIM_Application_InputBuffered);

            DoMethod(self, MUIM_CopyFile, srcFile, dstFile);

            set(data->gauge2, MUIA_Gauge_Current, ((100/(numgrubfiles +1)) * (file_count/2)));

            file_count += 2;
        }

        Execute("C:install-i386-pc DEVICE ide.device UNIT 0 KERNEL DH0:boot/aros-pc-i386.gz GRUB DH0:boot/grub", NULL, NULL);
        set(data->gauge2, MUIA_Gauge_Current, 100);
    }
  
    set(data->proceed, MUIA_Disabled, FALSE);
    
    return 0;
}

IPTR Install__MUIM_RefreshWindow
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    ULONG   cur_width,cur_height;

    get( data->window, MUIA_Window_Width, &cur_width);
    get( data->window, MUIA_Window_Height, &cur_height);
    
    if ((data->cur_width != cur_width)||(data->cur_height != cur_height))
    {
        DoMethod(data->contents,MUIM_Hide);
        DoMethod(data->contents,MUIM_Layout);
        DoMethod(data->contents,MUIM_Show);
    }
    else MUI_Redraw(data->contents, MADF_DRAWOBJECT);

    return 0;
}

int CreateDestDIR( Class *CLASS, Object *self, TEXT *dest_dir) 
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    ULONG   dstLen      = strlen(kDstDir);
    ULONG   destDirLen  = dstLen + strlen(dest_dir) + 2;
    TEXT    newDestDir[destDirLen];
  
    CopyMem(kDstDir, newDestDir, dstLen + 1);
    AddPart(newDestDir, dest_dir, destDirLen);

    BPTR destDirLock = Lock(newDestDir, ACCESS_READ);
    if (destDirLock == NULL)
    {
        destDirLock = CreateDir(newDestDir);              /* create the newDestDir dir */
        if(destDirLock == NULL) 
        {
            D(bug("[INSTALLER] CreateDestDIR: Failed to create '%s' dir!!\n",newDestDir));
            data->inst_success = MUIV_Inst_Failed;
            return FALSE;
        }
        D(bug("[INSTALLER] CreateDestDIR: Created dest dir '%s'\n",newDestDir));
    }
    else 
    {
        D(bug("[INSTALLER] CreateDestDIR: Dir '%s' already exists ..\n",newDestDir));
    }
    
    UnLock(destDirLock);

    return TRUE;
}

int CopyDirArray( Object *self, struct Install_DATA* data, TEXT *copy_files[]) 
{
        int         numdirs = 0,dir_count = 0;
        int         noOfFiles = 0;
        BPTR        lock = NULL;
        ULONG       srcLen = strlen(kSrcDir);
        ULONG       dstLen = strlen(kDstDir);

        set(data->gauge2, MUIA_Gauge_Current, 0);

        while (copy_files[numdirs]!=NULL)
        {
            numdirs += 1;
        }
        numdirs = (numdirs - 1)/2;

        while (copy_files[dir_count]!=NULL)
        {
            ULONG newSrcLen = srcLen + strlen(copy_files[dir_count]) + 2;
            ULONG newDstLen = dstLen + strlen(copy_files[dir_count+1]) + 2;
            
            TEXT srcDirs[newSrcLen + strlen(".info") ];
            TEXT dstDirs[newDstLen + strlen(".info")];
            
            CopyMem(kSrcDir, srcDirs, srcLen + 1);
            CopyMem(kDstDir, dstDirs, dstLen + 1);
            AddPart(srcDirs, copy_files[dir_count], newSrcLen);
            AddPart(dstDirs, copy_files[dir_count+1], newDstLen);

            set(data->actioncurrent, MUIA_Text_Contents, srcDirs);

            if ((noOfFiles = DoMethod(self, MUIM_MakeDirs, srcDirs, dstDirs))==0)
            {
                data->inst_success = MUIV_Inst_Failed;
                D(bug("[INSTALLER] Failed to create %s...\n",dstDirs));
            }

            /* OK Now copy the contents */
            noOfFiles += DoMethod(self, MUIM_CopyFiles, srcDirs, dstDirs, noOfFiles, 0);

            /* check if folder has an icon */
            CopyMem(".info", srcDirs + strlen(srcDirs) , strlen(".info") + 1);
            CopyMem(".info", dstDirs + strlen(dstDirs) , strlen(".info") + 1);
            if ((lock = Lock(srcDirs, ACCESS_READ)) != NULL)
            {
                UnLock(lock);
                DoMethod(self, MUIM_CopyFile, srcDirs, dstDirs);
            }

            set(data->gauge2, MUIA_Gauge_Current, ((100/(numdirs +1)) * (dir_count/2)));
            /* Folder copied =) */
            dir_count += 2;
        }

    return dir_count;   /* Return no. of succesfully copied dir's */
}

BOOL FormatPartition(CONST_STRPTR device, CONST_STRPTR name)
{
    BOOL success = FALSE;
    
    if (Inhibit(device, DOSTRUE))
    {
        success = Format(device, name, ID_FFS_DISK);
        Inhibit(device, DOSFALSE);
    }
    
    return success;
}

IPTR Install__MUIM_Format
(     
 Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA *data    = INST_DATA(CLASS, self);
    BOOL                 success = FALSE;
    BPTR                 lock;
    
    set(data->label, MUIA_Text_Contents, "Formatting...");
    set(data->gauge2, MUIA_Gauge_Current, 0);
    
    /* Format DH0: */
    success = FormatPartition(kDstDir, kDstName);
    
    /* Format DH1:, if it's not already formated */
    if ((lock = Lock("DH1:", ACCESS_READ)) != NULL)
    {
        UnLock(lock);
    }
    else
    {
        FormatPartition("DH1:", "Work");
    }
    
    if (success) set(data->gauge2, MUIA_Gauge_Current, 100);
    
    return success;
}

IPTR Install__MUIM_MakeDirs
(
    Class *CLASS, Object *self, struct MUIP_Dir* message 
)
{
    struct Install_DATA *data    = INST_DATA(CLASS, self);

    UBYTE buffer[4096];
    struct ExAllData *ead = (struct ExAllData*)buffer;
    struct ExAllData *oldEad = ead;
    struct ExAllControl  *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    BPTR lock = Lock(message->dstDir, SHARED_LOCK);     /* check the dest dir exists */
    if(lock == 0)
    {
        BPTR dstLock = CreateDir(message->dstDir);      /* no, so create it */
        if(dstLock != NULL) UnLock(dstLock);
        else
        {
            D(bug("[INSTALLER] Failed to create dest dir: %s (Error: %d)\n", message->dstDir, IoErr()));
            data->inst_success = MUIV_Inst_Failed;
            return 0;
        }
    }
    else
    {
        UnLock(lock);
        lock = 0;
    }

    lock = Lock(message->srcDir, SHARED_LOCK);          /* get the source dir */
    if(lock == 0)
    {
        D(bug("[INSTALLER] Failed to lock dir when making the dirs: %s (Error: %d)\n", message->srcDir, IoErr()));
        data->inst_success = MUIV_Inst_Failed;
        return 0;
    }

    LONG noOfFiles=0;
    BOOL  loop;

    D(bug("[INSTALLER] Locked and loaded\n"));
    
    do
    {
        ead = oldEad;
        loop = ExAll(lock, ead, sizeof(buffer), ED_COMMENT, eac);

        if(!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
        {
            break;
        }
        
        if(eac->eac_Entries != 0)
        {
            do
            {
                D(bug("[INSTALLER] Doin the entries: %d\n", ead->ed_Type));

                switch(ead->ed_Type)
                {
                    default:
                        D(bug("[INSTALLER] Type: %d\tName: %s\n", ead->ed_Type, ead->ed_Name));
                        break;
                    case ST_FILE:
                        noOfFiles++;
                        break;
                    case ST_USERDIR:
                    {
                        ULONG srcLen = strlen(message->srcDir);
                        ULONG dstLen = strlen(message->dstDir);
                        ULONG newSrcLen = srcLen + strlen(ead->ed_Name) + 2;
                        ULONG newDstLen = dstLen + strlen(ead->ed_Name) + 2;

                        TEXT srcDir[newSrcLen];
                        TEXT dstDir[newDstLen];

                        CopyMem(message->srcDir, srcDir, srcLen + 1);
                        CopyMem(message->dstDir, dstDir, dstLen + 1);
                        if(AddPart(srcDir, ead->ed_Name, newSrcLen) && AddPart(dstDir, ead->ed_Name, newDstLen))
                        {
                            D(bug("[INSTALLER] R: %s -> %s \n", srcDir, dstDir));
                            BPTR dirLock = CreateDir(dstDir);
                            if(dirLock != NULL) UnLock(dirLock);
                            noOfFiles += DoMethod(self, MUIM_MakeDirs, srcDir, dstDir);
                        }
                        else
                        {
                            data->inst_success = MUIV_Inst_Failed;
                            D(bug("[INSTALLER] BUG"));// %s%s (%d - %d - %d) %s\n",message->dir,  ead->ed_Name, dirlen, strlen(ead->ed_Name), newlen, dir));
                        }
                        break;
                    }
                }               
                ead = ead->ed_Next;
            } while((ead != NULL)&&(data->inst_success == MUIV_Inst_InProgress));
        }
    } while((loop)&&(data->inst_success == MUIV_Inst_InProgress));
    
    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(lock);

    return noOfFiles;
}

IPTR Install__MUIM_CopyFiles
(
    Class *CLASS, Object *self, struct MUIP_CopyFiles* message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    BPTR lock = Lock(message->srcDir, SHARED_LOCK);

    if(lock == 0)
    {
        D(bug("[INSTALLER] Failed to lock dir/file when copying files: %s (Error: %d)\n", message->srcDir, IoErr()));
        data->inst_success = MUIV_Inst_Failed;
        return 0;
    }
    
    UBYTE buffer[4096];
    struct ExAllData *ead = (struct ExAllData*)buffer;
    struct ExAllControl  *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    BOOL  loop;
    struct ExAllData *oldEad = ead;
    
    do
    {
        ead = oldEad;
        loop = ExAll(lock, ead, sizeof(buffer), ED_COMMENT, eac);

        if(!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
        {
            break;
        }
        
        if(eac->eac_Entries != 0)
        {
            do
            {
                if(ead->ed_Type == ST_FILE || ead->ed_Type == ST_USERDIR)
                {
                    ULONG srcLen = strlen(message->srcDir);
                    ULONG dstLen = strlen(message->dstDir);
                    ULONG newSrcLen = srcLen + strlen(ead->ed_Name) + 2;
                    ULONG newDstLen = dstLen + strlen(ead->ed_Name) + 2;
                    
                    TEXT srcFile[newSrcLen];
                    TEXT dstFile[newDstLen];
                    
                    CopyMem(message->srcDir, srcFile, srcLen + 1);
                    CopyMem(message->dstDir, dstFile, dstLen + 1);
                    if(AddPart(srcFile, ead->ed_Name, newSrcLen) && AddPart(dstFile, ead->ed_Name, newDstLen))
                    {
                        //D(bug("[INSTALLER] R: %s -> %s \n", srcFile, dstFile));
                        set(data->actioncurrent, MUIA_Text_Contents, srcFile);

                        DoMethod(data->installer,MUIM_Application_InputBuffered);

                        switch(ead->ed_Type)
                        {
                            case ST_FILE:
                                DoMethod(self, MUIM_CopyFile, srcFile, dstFile);
                                
                                message->currFile++;
                                break;

                            case ST_USERDIR:
                                message->currFile = DoMethod(self, MUIM_CopyFiles, srcFile, dstFile, message->noOfFiles, message->currFile);
                                break;
                        }
                        ULONG percent = message->currFile == 0 ? 0 : (message->currFile*100)/message->noOfFiles;
                        set(data->gauge2, MUIA_Gauge_Current, percent);
                        //D(bug("[INSTALLER] %2d\n", percent));
                    }
                    else
                    {
                        D(bug("[INSTALLER] BUG"));// %s%s (%d - %d - %d) %s\n",message->dir,  ead->ed_Name, dirlen, strlen(ead->ed_Name), newlen, dir));
                    }
                }               
                ead = ead->ed_Next;
            } while((ead != NULL)&&(data->inst_success == MUIV_Inst_InProgress));
        }
    } while((loop)&&(data->inst_success == MUIV_Inst_InProgress));
    
    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(lock);

    return message->currFile;
}

IPTR Install__MUIM_CopyFile
(
    Class *CLASS, Object *self, struct MUIP_CopyFile* message 
)
{
    struct Install_DATA *data    = INST_DATA(CLASS, self);
    static TEXT buffer[kBufSize];

    BPTR from, to;
    if((from = Open(message->srcFile, MODE_OLDFILE)))
    {
        if((to = Open(message->dstFile, MODE_NEWFILE)))
        {
            LONG s, err = 0;
            
            do
            {
                if ((s = Read(from, buffer, kBufSize)) == -1 || Write(to, buffer, s) == -1)
                {
                    goto fail;
                }
            } while ((s == kBufSize && !err)&&(data->inst_success == MUIV_Inst_InProgress));
            Close(to);
        }
        else
        {
            D(bug("[INSTALLER] Failed to open: %s (%d)\n", message->srcFile, IoErr()));
            data->inst_success = MUIV_Inst_Failed;
        }
        Close(from);
    }
    else
    {
        D(bug("[INSTALLER] Failed to open: %s (%d)\n", message->srcFile, IoErr()));
        data->inst_success = MUIV_Inst_Failed;
    }
    return 0;
 fail:
    D(bug("[INSTALLER] Failed to copy: %s (%d)\n", message->srcFile, IoErr()));
    data->inst_success = MUIV_Inst_Failed;
    if(from) Close(from);
    if(to) Close(to);
    return RETURN_FAIL;
}

IPTR Install__MUIM_Reboot
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    IPTR                    option = FALSE;

    get(data->options->opt_reboot,MUIA_Selected,&option);        // Make sure the user wants to reboot
    if ((option==TRUE)&&( data->inst_success ==  MUIV_Inst_InProgress))
    {
        D(bug("[INSTALLER] Cold rebooting...\n"));
        ColdReboot();
    }
    else
    {
        D(bug("[INSTALLER] Install Finished [no reboot]...\n"));
        if (data->inst_success == MUIV_Inst_InProgress) data->inst_success = MUIV_Inst_Completed;
        set(data->window,MUIA_Window_CloseRequest,TRUE);
    }

    return TRUE; /* Keep the compiler happy... */
}

BOOPSI_DISPATCHER(IPTR, Install_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return Install__OM_NEW(CLASS, self, (struct opSet *) message);

        case MUIM_DH0:
            return Install__MUIM_DH0(CLASS, self, message);

        case MUIM_NextStep:   
            return Install__MUIM_NextStep(CLASS, self, message);

        case MUIM_PrevStep:   
            return Install__MUIM_PrevStep(CLASS, self, message);
        //cancel control methods
        case MUIM_CancelInstall:   
            return Install__MUIM_CancelInstall(CLASS, self, message);

        case MUIM_ContinueInstall:   
            return Install__MUIM_ContinueInstall(CLASS, self, message);

        case MUIM_QuitInstall:   
            return Install__MUIM_QuitInstall(CLASS, self, message);

        case MUIM_Reboot:
            return Install__MUIM_Reboot(CLASS, self, message);

        //This should dissapear
        case MUIM_RefreshWindow:
            return Install__MUIM_RefreshWindow(CLASS, self, message);
/**/
        case MUIM_Install:
            return Install__MUIM_Install(CLASS, self, message);

        //These will be consumed by the io task..
        case MUIM_Partition:
            return Install__MUIM_Partition(CLASS, self, message);

        case MUIM_Format:
            return Install__MUIM_Format(CLASS, self, message);
            
        case MUIM_MakeDirs:
            return Install__MUIM_MakeDirs(CLASS, self, (struct MUIP_Dir*)message);

        case MUIM_CopyFiles:
            return Install__MUIM_CopyFiles(CLASS, self, (struct MUIP_CopyFiles*)message);
            
        case MUIM_CopyFile:
            return Install__MUIM_CopyFile(CLASS, self, (struct MUIP_CopyFile*)message);

        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    
    return 0;
}
BOOPSI_DISPATCHER_END

int main(int argc,char *argv[])
{
/**/
    Object* cwnd = NULL;            /* cancel window objects */
    Object* cancelmessage = NULL;
/**/
    Object* wnd = NULL;             /* installer window objects  - will get swallowed into the class eventually */
    Object* wndcontents = NULL;
    Object* page = NULL;

    Object* welcomeMsg = NULL;
    Object* doneMsg = NULL;

    Object* pagetitle = NULL;
    Object* pageheader = NULL;
    Object* currentaction = NULL;

    Object* gad_back    = SimpleButton("<< _Back..");
    Object* gad_proceed = SimpleButton(KMsgProceed);
    Object* gad_cancel  = SimpleButton("_Cancel");

    Object* gad_quit    = SimpleButton(KMsgProceed);
    Object* gad_nquit   = SimpleButton("_Cancel");

    Object* check_autopart = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,TRUE , End;
    Object* check_locale = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,FALSE , End;
    Object* check_core = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,TRUE , End;
    Object* check_dev = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,FALSE , End;
    Object* check_extras = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,TRUE , End;
    Object* check_bootloader = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,TRUE , End;

    Object* check_reboot = ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected,TRUE , End;

    Object* gauge1 = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);
    Object* gauge2 = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);
    Object* gauge3 = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);
/**/
    Object* label;

    struct Install_Options  *install_opts = NULL;

    install_opts = AllocMem( sizeof(struct Install_Options), MEMF_CLEAR | MEMF_PUBLIC );

    BPTR lock = NULL;

    lock = Lock(DEF_INSTALL_IMAGE, ACCESS_READ);
    UnLock(lock);

    Object *app = ApplicationObject,
        MUIA_Application_Title,       (IPTR) "AROS Installer",
        MUIA_Application_Version,     (IPTR) "$VER: InstallAROS 0.2 (22.03.04)",
        MUIA_Application_Copyright,   (IPTR) "Copyright © 2003, The AROS Development Team. All rights reserved.",
        MUIA_Application_Author,      (IPTR) "John \"Forgoil\" Gustafsson & Nic Andrews",
        MUIA_Application_Description, (IPTR) "Installs AROS onto your PC.",
        MUIA_Application_Base,        (IPTR) "INSTALLER",

        SubWindow, (IPTR) (wnd = WindowObject,
            MUIA_Window_Title, (IPTR) "AROS Installer",
            MUIA_Window_ID, MAKE_ID('f','o','r','g'),
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

                        Child, (IPTR) VGroup,
                            Child, (IPTR) (page = VGroup,
                                MUIA_Group_PageMode, TRUE,
                                ReadListFrame,
                                MUIA_Background, MUII_SHINE,

    /* each page represents an install time page .. you must have one for each enumerated install progress page */

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) (welcomeMsg = FreeCLabel("")),
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgInstallOptions),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            Child, (IPTR) LLabel("Auto Select & Format Partitions"),
                                            Child, (IPTR) check_autopart,
                                            Child, (IPTR) LLabel("Choose Language Options"),
                                            Child, (IPTR) check_locale,
                                            Child, (IPTR) LLabel("Install AROS Core System"),
                                            Child, (IPTR) check_core,
                                            Child, (IPTR) LLabel("Install Extra Software"),
                                            Child, (IPTR) check_extras,
                                            Child, (IPTR) LLabel("Install Development Software"),
                                            Child, (IPTR) check_dev,
                                            Child, (IPTR) LLabel("Copy and Install Bootloader "),
                                            Child, (IPTR) check_bootloader,
                                        End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgPartitioning),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, gauge1, End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) CLabel(KMsgPartitioningWipe),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, (IPTR) gauge3, End,
                                        Child, (IPTR) ScaleObject, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        Child, (IPTR) (pagetitle        = CLabel(" ")),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (pageheader       = FreeCLabel(KMsgInstall)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (label            = FreeLLabel("YOU SHOULD NOT SEE THIS")),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) (currentaction    = TextObject,MUIA_Text_Contents,(IPTR)" ",End),
                                        Child, (IPTR) VGroup, GaugeFrame,MUIA_Background, MUII_HSHINEBACK, Child, gauge2, End,
                                        Child, (IPTR) HVSpace,
                                    End,
                                End,

                                Child, (IPTR) VGroup,
                                    Child, (IPTR) VGroup,
                                        MUIA_Group_SameHeight, FALSE,
                                        Child, (IPTR) (doneMsg = FreeCLabel(KMsgDone)),
                                        Child, (IPTR) HVSpace,
                                        Child, (IPTR) ColGroup(2),
                                            MUIA_Weight,0,
                                            Child, (IPTR) LLabel("Reboot this computer now?"),
                                            Child, (IPTR) check_reboot,
                                        End,
                                    End,
                                End,
    /* */
                            End),
                        End,
                    End,
                    Child, (IPTR) HGroup,
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) gad_back,
                        Child, (IPTR) gad_proceed,
                        Child, (IPTR) gad_cancel,
                    End,

                End,
            End),
        End),
    End;

    if (!app)
    {
        exit(5);
    }
   
    install_opts->opt_partition = check_autopart;
    install_opts->opt_locale = check_locale;
    install_opts->opt_copycore = check_core;
    install_opts->opt_copyextra = check_extras;
    install_opts->opt_development = check_dev;
    install_opts->opt_bootloader = check_bootloader;
    install_opts->opt_reboot = check_reboot;

    struct MUI_CustomClass *mcc = MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct Install_DATA), Install_Dispatcher);
    Object *installer = NewObject(mcc->mcc_Class, NULL,

                MUIA_Page, (IPTR) page,
                MUIA_Gauge1, (IPTR) gauge1,
                MUIA_Gauge2, (IPTR) gauge2,
                MUIA_Install, (IPTR) label,
/**/

                MUIA_OBJ_Installer,(IPTR) app,

                MUIA_WelcomeMsg, (IPTR) welcomeMsg,
                MUIA_FinishedMsg, (IPTR) doneMsg,
/**/
                MUIA_List_Options, (IPTR) install_opts,
/**/
                MUIA_OBJ_WindowContent,(IPTR) wndcontents,
                MUIA_OBJ_Window,(IPTR) wnd,

                MUIA_OBJ_PageTitle, (IPTR) pagetitle,
                MUIA_OBJ_PageHeader, (IPTR) pageheader,
                MUIA_OBJ_CActionStrng, (IPTR) currentaction,
                MUIA_OBJ_Back,(IPTR) gad_back,
                MUIA_OBJ_Proceed,(IPTR) gad_proceed,
                MUIA_OBJ_Cancel,(IPTR) gad_cancel,
/**/
                MUIA_OBJ_CWindow,(IPTR) cwnd,
                MUIA_OBJ_CText, (IPTR) cancelmessage,
                MUIA_OBJ_COK, (IPTR) gad_quit,
                MUIA_OBJ_CCancel, (IPTR) gad_nquit,

                TAG_DONE);

    IPTR        install_content1[20],install_content2[20],install_content3[20],install_content4[20],install_content5[20],install_content6[20];
    IPTR        install_pages[20];

#define OPTION_PREPDRIVES       1
#define OPTION_FORMAT           2
#define OPTION_LANGUAGE         3
#define OPTION_CORE             4
#define OPTION_EXTRAS           5
#define OPTION_BOOTLOADER       6

#define INSTV_TITLE             101001
#define INSTV_LOGO              101002
#define INSTV_PAGE              101003

#define INSTV_TEXT              101004
#define INSTV_SPACE             101005
#define INSTV_BOOL              101006
#define INSTV_RETURN            101007

#define INSTV_CURR              101100

/* page descriptions */
    /* welcome page */
    install_content1[0] = INSTV_TEXT;
    install_content1[1] = (IPTR) KMsgWelcome;

    install_content1[2] = TAG_DONE;

    /* Options Page */
    install_content2[0] = INSTV_TEXT;
    install_content2[1] = (IPTR) KMsgInstallOptions;

    install_content2[0] = INSTV_SPACE;
    install_content2[1] = TAG_IGNORE;

    install_content2[0] = INSTV_BOOL;
    install_content2[1] = (IPTR) check_autopart;

    install_content2[2] = INSTV_BOOL;
    install_content2[3] = (IPTR) check_locale;

    install_content2[4] = INSTV_BOOL;
    install_content2[5] = (IPTR) check_core;

    install_content2[6] = INSTV_BOOL;
    install_content2[7] = (IPTR) check_extras;

    install_content2[8] = INSTV_BOOL;
    install_content2[9] = (IPTR) check_bootloader;

    install_content2[10] = TAG_DONE;

    /* Prepare Drives Page */
    install_content3[0] = INSTV_TEXT;
    install_content3[1] = (IPTR) KMsgPartitioning;

    install_content3[2] = INSTV_RETURN;
    install_content3[3] = OPTION_PREPDRIVES;

    install_content3[4] = TAG_DONE;

    /* Wipe Drives */
    install_content4[0] = INSTV_TEXT;
    install_content4[1] = (IPTR) KMsgPartitioningWipe;

    install_content4[2] = INSTV_RETURN;
    install_content4[3] = OPTION_FORMAT;

    install_content4[4] = TAG_DONE;

    /* */
    install_content5[4] = TAG_DONE;

    /* ALL DONE !! */
    install_content6[0] = INSTV_TEXT;
    install_content6[1] = (IPTR) KMsgDone;

    install_content6[2] = TAG_DONE;
/* installer pages */

    install_pages[0] = INSTV_CURR;
    install_pages[1] = (IPTR) install_content1;

    install_pages[0] = INSTV_TITLE;
    install_pages[1] = (IPTR)"AROS Installer";

    install_pages[0] = INSTV_LOGO;
    install_pages[1] = (IPTR)"3:"DEF_INSTALL_IMAGE;

    install_pages[0] = INSTV_PAGE;
    install_pages[1] = (IPTR) install_content1;

    install_pages[2] = INSTV_PAGE;
    install_pages[3] = (IPTR) install_content2;

    install_pages[4] = INSTV_PAGE;
    install_pages[5] = (IPTR) install_content3;

    install_pages[6] = INSTV_PAGE;
    install_pages[7] = (IPTR) install_content4;

    install_pages[8] = INSTV_PAGE;
    install_pages[9] = (IPTR) install_content5;

    install_pages[10] = INSTV_PAGE;
    install_pages[11] = (IPTR) install_content6;

    install_pages[12] = TAG_DONE;
    
/* */
    DoMethod(wnd,MUIM_Notify,MUIA_Window_CloseRequest,TRUE, app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

    set(wnd,MUIA_Window_Open,TRUE);
    {
        ULONG sigs = 0;

        while (DoMethod(app,MUIM_Application_NewInput,&sigs) != MUIV_Application_ReturnID_Quit)
        {
            if (sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if (sigs & SIGBREAKF_CTRL_C) break;
            }
        }
    }

    D(bug("[INST-APP] Closing Window\n"));

    set(wnd,MUIA_Window_Open,FALSE);

    D(bug("[INST-APP] Disposing of Installer Object\n"));

    DisposeObject(installer);

    D(bug("[INST-APP] Removing Custom Class\n"));

    MUI_DeleteCustomClass(mcc);

    D(bug("[INST-APP] Removing App Object\n"));

    MUI_DisposeObject(app);

    return 0;
}
