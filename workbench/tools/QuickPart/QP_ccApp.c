/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#define INTUITION_NO_INLINE_STDARG
#include	<aros/debug.h>

#include	<proto/expansion.h>
#include	<proto/exec.h>
#include	<proto/partition.h>
#include	<proto/dos.h>
#include	<proto/intuition.h>
#include	<proto/muimaster.h>
#include	<proto/locale.h>
#include	<proto/utility.h>
#include	<proto/uuid.h>

#include   <proto/alib.h>

#include	<libraries/configvars.h>
#include	<libraries/expansionbase.h>
#include	<libraries/partition.h>
#include	<libraries/mui.h>

#include	<devices/trackdisk.h>
#include	<devices/scsidisk.h>

#include	<dos/dos.h>
#include	<dos/filehandler.h>

#include <utility/tagitem.h>

#include	<exec/memory.h>
#include	<exec/execbase.h>
#include	<exec/lists.h>
#include	<exec/nodes.h>
#include	<exec/types.h>
#include	<exec/ports.h>

#include	<zune/systemprefswindow.h>
#include	<zune/prefseditor.h>
#include	<zune/aboutwindow.h>

#include	<stdlib.h>
#include	<stdio.h>
#include	<strings.h>
#include <string.h>

#include "QP_Intern.h"

#define _QP_CCPARTITION_C

#include "QP_ccApp.h"
#include "QP_ccOperation.h"
#include "QP_ccTxt.h"
#include "QP_ccPartition.h"
#include "QP_ccPartitionContainer.h"
#include "QP_PartionColors.h"
#include "QP_locale.h"

//temp
extern Object *AQPart_txt_Info, *AQPart_grp_Info, *AQPart_spcr_Info;
extern Object *AQPart_but_Create, *AQPart_but_Delete;
extern Object *AQPart_grp_Pending, *AQPart_panel_Pending;
Object *QPart_AppWin_Legend = NULL;

/* QuickPart PARTITION Custom Class */

#define SETUP_INST_DATA struct QPApp_DATA *data = INST_DATA(CLASS, self)
extern struct   MUI_CustomClass      *mcc_qpfree;
extern struct   MUI_CustomClass      *mcc_qpop;

/** Standard Class Methods **/

#define LEGENDSWATCH(bgcol) (VGroup, Child, HVSpace, Child, (IPTR)(HGroup, Child, HVSpace, Child, (IPTR)(RectangleObject, MUIA_Background, (IPTR)(bgcol), MUIA_FixHeight, 15, MUIA_FixWidth , 15, End), Child, HVSpace, End), Child, HVSpace, End)

static IPTR QPApp__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
//    Object *QPart_AppWin_Legend;

    D(bug("[QuickPart:App] Creating App Object\n"));

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,

            SubWindow, (IPTR)(QPart_AppWin_Legend = WindowObject,
                    MUIA_Window_Title, __(WORD_LegendTitle),
                    MUIA_Window_Activate, FALSE,
                    WindowContents, (IPTR)(VGroup,
                            Child, (IPTR) CLabel(MUIX_B "Quick Part Legend" MUIX_N),
                            MUIA_Group_SameHeight, FALSE,
                            Child,(ScrollgroupObject,
                                MUIA_Scrollgroup_FreeHoriz, FALSE,
                                MUIA_Scrollgroup_FreeVert, TRUE,
                                MUIA_Scrollgroup_Contents,(IPTR)(VirtgroupObject,
                                    Child, (IPTR) ColGroup(2),
                                        Child, (IPTR) LEGENDSWATCH(DEF_PART_UNUSED),
                                        Child, (IPTR) LLabel( _(WORD_Unused)),
                                        Child, (IPTR) LEGENDSWATCH(DEF_PART_AROSEXT),
                                        Child, (IPTR) LLabel("AROS Extended"),
                                        Child, (IPTR) LEGENDSWATCH(DEF_PART_MBREXT),
                                        Child, (IPTR) LLabel("MBR Extended"),
                                        Child, (IPTR) LEGENDSWATCH(DEF_PART_GPTEXT),
                                        Child, (IPTR) LLabel("GPT"),
                                    End,
                                    Child, (IPTR) HVSpace,

                                    Child, (IPTR)(VGroup,
                                        GroupFrameT(_(WORD_Partitions)),
                                        Child, (IPTR) VSpace(0),
                                        Child, (IPTR)(ColGroup(5),
                                            MUIA_Group_VertSpacing, 2,
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_AFFS),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("AFFS"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_SFS),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("SFS"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_PFS),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("PFS"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_FAT),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("Fat32"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_NTFS),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("NTFS"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_EXT),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("Ext2"),
                                            Child, (IPTR)HSpace(0),

                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LEGENDSWATCH(DEF_PART_REISER),
                                            Child, (IPTR)HSpace(0),
                                            Child, (IPTR)LLabel("ReiserFS"),
                                            Child, (IPTR)HSpace(0),
                                        End),
                                        Child, (IPTR) VSpace(0),
                                    End),
                                End),
                            End),
                        End),
                    End),
            TAG_MORE, (IPTR)message->ops_AttrList   
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

#if (0)
        if (((mcc_qpdisk = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPDisk_DATA), QPDisk_Dispatcher))) &&
           ((mcc_qppartition = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPPartition_DATA), QPPartition_Dispatcher))) &&
           ((mcc_qppartitioncontainer = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct QPPartitionContainer_DATA), QPPartitionContainer_Dispatcher))))
        {
        }
#endif
        DoMethod(QPart_AppWin_Legend, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,  QPart_AppWin_Legend, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        D(bug("[QuickPart:App] App Object @ 0x%p, Data @ 0x%p\n", self, data));
    }

    return (IPTR)self;
}

static IPTR QPApp__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR                          *store = message->opg_Storage;
    IPTR    	      		 retval = TRUE;

    switch(message->opg_AttrID)
    {
    case MUIA_QPart_ccApp_ActivePart:
        *store = (IPTR)data->qpad_Active;
        break;
    default:
        retval = DoSuperMethodA(CLASS, self, (Msg)message);
        break;
    }

    return retval;
}

static IPTR QPApp__OM_SET
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
        case MUIA_QPart_ccApp_ActivePart:
            {
                if (data->qpad_Active)
                {
                    Object *oldActive = data->qpad_Active;
                    data->qpad_Active = NULL;
                    set(_win(oldActive), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
                    MUI_Redraw(oldActive, MADF_DRAWOBJECT);
                }
                data->qpad_Active = (Object *)tag->ti_Data;
                if (data->qpad_Active)
                {
                    UQUAD partStart = 0, partEnd = 0;
                    IPTR    partTypeStr = 0, partSizeStr = 0,
                            partDevStr = 0, partPos = 0;
                    struct PartitionType partType;
                    char partDesc[256];
                    char *ContainerStr = "", *ContainerSubStr = "";
                    Object *partParent = NULL;
                    int stroff = 0, spfrv;

                    D(bug("[QuickPart:App] Setting Active Partition Object @ 0x%p <class @ 0x%p>\n", data->qpad_Active, OCLASS(data->qpad_Active)));
                    partDesc[0] = '\0';
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_Start, &partStart);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_End, &partEnd);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_Type, &partType);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_TypeStr, &partTypeStr);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_SizeStr, &partSizeStr);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_DOSDevStr, &partDevStr);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_Position, &partPos);
                    GET(data->qpad_Active, MUIA_QPart_ccPartition_Parent, &partParent);
                    if (partType.id_len == 1)
                    {
                        switch (partType.id[0])
                        {
                        case 5:                                 /* EBR CHS */
                            ContainerSubStr = "Extended (CHS) ";
                            break;

                        case 15:                                /* EBR LBA*/
                            ContainerSubStr = "Extended (LBA) ";
                            break;

                        case 48:                                /* Logical AROS Extended Partition */
                            ContainerSubStr = "RDB Container ";
                            break;                            
                        }
                    }
                    if (partParent)
                    {
                        IPTR parentType;
                        GET(partParent, MUIA_QPart_ccPartitionContainer_Type, &parentType);
                        switch(parentType)
                        {
                        case PHPTT_RDB:
                            ContainerStr = "RDB ";
                            break;

                        case PHPTT_MBR:
                            {
                                ContainerStr = "MBR ";
                                if (!ContainerSubStr)
                                    ContainerSubStr = "primary ";
                            }
                            break;

                        case PHPTT_GPT:
                            ContainerStr = "GPT ";
                            break;

                        default:
                            break;
                        }
                    }
                    spfrv = snprintf(partDesc, (256 - stroff), MUIX_B "Details:" MUIX_N "\n%s%spartition #%u\n" MUIX_B "Format:" MUIX_N "\n ", ContainerStr, ContainerSubStr, (partPos & 0xFF) + 1);
                    if (spfrv > 0)
                        stroff += spfrv;
                    if (partTypeStr)
                    {
                        spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "%s ", (char *)partTypeStr);
                        if (spfrv > 0)
                            stroff += spfrv;
                    }
                    spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "<ID ");
                    if (spfrv > 0)
                        stroff += spfrv;
                    switch (partType.id_len)
                    {
                        case 1:
                            spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "%02u>", partType.id[0]);
                            break;
                        case 4:
                            spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "%08x>", AROS_BE2LONG(*(ULONG *)&partType.id[0]));
                            break;
                        case 16:
                            UUID_Unparse((const uuid_t *)&partType.id[0], (char *)((IPTR)partDesc + stroff));
                            stroff = strlen(partDesc);
                            //fall through
                        default:
                            spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "unknown>");
                            break;
                    }
                    if (spfrv > 0)
                        stroff += spfrv;
                    if (partDevStr)
                    {
                        spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), ", Mounted as %s\n", (char *)partDevStr);
                        if (spfrv > 0)
                            stroff += spfrv;
                    }
                    else
                    {
                        spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "\n");
                        if (spfrv > 0)
                            stroff += spfrv;
                    }
                    if (partSizeStr)
                    {
                        spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), MUIX_B "Capacity:" MUIX_N "\n %s\n", partSizeStr);
                        if (spfrv > 0)
                            stroff += spfrv;
                    }
                    spfrv = snprintf((char *)((IPTR)partDesc + stroff), (256 - stroff), "%s" MUIX_B "Start Block:" MUIX_N "\n %llu\n" MUIX_B "End Block:" MUIX_N "\n %llu", (stroff) ? "\n" : "", partStart, partEnd);
                    if (spfrv > 0)
                        stroff += spfrv;
                    if (DoMethod(AQPart_grp_Info, MUIM_Group_InitChange))
                    {
#if (0)
                        SET(AQPart_txt_Info, MUIA_Text_Contents, partDesc);
#else
                        SET(AQPart_txt_Info, MUIA_Floattext_Text, partDesc);
                        SET(AQPart_txt_Info, MUIA_VertWeight, 100);
                        SET(AQPart_spcr_Info, MUIA_VertWeight, 0);
#endif
                        DoMethod(AQPart_grp_Info, MUIM_Group_ExitChange);
                    }
                    set(AQPart_but_Create, MUIA_Disabled, TRUE);
                    set(AQPart_but_Delete, MUIA_Disabled, FALSE);
                    MUI_Redraw(data->qpad_Active, MADF_DRAWUPDATE);
                    Object *partObj = NULL;
                    get(data->qpad_Active, MUIA_QPart_ccPartition_PartObj, &partObj);
                    if (partObj)
                        set(_win(data->qpad_Active), MUIA_Window_ActiveObject, partObj);
                }
                else
                {
                    if (DoMethod(AQPart_grp_Info, MUIM_Group_InitChange))
                    {
#if (0)
                        SET(AQPart_txt_Info, MUIA_Text_Contents, "");
#else
                        SET(AQPart_txt_Info, MUIA_Floattext_Text, "");
#endif
                        SET(AQPart_txt_Info, MUIA_VertWeight, 0);
                        SET(AQPart_spcr_Info, MUIA_VertWeight, 100);
                        DoMethod(AQPart_grp_Info, MUIM_Group_ExitChange);
                    }
                    set(AQPart_but_Delete, MUIA_Disabled, TRUE);
                }
            }
            break;

        default:
            break;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

static IPTR QPApp__MUIM_QPart_ccApp_DeleteActive
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    Object *Active_Part;
    IPTR    	      		 retval = TRUE;
    if ((Active_Part = data->qpad_Active) != NULL)
    {
        struct PartCont_ChildIntern *newSpaceNode = NULL, *partChildNode = NULL;
        Object *PartContObj = NULL, *newOperObj;

        data->qpad_Active = NULL;
        SET(self, MUIA_QPart_ccApp_ActivePart, NULL);

        GET(Active_Part, MUIA_QPart_ccPartition_Parent, &PartContObj);
        GET(Active_Part, MUIA_UserData, &partChildNode);

        D(bug("[QuickPart:App] Deleting Partition Object @ 0x%p <Container @ 0x%p>\n", Active_Part, PartContObj));

        newOperObj = NewObject(mcc_qpop->mcc_Class, NULL,
                MUIA_QPart_ccOperation_Type, MUIV_QPart_ccOperation_Delete,
                MUIA_QPart_ccOperation_Target, (IPTR)Active_Part,
            TAG_DONE);
        if (newOperObj)
        {
            if ((newSpaceNode = AllocVec(sizeof(struct PartCont_ChildIntern), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
            {
                newSpaceNode->PCCN_ChildObj = NewObject(mcc_qpfree->mcc_Class, NULL,
                    MUIA_HorizWeight, partChildNode->PCCN_Weight,
                    MUIA_UserData, (IPTR)newSpaceNode,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Background, (IPTR)DEF_PART_UNUSED,
                TAG_DONE);
                newSpaceNode->PCCN_Type = PCCN_TYPE_FREE;
                newSpaceNode->PCCN_Start = partChildNode->PCCN_Start;
                newSpaceNode->PCCN_End = partChildNode->PCCN_End;
                newSpaceNode->PCCN_Weight = partChildNode->PCCN_Weight;
                newSpaceNode->PCCN_Delete = FALSE;

                D(bug("[QuickPart:App] New Internal Child Node Created for FREESPACE Object @ 0x%p\n", newSpaceNode));
                D(bug("[QuickPart:App] FREESPACE Weight = %u\n", newSpaceNode->PCCN_Weight));
                if (DoMethod(PartContObj, MUIM_Group_InitChange))
                {
                    DoMethod(PartContObj, OM_REMMEMBER, Active_Part);
                    DoMethod(PartContObj, MUIM_QPart_ccPartitionContainer_AddFree, newSpaceNode);
                    DoMethod(PartContObj, MUIM_Group_ExitChange);
                    if (DoMethod(AQPart_grp_Pending, MUIM_Group_InitChange))
                    {
                        DoMethod(AQPart_grp_Pending, OM_ADDMEMBER, newOperObj);
                        SET(Active_Part, MUIA_Disabled, TRUE);
                        DoMethod(AQPart_grp_Pending, MUIM_Group_ExitChange);
                    }
                    SET(AQPart_panel_Pending, MUIA_ShowMe, TRUE);
                }
            }
            else
            {
                
            }
        }
        else
        {
            
        }
    }

    return retval;
}

BOOPSI_DISPATCHER(IPTR, QPApp_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPApp__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_GET:
        return QPApp__OM_GET(CLASS, self,  (struct opGet *) message);
    case OM_SET:
        return QPApp__OM_SET(CLASS, self,  (struct opSet *) message);

    case MUIM_QPart_ccApp_DeleteActive:
        return QPApp__MUIM_QPart_ccApp_DeleteActive(CLASS, self, message);

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
