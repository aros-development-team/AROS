/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#include   <aros/debug.h>

#include   <proto/expansion.h>
#include   <proto/exec.h>
#include   <proto/partition.h>
#include   <proto/dos.h>
#include   <proto/intuition.h>
#include   <proto/muimaster.h>
#include   <proto/locale.h>
#include   <proto/utility.h>

#include   <libraries/configvars.h>
#include   <libraries/expansionbase.h>
#include   <libraries/partition.h>
#include   <libraries/mui.h>

#include   <devices/trackdisk.h>

#include   <dos/dos.h>
#include   <dos/filehandler.h>

#include <utility/tagitem.h>

#include   <exec/memory.h>
#include   <exec/execbase.h>
#include   <exec/lists.h>
#include   <exec/nodes.h>
#include   <exec/types.h>
#include   <exec/ports.h>

#include   <zune/systemprefswindow.h>
#include   <zune/prefseditor.h>
#include   <zune/aboutwindow.h>

#include   <stdlib.h>
#include   <stdio.h>
#include   <strings.h>
#include   <string.h>

#include    <clib/alib_protos.h>

#include "QP_Intern.h"

#define _QP_CCDISK_C

#include "QP_ccDisk.h"
#include "QP_ccPartition.h"
#include "QP_ccPartitionContainer.h"
#include "QP_globals.h"
#include "QP_locale.h"

#define SETUP_INST_DATA struct QPDisk_DATA *data = INST_DATA(CLASS, self)
extern struct   MUI_CustomClass      *mcc_qpfree;

/** Internal Class Funcs **/

#define QPDISK_SCSIBUFF_SIZE            512
#define QPDISK_DISKNAMEBUFF_MAXSIZE     (QPDISK_SCSIBUFF_SIZE/4)
#define QPDISK_DISKCAPACITYBUFF_MAXSIZE 100

extern struct   MUI_CustomClass      *mcc_qptxt;

struct localeStreamHookData
{
    char               *str_sizeval;
    struct QPDisk_DATA *str_intdata;
};

void QPDisk__Func_w2strcpy(UWORD *name, UWORD *wstr, ULONG len)
{
    while (len)
    {
        *(name++) = AROS_BE2WORD(*wstr);
        len -= 2;
        wstr++;
    }
    name -= 2;
    while ((*name==0) || (*name==' '))
        *name-- = 0;
}

BOOL QPDisk__Func_IdentifyDrive(struct IOStdReq *ioreq, STRPTR name)
{
    UWORD          data[QPDISK_SCSIBUFF_SIZE];
    struct SCSICmd scsicmd;
    UBYTE          cmd;

    D(bug("[QuickPart:Disk] IdentifyDrive()\n"));

    /* Try using ATA Identify first .. */
    cmd = ATA_C_IDENTIFY;
    scsicmd.scsi_Data = data;
    scsicmd.scsi_Length = QPDISK_SCSIBUFF_SIZE;
    scsicmd.scsi_Command = &cmd;
    scsicmd.scsi_CmdLength = 1;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = &scsicmd;
    ioreq->io_Length = sizeof(struct SCSICmd);
    if (!DoIO((struct IORequest *)ioreq))
    {
        D(bug("[QuickPart:Disk] IdentifyDrive: Using ATA Identify Model\n"));
        QPDisk__Func_w2strcpy((UWORD *)name, &data[27], 40);
        return TRUE;
    }

    /* Failing that try using SCSI Inquiry .. */
    cmd = SCSI_INQUIRY;
    scsicmd.scsi_Data = data;
    scsicmd.scsi_Length = QPDISK_SCSIBUFF_SIZE;
    scsicmd.scsi_Command = &cmd;
    scsicmd.scsi_CmdLength = 1;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = &scsicmd;
    ioreq->io_Length = sizeof(struct SCSICmd);
    if (!DoIO((struct IORequest *)ioreq))
    {
        D(bug("[QuickPart:Disk] IdentifyDrive: Using SCSI Inquiry Product\n"));
        QPDisk__Func_w2strcpy((UWORD *)name, &data[16], 16);
        return TRUE;
    }
    return FALSE;
}

/* LOCALE: Convert a number to the localised version .. */

AROS_UFH3(void, QPDisk__HookFunc_LocaliseSize,
    AROS_UFHA(struct Hook *, locSizHook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, c, A1))
{
    AROS_USERFUNC_INIT

    struct QPDisk_DATA *data = NULL;
    char               *out_buff = NULL;

    if (locSizHook->h_Data)
    {
        D(bug("[QuickPart:Disk] [localhook] hook->h_Data @ %p\n", locSizHook->h_Data));
        data = ((struct localeStreamHookData *)locSizHook->h_Data)->str_intdata;
        out_buff = ((struct localeStreamHookData *)locSizHook->h_Data)->str_sizeval;
    }
    else
        return;

    if ((out_buff == NULL)||(data == NULL))
        return;

    if (c != '\0')
    {
        out_buff[data->qpd_Disk_hook_LocaliseNumber_ConvCnt] = c;
        data->qpd_Disk_hook_LocaliseNumber_ConvCnt ++;
    }
    else
    {
        data->qpd_Disk_hook_LocaliseNumber_ConvCnt = 0;
        D(bug("[QuickPart:Disk] [localhook] Localised size : %s\n", out_buff));
    }

    AROS_USERFUNC_EXIT
}

/** CLASS Methods **/
static IPTR QPDisk__MUIM_QPart_Disk_ScanForParts
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    UQUAD   adjustedsize = 0;
    int sizecounter = 0;

    struct   Locale             *locale = NULL;

    D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts()\n"));

    if (OpenDevice(data->qpd_Disk_Handler, data->qpd_Disk_Unit, (struct IORequest *)data->qpd_Disk_IOReq, 0) == 0)
    {
        D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts: Device %s unit %d Opened\n", data->qpd_Disk_Handler, data->qpd_Disk_Unit));

        if (data->qpd_Disk_str_DriveName)
        {
            char *origName = data->qpd_Disk_str_DriveName;
            data->qpd_Disk_str_DriveName = AllocVec(strlen(origName) + 1, MEMF_CLEAR|MEMF_PUBLIC);
            strcpy(data->qpd_Disk_str_DriveName, origName);
        }
        else
        {
            data->qpd_Disk_str_DriveName = AllocVec(QPDISK_DISKNAMEBUFF_MAXSIZE + 2, MEMF_CLEAR|MEMF_PUBLIC);

            if (!QPDisk__Func_IdentifyDrive((struct IOStdReq *)data->qpd_Disk_IOReq, (STRPTR)data->qpd_Disk_str_DriveName))
            {
                sprintf(data->qpd_Disk_str_DriveName, "HardDisk Unit %d", data->qpd_Disk_Unit);
            }
        }
        CloseDevice((struct IORequest *)data->qpd_Disk_IOReq);

        data->qpd_Disk_PH = OpenRootPartition(data->qpd_Disk_Handler, data->qpd_Disk_Unit);

        if (data->qpd_Disk_PH)
        {
            GetPartitionAttrsTags
            (
                data->qpd_Disk_PH,
                PT_GEOMETRY, (IPTR) &data->qpd_Geom,
                TAG_DONE
            );

            D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts: Partition Handle @ %p\n", data->qpd_Disk_PH));

            data->qpd_Disk_val_DriveCapacity = ((UQUAD)data->qpd_Geom.dg_SectorSize * data->qpd_Geom.dg_TotalSectors);
            D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  %d sectors * %d bytes per sector\n", data->qpd_Geom.dg_TotalSectors, data->qpd_Geom.dg_SectorSize));
            D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  = %llu bytes\n", data->qpd_Disk_val_DriveCapacity));

            adjustedsize = data->qpd_Disk_val_DriveCapacity;

            while (( adjustedsize > 1024 ) && (QP__SizeName[sizecounter] != NULL))
            {
                sizecounter +=1;
                adjustedsize /= 1024;
            }

            /* localise the byte size for display */
            if ((data->qpd_Disk_val_DriveCapacity <= 0xFFFFFFFF) && ((locale = OpenLocale(NULL))))
            {
                ULONG sizeval = (ULONG)data->qpd_Disk_val_DriveCapacity;
                FormatString(locale, "%lU", (RAWARG)&sizeval, data->qpd_Disk_hook_LocaliseNumber);
                CloseLocale(locale);
            }
            else
                sprintf (((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_sizeval, "%llu", data->qpd_Disk_val_DriveCapacity);

            D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Finished localising size\n"));

            if (!(data->qpd_Disk_str_DriveNameVerbose))
                data->qpd_Disk_str_DriveNameVerbose = AllocVec(QPDISK_DISKNAMEBUFF_MAXSIZE, MEMF_CLEAR|MEMF_PUBLIC);

            if (data->qpd_Disk_str_DriveNameVerbose)
            {
                sprintf(data->qpd_Disk_str_DriveNameVerbose, ":" MUIX_B " %s " MUIX_N "[%s]" , data->qpd_Disk_str_DriveName, _(WORD_Unknown));
                set(data->qpd_Object_txt_DriveName, MUIA_Text_Contents, data->qpd_Disk_str_DriveNameVerbose);
            }

            if (!(data->qpd_Disk_str_DriveCapacity))
                data->qpd_Disk_str_DriveCapacity = AllocVec(QPDISK_DISKCAPACITYBUFF_MAXSIZE, MEMF_CLEAR|MEMF_PUBLIC);

            if ((data->qpd_Disk_str_DriveCapacity) && (data->qpd_Disk_hook_LocaliseNumber->h_Data))
            {
                if ((((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_sizeval))
                {
                    sprintf(data->qpd_Disk_str_DriveCapacity, ": %u%s ( %s %s )", adjustedsize, QP__SizeName[sizecounter], ((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_sizeval, _(WORD_Bytes));
                    set(data->qpd_Object_txt_DriveCapacity, MUIA_Text_Contents, data->qpd_Disk_str_DriveCapacity);
                }
            }

            /* Look For  Disk "CONTENTS" */
            if (OpenPartitionTable(data->qpd_Disk_PH) == 0)
            {
                D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Disk has a partition Table @ %p\n", data->qpd_Disk_PH));
                Object *tmpqpd_Object_HardDisk_Contents = NULL;
                //ClosePartitionTable(data->qpd_Disk_PH);

                if ((tmpqpd_Object_HardDisk_Contents = NewObject(mcc_qppartitioncontainer->mcc_Class, NULL,
                    MUIA_QPart_Disk_Object, (IPTR)self,
                    MUIA_QPart_Disk_Geometry, (IPTR)&data->qpd_Geom,
                    MUIA_QPart_ccPartition_Handle, (IPTR)data->qpd_Disk_PH,
                    TAG_DONE)) != NULL)
                {
                    D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Attempting to insert Disk Container Object...\n"));
                    if (DoMethod(data->qpd_Object_HardDisk_Rectangle, MUIM_Group_InitChange))
                    {
                        //D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Inserting Disk Container Object @ %p\n", tmpqpd_Object_HardDisk_Contents));
                        DoMethod(data->qpd_Object_HardDisk_Rectangle, OM_REMMEMBER, data->qpd_Object_HardDisk_Contents);

                        DoMethod(data->qpd_Object_HardDisk_Rectangle, OM_ADDMEMBER, tmpqpd_Object_HardDisk_Contents);

                        DoMethod(data->qpd_Object_HardDisk_Rectangle, MUIM_Group_ExitChange);
                        data->qpd_Object_HardDisk_Contents = tmpqpd_Object_HardDisk_Contents;
                    }

                    if (data->qpd_Object_HardDisk_Contents == tmpqpd_Object_HardDisk_Contents)
                    {
                        const char *Partitioning_style = NULL;
                        IPTR tmpqpd_Container_Type = 0;

                        D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Disk Container Object successfully created\n"));

                        get(data->qpd_Object_HardDisk_Contents, MUIA_QPart_ccPartitionContainer_Type, &tmpqpd_Container_Type);

                        switch(tmpqpd_Container_Type)
                        {
                        case PHPTT_RDB:
                            Partitioning_style = "RDB";
                            break;
                        case PHPTT_MBR:
                            Partitioning_style = "MBR";
                            break;
                        case PHPTT_GPT:
                            Partitioning_style = "GPT";
                            break;
                        default:
                            Partitioning_style = _(WORD_Unknown);
                            break;
                        }

                        if (data->qpd_Disk_str_DriveNameVerbose)
                        {
                            D(bug("[QuickPart:Disk] MUIM_QPart_Disk_ScanForParts:  Setting Disk Object Name And type\n"));
                            sprintf(data->qpd_Disk_str_DriveNameVerbose, ":" MUIX_B " %s " MUIX_N "[%s]" , data->qpd_Disk_str_DriveName, Partitioning_style);
                            set(data->qpd_Object_txt_DriveName, MUIA_Text_Contents, data->qpd_Disk_str_DriveNameVerbose);
                        }

                        DoMethod(data->qpd_Object_HardDisk_Contents, MUIM_QPart_ccPartitionContainer_UpdateParts);
                    }
                }
            }
        }
    }
    return TRUE;
}

/** Standard Class Methods **/
static IPTR QPDisk__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct   TagItem    *tags=NULL,
                        *tag=NULL;

    /* Tmp Object Pointers Used to create our "Disk" */
    Object            *tmpqpd_Object_HardDiskArea = NULL;
    Object            *tmpqpd_Object_txt_DriveName = NULL;
    Object            *tmpqpd_Object_txt_DriveCapacity = NULL;
    Object            *tmpqpd_Object_txt_DriveUnit = NULL;
    Object            *tmpqpd_Object_VOID_spacer = NULL;
    /*** The Drive Image (rectangle) **/
    Object            *tmpqpd_Object_HardDisk_LayoutGrp = NULL;
    Object            *tmpqpd_Object_HardDisk_Rectangle = NULL;
    Object            *tmpqpd_Object_HardDisk_Contents = NULL;
    Object            *tmpqpd_Object_VOID_weight = NULL;

    /* Tmp strings & values */
    IPTR              tmpqpd_Disk_BGColor = (IPTR)DEF_DRIVEPANE_BACK;
    char              *tmpqpd_Disk_Handler = NULL, *diskName = NULL;
    LONG              tmpqpd_Disk_Unit = -1, diskPad = 20;

    struct IOStdReq   *tmpqpd_Disk_IOReq = NULL;
    struct MsgPort    *tmpqpd_Disk_MP = NULL;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            tmpqpd_Disk_BGColor = (IPTR)tag->ti_Data;
            D(bug("[QuickPart:Disk.I] BGColor set\n"));
            break;

        case MUIA_QPart_Disk_Spacing:
            diskPad = (LONG)tag->ti_Data;
            D(bug("[QuickPart:Disk.I] Padding: %d\n", diskPad));
            break;

        case MUIA_QPart_Disk_Name:
            diskName = (char *)tag->ti_Data;
            D(bug("[QuickPart:Disk.I] Name: '%s'\n", diskName));
            break;

        case MUIA_QPart_Disk_Handler:
            tmpqpd_Disk_Handler = (char *)tag->ti_Data;
            D(bug("[QuickPart:Disk.I] Handler: '%s'\n", tmpqpd_Disk_Handler));
            break;

        case MUIA_QPart_Disk_Unit:
            tmpqpd_Disk_Unit = (LONG)tag->ti_Data;
            D(bug("[QuickPart:Disk.I] Unit: %d\n", tmpqpd_Disk_Unit));
            break;

        default:
            continue; /* Don't supress non-processed tags */
        }
        tag->ti_Tag = TAG_IGNORE;
    }

    if ((tmpqpd_Disk_Handler==NULL)||(tmpqpd_Disk_Unit==-1))
    {
        return (IPTR)NULL;
    }

    tmpqpd_Disk_MP = CreateMsgPort();
    tmpqpd_Disk_IOReq = (struct IOStdReq *)CreateIORequest(tmpqpd_Disk_MP, sizeof(struct IOStdReq));

    if (OpenDevice(tmpqpd_Disk_Handler, tmpqpd_Disk_Unit, (struct IORequest *)tmpqpd_Disk_IOReq, 0) == 0)
    {
        D(bug("[QuickPart:Disk.I] Creating DISK Object\n"));
        self = (Object *) DoSuperNewTags
        (
            CLASS, self, NULL,

            MUIA_Background, (IPTR)tmpqpd_Disk_BGColor,

            Child, (IPTR)(tmpqpd_Object_HardDiskArea = VGroup,
                Child, (IPTR)VGroup,
                    //MUIA_InputMode, MUIV_InputMode_RelVerify,
                    Child, (IPTR)HGroup,
                        Child, (IPTR)ImageObject,
                            MUIA_Frame,  MUIV_Frame_None,
                            MUIA_Image_Spec, (IPTR)"3:"DEF_HDISK_IMAGE,
                        End,
                        Child, (IPTR)VGroup,
                            Child, (IPTR)ColGroup(2),
                                Child, (IPTR)LLabel( _(WORD_Drive)),
                                Child, (IPTR)(tmpqpd_Object_txt_DriveName = NewObject(mcc_qptxt->mcc_Class, NULL,
                                    (diskName) ? MUIA_Text_Contents : TAG_IGNORE,
                                        diskName,
                                TAG_DONE)),
                                Child, (IPTR)LLabel( _(WORD_Capacity)),
                                Child, (IPTR)(tmpqpd_Object_txt_DriveCapacity = NewObject(mcc_qptxt->mcc_Class, NULL,
                                TAG_DONE)),
                            End,
                            Child, (IPTR)HVSpace,
                            Child, (IPTR)(tmpqpd_Object_txt_DriveUnit = NewObject(mcc_qptxt->mcc_Class, NULL,
                            TAG_DONE)),
                        End,
                        Child, (IPTR)(tmpqpd_Object_VOID_spacer = HVSpace),
                    End,
                    Child, (IPTR)(tmpqpd_Object_HardDisk_LayoutGrp = HGroup,
                        //MUIA_InputMode, MUIV_InputMode_None,
                        MUIA_FixHeight, 60,
                        MUIA_Group_HorizSpacing, 0,
                        Child, (IPTR)(tmpqpd_Object_HardDisk_Rectangle = HGroup,
                            MUIA_InnerLeft, 0,
                            MUIA_InnerTop, 0,
                            MUIA_InnerRight, 0,
                            MUIA_InnerBottom, 0,
                            MUIA_Group_SameWidth, FALSE,
                            MUIA_Frame,  MUIV_Frame_ImageButton,
                            Child, (IPTR)(tmpqpd_Object_HardDisk_Contents = NewObject(mcc_qpfree->mcc_Class, NULL,
                                MUIA_Background, (IPTR)DEF_DRIVE_EMPTY,
                            TAG_DONE)),
                        End),
                        Child, (IPTR)(tmpqpd_Object_VOID_weight = HVSpace),
                    End),
                    Child, (IPTR)HGroup,
                        MUIA_FixHeight, 5,
                        Child, HVSpace,
                    End,
                End,
                Child, (IPTR)HGroup,
                    MUIA_FixHeight, diskPad,
                    Child, HVSpace,
                End,
            End),

            TAG_MORE, (IPTR) message->ops_AttrList   
        );
    }
    else self = NULL;

    if (self)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        data->qpd_Disk_Handler = AllocVec(strlen(tmpqpd_Disk_Handler) + 1, MEMF_CLEAR|MEMF_PUBLIC);
        CopyMem(tmpqpd_Disk_Handler, data->qpd_Disk_Handler, strlen(tmpqpd_Disk_Handler));

        data->qpd_Disk_Unit = tmpqpd_Disk_Unit;

        D(bug("[QuickPart:Disk.I] Self @ %p for '%s:%d'\n", self, data->qpd_Disk_Handler, data->qpd_Disk_Unit));

        data->qpd_Disk_MP = tmpqpd_Disk_MP;
        data->qpd_Disk_IOReq = tmpqpd_Disk_IOReq;

        data->qpd_Disk_str_DriveName = diskName;

        if ((data->qpd_Disk_hook_LocaliseNumber = AllocVec(sizeof(struct Hook), MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
        {
            //        data->qpd_Disk_hook_LocaliseNumber->h_Entry = HookEntry;
            data->qpd_Disk_hook_LocaliseNumber->h_Entry = (void *)QPDisk__HookFunc_LocaliseSize;
            //        data->qpd_Disk_hook_LocaliseNumber->h_SubEntry = (void *)QPDisk__HookFunc_LocaliseSize;
            if ((data->qpd_Disk_hook_LocaliseNumber->h_Data = AllocVec(sizeof(struct localeStreamHookData) + 100, MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
            {
                ((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_sizeval = data->qpd_Disk_hook_LocaliseNumber->h_Data + sizeof(struct localeStreamHookData);
                ((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_intdata = data;
                D(bug("[QuickPart:Disk.I] Localise String HookData @ %p, str_sizeval @ %p\n", data->qpd_Disk_hook_LocaliseNumber->h_Data, ((struct localeStreamHookData *)data->qpd_Disk_hook_LocaliseNumber->h_Data)->str_sizeval));
            }
        }

        data->qpd_Object_HardDisk_LayoutGrp = tmpqpd_Object_HardDisk_LayoutGrp;
        data->qpd_Object_HardDiskArea = tmpqpd_Object_HardDiskArea;
        data->qpd_Object_txt_DriveName = tmpqpd_Object_txt_DriveName;
        data->qpd_Object_txt_DriveCapacity = tmpqpd_Object_txt_DriveCapacity;
        data->qpd_Object_txt_DriveUnit = tmpqpd_Object_txt_DriveUnit;
        data->qpd_Object_VOID_spacer = tmpqpd_Object_VOID_spacer;
        /*** The Drive Image (rectangle) **/
        data->qpd_Object_HardDisk_Rectangle = tmpqpd_Object_HardDisk_Rectangle;
        data->qpd_Object_HardDisk_Contents = tmpqpd_Object_HardDisk_Contents;
        data->qpd_Object_VOID_weight = tmpqpd_Object_VOID_weight;

        data->qpd_Disk_str_DriveUnit = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);

        if (data->qpd_Disk_str_DriveUnit)
        {
            sprintf(data->qpd_Disk_str_DriveUnit, " Connected on %s %s %d ", data->qpd_Disk_Handler, _(WORD_Unit), data->qpd_Disk_Unit);
            set(data->qpd_Object_txt_DriveUnit, MUIA_Text_Contents, data->qpd_Disk_str_DriveUnit);
        }
        set(data->qpd_Object_HardDisk_Rectangle, MUIA_HorizWeight, 100);
        set(data->qpd_Object_VOID_weight, MUIA_HorizWeight, 0);
    }
    return (IPTR)self;
}

static IPTR QPDisk__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem              *tstate = message->ops_AttrList,
               *tag = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_QPart_Disk_Scale:
            if (DoMethod(data->qpd_Object_HardDisk_LayoutGrp, MUIM_Group_InitChange))
            {
                set(data->qpd_Object_HardDisk_Rectangle, MUIA_HorizWeight, (IPTR)tag->ti_Data);
                set(data->qpd_Object_VOID_weight, MUIA_HorizWeight, (IPTR)(100 - tag->ti_Data));
                DoMethod(data->qpd_Object_HardDisk_LayoutGrp, MUIM_Group_ExitChange);
            }
            break;
        default:
            break;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

static IPTR QPDisk__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR                          *store = message->opg_Storage;
    IPTR                    retval = TRUE;

    switch(message->opg_AttrID)
    {
    case MUIA_QPart_Disk_Geometry:
        *store = (IPTR)&data->qpd_Geom;
        break;

    case MUIA_QPart_Disk_Scale:
        *store = XGET(data->qpd_Object_HardDisk_Rectangle, MUIA_HorizWeight);
        break;

    case MUIA_QPart_Disk_Handler: 
        *store = (IPTR)data->qpd_Disk_Handler;
        break;

    case MUIA_QPart_Disk_Unit:
        *store = (IPTR)data->qpd_Disk_Unit;
        break;

    case MUIA_QPart_Disk_Name:
        *store = (IPTR)data->qpd_Disk_str_DriveName;
        break;

    case MUIA_QPart_Disk_Capacity:
        *(UQUAD *)store = data->qpd_Disk_val_DriveCapacity;
        break;

    default:
        retval = DoSuperMethodA(CLASS, self, (Msg)message);
        break;
    }

    return retval;
}

static IPTR QPDisk__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    D(bug("[QuickPart:Disk] DISPOSE (data=%p)\n",data));

    if (data->qpd_Disk_IOReq)
        DeleteIORequest((struct IORequest *)data->qpd_Disk_IOReq);

    if (data->qpd_Disk_MP)
        DeleteMsgPort(data->qpd_Disk_MP);

    return DoSuperMethodA(CLASS, self, message);
}
   
BOOPSI_DISPATCHER(IPTR, QPDisk_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPDisk__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_DISPOSE: 
        return QPDisk__OM_DISPOSE(CLASS, self, message);
    case OM_SET:
        return QPDisk__OM_SET(CLASS, self, (struct opSet *) message);
    case OM_GET:
        return QPDisk__OM_GET(CLASS, self,  (struct opGet *) message);

    case MUIM_QPart_Disk_ScanForParts:
        return QPDisk__MUIM_QPart_Disk_ScanForParts(CLASS, self, message);

    case MUIM_QPart_Disk_SaveDisk:
        return 0;

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
