/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
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

IPTR Install__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);

    struct Install_DATA* data = INST_DATA(CLASS, self);

    data->welcomeMsg = (APTR) GetTagData (MUIA_WelcomeMsg,      (ULONG) NULL, message->ops_AttrList);
    data->welcome    = (APTR) GetTagData (MUIA_WelcomeButton,   (ULONG) NULL, message->ops_AttrList);
    data->partition  = (APTR) GetTagData (MUIA_PartitionButton, (ULONG) NULL, message->ops_AttrList);
    data->page       = (APTR) GetTagData (MUIA_Page,            (ULONG) NULL, message->ops_AttrList);
    data->gauge1     = (APTR) GetTagData (MUIA_Gauge1,          (ULONG) NULL, message->ops_AttrList);
    data->gauge2     = (APTR) GetTagData (MUIA_Gauge2,          (ULONG) NULL, message->ops_AttrList);
    data->label      = (APTR) GetTagData (MUIA_Install,         (ULONG) NULL, message->ops_AttrList);

    BOOL dh0 = (BOOL)DoMethod(self, MUIM_DH0);
    if(dh0)
    {
        set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome2);
        set(data->welcome, MUIA_Text_Contents, "Install");
        data->nextStage = EInstallStage;
    }
    else
    {
        set(data->welcomeMsg, MUIA_Text_Contents, KMsgWelcome1);
        set(data->welcome, MUIA_Text_Contents, "Partition");
        data->nextStage = EPartitioningStage;
    }

    return (IPTR) self;
}

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

IPTR Install__MUIM_Continue
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);
    set(data->page,MUIA_Group_ActivePage, data->nextStage);

    switch(data->nextStage)
    {
        case EPartitioningStage:
            if(DoMethod(self, MUIM_Partition) != RETURN_OK)
            {
                exit(RETURN_FAIL);
            }
            set(data->gauge1, MUIA_Gauge_Current, 100);
            set(data->partition, MUIA_Disabled, FALSE);
            break;
        case EInstallStage:
            DoMethod(self, MUIM_Install);
            set(data->page,MUIA_Group_ActivePage, EDoneStage);
            break;
        default:
            break;
    }

    return NULL;
}

IPTR Install__MUIM_Partition
(
    Class *CLASS, Object *self, Msg message 
)
{
    return SystemTagList("C:Partition FORCE QUIET", NULL);
}

IPTR Install__MUIM_Install
(     
 Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);

    // Formatting
    D(bug("Formatting...\n"));
    DoMethod(self, MUIM_Format);

    // Making dirs
    D(bug("Making dirs...\n"));
    set(data->label, MUIA_Text_Contents, "Making dirs...");
    ULONG noOfFiles = DoMethod(self, MUIM_MakeDirs, kSrcDir, kDstDir);

    // Copying files
    D(bug("Copying files...\n"));
    set(data->label, MUIA_Text_Contents, "Copying files...");
    noOfFiles = DoMethod(self, MUIM_CopyFiles, kSrcDir, kDstDir, noOfFiles, 0);

    // Installing GRUB
    D(bug("Installing Grub...\n"));
    set(data->label, MUIA_Text_Contents, "Installing Grub...");

    set(data->gauge2, MUIA_Gauge_Current, 0);
    Execute("C:install-i386-pc DEVICE ide.device UNIT 0 KERNEL DH0:boot/aros-pc-i386.gz GRUB DH0:boot/grub", NULL, NULL);
    set(data->gauge2, MUIA_Gauge_Current, 25);
    Execute("C:Delete DH0:boot/grub/menu.lst", NULL, NULL);
    set(data->gauge2, MUIA_Gauge_Current, 50);
    Execute("C:Copy DH0:boot/grub/menu.lst.DH0 DH0:boot/grub/menu.lst", NULL, NULL);
    set(data->gauge2, MUIA_Gauge_Current, 75);
    Execute("C:Delete DH0:boot/grub/menu.lst.DH0", NULL, NULL);
    set(data->gauge2, MUIA_Gauge_Current, 100);

    // Rename can't handle absolut paths right now, would be nicer to use though.
    // Execute("C:Rename DH0:boot/grub/menu.lst.DH0 DH0:boot/grub/menu.lst", NULL, NULL);

    return NULL;
}

IPTR Install__MUIM_Format
(     
 Class *CLASS, Object *self, Msg message 
)
{
    struct Install_DATA* data = INST_DATA(CLASS, self);

    set(data->label, MUIA_Text_Contents, "Formatting...");
    set(data->gauge2, MUIA_Gauge_Current, 0);

    BOOL fail = FALSE;

        if(Inhibit(kDstDir, DOSTRUE))
        {
            if(!Format(kDstDir, kDstName, ID_FFS_DISK))
            {
            fail = TRUE;
            }
            Inhibit(kDstDir, DOSFALSE);
        }
        else
        {
        fail = TRUE;
        }

    if(fail)
    {
        return FALSE;
    }
    else
    {
        set(data->gauge2, MUIA_Gauge_Current, 100);
        return TRUE;
    }
}

IPTR Install__MUIM_MakeDirs
(
    Class *CLASS, Object *self, struct MUIP_Dir* message 
)
{
    UBYTE buffer[4096];
    struct ExAllData *ead = (struct ExAllData*)buffer;
    struct ExAllData *oldEad = ead;
    struct ExAllControl  *eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;

    BPTR lock = Lock(message->srcDir, SHARED_LOCK);

    if(lock == 0)
    {
        D(bug("Failed to lock dir when making the dirs: %s (Error: %d)\n", message->srcDir, IoErr()));
        return 0;
    }

    LONG noOfFiles=0;
    BOOL  loop;

    D(bug("Locked and loaded\n"));
    
    do
    {
        ead = oldEad;
        loop = ExAll(lock, ead, sizeof(buffer), ED_COMMENT, eac);

        if(!loop && IoErr() != ERROR_NO_MORE_ENTRIES)
        {
            break;
        }
        
        if(eac->eac_Entries != NULL)
        {
            do
            {
                D(bug("Doin the entries: %d\n", ead->ed_Type));

                switch(ead->ed_Type)
                {
                    default:
                        D(bug("Type: %d\tName: %s\n", ead->ed_Type, ead->ed_Name));
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
                            D(bug("R: %s -> %s \n", srcDir, dstDir));
                            BPTR dirLock = CreateDir(dstDir);
                            if(dirLock != NULL) UnLock(dirLock);
                            noOfFiles += DoMethod(self, MUIM_MakeDirs, srcDir, dstDir);
                        }
                        else
                        {
                            D(bug("BUG"));// %s%s (%d - %d - %d) %s\n",message->dir,  ead->ed_Name, dirlen, strlen(ead->ed_Name), newlen, dir));
                        }
                        break;
                    }
                }               
                ead = ead->ed_Next;
            } while(ead != NULL);
        }
    } while(loop);
    
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
        D(bug("Failed to lock dir/file when copying files: %s (Error: %d)\n", message->srcDir, IoErr()));
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
        
        if(eac->eac_Entries != NULL)
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
                        //D(bug("R: %s -> %s \n", srcFile, dstFile));
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
                        //D(bug("%2d\n", percent));
                    }
                    else
                    {
                        D(bug("BUG"));// %s%s (%d - %d - %d) %s\n",message->dir,  ead->ed_Name, dirlen, strlen(ead->ed_Name), newlen, dir));
                    }
                }               
                ead = ead->ed_Next;
            } while(ead != NULL);
        }
    } while(loop);
    
    FreeDosObject(DOS_EXALLCONTROL, eac);
    UnLock(lock);

    return message->currFile;
}

IPTR Install__MUIM_CopyFile
(
    Class *CLASS, Object *self, struct MUIP_CopyFile* message 
)
{
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
            } while (s == kBufSize && !err);
            Close(to);
        }
        else
        {
            D(bug("Failed to open: %s (%d)\n", message->srcFile, IoErr()));
        }
        Close(from);
    }
    else
    {
        D(bug("Failed to open: %s (%d)\n", message->srcFile, IoErr()));
    }
    return 0;
 fail:
    D(bug("Failed to copy: %s (%d)\n", message->srcFile, IoErr()));
    if(from) Close(from);
    if(to) Close(to);
    return RETURN_FAIL;
}

IPTR Install__MUIM_Reboot
(
    Class *CLASS, Object *self, Msg message 
)
{
    D(bug("Cold rebooting...\n"));
    ColdReboot();
    
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

        case MUIM_Continue:   
            return Install__MUIM_Continue(CLASS, self, message);

        case MUIM_Partition:
            return Install__MUIM_Partition(CLASS, self, message);

        case MUIM_Format:
            return Install__MUIM_Format(CLASS, self, message);

        case MUIM_Install:
            return Install__MUIM_Install(CLASS, self, message);
            
        case MUIM_MakeDirs:
            return Install__MUIM_MakeDirs(CLASS, self, (struct MUIP_Dir*)message);

        case MUIM_CopyFiles:
            return Install__MUIM_CopyFiles(CLASS, self, (struct MUIP_CopyFiles*)message);
            
        case MUIM_CopyFile:
            return Install__MUIM_CopyFile(CLASS, self, (struct MUIP_CopyFile*)message);
            
        case MUIM_Reboot:
            return Install__MUIM_Reboot(CLASS, self, message);

        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    return NULL;
}

int main(int argc,char *argv[])
{
    Object* wnd;
    Object* page;

    Object* welcomeMsg;
    Object* welcome = SimpleButton("Should not see this");
    Object* gauge1 = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);

    Object* gauge2 = (GaugeObject, MUIA_Gauge_InfoText, "%ld %%", MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, End);

    Object* label;

    Object* reboot1 = SimpleButton("Reboot");
    Object* reboot2 = SimpleButton("Reboot");

    set(reboot1, MUIA_Disabled, TRUE);

    Object* app = ApplicationObject,
        MUIA_Application_Title      , "AROS Installer",
        MUIA_Application_Version    , "$VER: InstallAROS 0.1 (05.07.03)",
        MUIA_Application_Copyright  , "Copyright © 2003, The AROS Development Team. All rights reserved.",
        MUIA_Application_Author     , "John \"Forgoil\" Gustafsson",
        MUIA_Application_Description, "Installs AROS onto your dh0: drive.",
        MUIA_Application_Base       , "INSTALLER",

        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, "AROS Installer",
            MUIA_Window_ID, MAKE_ID('f','o','r','g'),
        WindowContents, VGroup,

                Child, page = VGroup, MUIA_Group_PageMode, TRUE,
        Child, VGroup, Child, welcomeMsg = CLabel(""), Child, HVSpace, Child, welcome, End,

        Child, VGroup,
        Child, CLabel(KMsgPartition),
        Child, HVSpace,
        Child, VGroup, GaugeFrame, Child, gauge1, End,
        Child, ScaleObject, End,
        Child, HVSpace,
        Child, reboot1,
        End,

        Child, VGroup,
        Child, CLabel(KMsgInstall),
        Child, label = LLabel("YOU SHOULD NOT SEE THIS"),
        Child, VGroup, GaugeFrame, Child, gauge2, End,
        Child, ScaleObject, End,
        End,

        Child, VGroup, Child, CLabel(KMsgDone), Child, HVSpace, Child, reboot2, End,

                End, End, End, End;

    if (!app)
    {
        exit(5);
    }

    struct MUI_CustomClass *mcc = MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct Install_DATA), Install_Dispatcher);
    Object *installer = NewObject(mcc->mcc_Class, NULL,
                 MUIA_WelcomeMsg, welcomeMsg,
                 MUIA_WelcomeButton, welcome,
                 MUIA_PartitionButton, reboot1,
                 MUIA_Page, page,
                 MUIA_Gauge1, gauge1,
                 MUIA_Gauge2, gauge2,
                 MUIA_Install, label, TAG_DONE);

    DoMethod(wnd,MUIM_Notify,MUIA_Window_CloseRequest,TRUE, app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
    DoMethod(welcome, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) installer, 1, MUIM_Continue);
    DoMethod(reboot1, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) installer, 1, MUIM_Reboot);
    DoMethod(reboot2, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) installer, 1, MUIM_Reboot);

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

    set(wnd,MUIA_Window_Open,FALSE);

    DisposeObject(installer);

    MUI_DeleteCustomClass(mcc);
    MUI_DisposeObject(app);

    return 0;
}
