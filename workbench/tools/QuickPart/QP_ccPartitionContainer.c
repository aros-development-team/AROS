/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#include <aros/locale.h>

#include "QP_Intern.h"

#define _QP_CCPARTITIONCONTAINER_C

#include "QP_ccApp.h"
#include "QP_ccDisk.h"
#include "QP_ccPartitionContainer.h"
#include "QP_ccPartition.h"
#include "QP_PartionColors.h"

/* QuickPart PARTITION CONTAINER Custom Class */

#define SETUP_INST_DATA struct QPPartitionContainer_DATA *data = INST_DATA(CLASS, self)
extern struct   MUI_CustomClass      *mcc_qpfree;

struct NewEntryData
{
    Object *ned_Object;
    Object  *ned_InsertBefore;
    Object  *ned_InsertAfter;
    IPTR    ned_PreSpacerStart;
    IPTR    ned_PreSpacerEnd;
    IPTR    ned_PostSpacerStart;
    IPTR    ned_PostSpacerEnd; 
    BOOL    ned_InsertSpacerPre;
    BOOL    ned_InsertSpacerPost;
};

BOOL InsertPartEntry(Class *CLASS, Object *self, struct NewEntryData *ned, UQUAD *startPtr, UQUAD *endPtr, struct PartCont_ChildIntern *found_PartNode)
{
    SETUP_INST_DATA;

    struct PartCont_ChildIntern *spacerPostNode = NULL, *spacerPreNode = NULL;
    struct PartCont_ChildIntern *newPartNode = NULL;

#if (0)
    Object_Partition = HGroup,
        MUIA_Background, (IPTR)DEF_PART_AFFS,
        Child, (IPTR)(VGroup,
            MUIA_InnerLeft, 1,
            MUIA_InnerTop, 0,
            MUIA_InnerRight, 1,
            MUIA_InnerBottom, 1,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            Child, (RectangleObject,
                MUIA_FixHeight, 10,
            End),
            Child, (RectangleObject,
                MUIA_Background, (IPTR)DEF_DRIVEPANE_BACK,
            End),
        End),
    End;
#endif

    D(bug("[QuickPart:Cont] InsertPartEntry: Object_Partition @ %p\n", ned->ned_Object));

    //Insert the new objects First
    if (ned->ned_InsertSpacerPre != FALSE)
    {
        D(bug("[QuickPart:Cont] InsertPartEntry: Creating New start FREESPACE Object\n"));
        if ((spacerPreNode = AllocVec(sizeof(struct PartCont_ChildIntern), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
        {
            spacerPreNode->PCCN_ChildObj = NewObject(mcc_qpfree->mcc_Class, NULL,
                MUIA_UserData, (IPTR)spacerPreNode,
                MUIA_QPart_ccPartition_Parent, (IPTR)self,
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_Background, (IPTR)DEF_PART_UNUSED,
            TAG_DONE);
            spacerPreNode->PCCN_Type = PCCN_TYPE_FREE;
            spacerPreNode->PCCN_Start = ned->ned_PreSpacerStart;
            spacerPreNode->PCCN_End = ned->ned_PreSpacerEnd;
            spacerPreNode->PCCN_Delete = FALSE;
            D(bug("[QuickPart:Cont] InsertPartEntry: New Internal Child Node Created for start FREESPACE Object @ 0x%p\n", spacerPreNode));
            spacerPreNode->PCCN_Weight = ((spacerPreNode->PCCN_End - spacerPreNode->PCCN_Start) * 100) / (data->qpcd_End - data->qpcd_Start);
            D(bug("[QuickPart:Cont] InsertPartEntry: <Pre> FREESPACE Weight = %u\n", spacerPreNode->PCCN_Weight));
        }
    }

    if ((newPartNode = AllocVec(sizeof(struct PartCont_ChildIntern), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
        newPartNode->PCCN_ChildObj = ned->ned_Object;
        newPartNode->PCCN_Type = PCCN_TYPE_PARTITION;
        newPartNode->PCCN_Start = *startPtr;
        newPartNode->PCCN_End = *endPtr;
        newPartNode->PCCN_Delete = FALSE;
        SET(ned->ned_Object, MUIA_UserData, (IPTR)newPartNode);

        D(bug("[QuickPart:Cont] InsertPartEntry: New Child Start = %llu, End = %llu\n", newPartNode->PCCN_Start, newPartNode->PCCN_End));
        D(bug("[QuickPart:Cont] InsertPartEntry: New Internal Child Node Created @ 0x%p for Partition Object <@ 0x%p>\n", newPartNode, ned->ned_Object));
        if ((newPartNode->PCCN_Start == data->qpcd_Start) && (newPartNode->PCCN_End == data->qpcd_End))
            newPartNode->PCCN_Weight = 100;
        else
            newPartNode->PCCN_Weight = ((newPartNode->PCCN_End - newPartNode->PCCN_Start) * 100) / (data->qpcd_End - data->qpcd_Start);
        D(bug("[QuickPart:Cont] InsertPartEntry: New Part Weight = %u\n", newPartNode->PCCN_Weight));
    }

    if (ned->ned_InsertSpacerPost != FALSE)
    {
        D(bug("[QuickPart:Cont] InsertPartEntry: Creating New end FREESPACE Object\n"));
        if ((spacerPostNode = AllocVec(sizeof(struct PartCont_ChildIntern), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
        {
            spacerPostNode->PCCN_ChildObj = NewObject(mcc_qpfree->mcc_Class, NULL,
                MUIA_UserData, (IPTR)spacerPostNode,
                MUIA_QPart_ccPartition_Parent, (IPTR)self,
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_Background, (IPTR)DEF_PART_UNUSED,
            TAG_DONE);
            spacerPostNode->PCCN_Type = PCCN_TYPE_FREE;
            spacerPostNode->PCCN_Start = ned->ned_PostSpacerStart;
            spacerPostNode->PCCN_End = ned->ned_PostSpacerEnd;
            spacerPostNode->PCCN_Delete = FALSE;
            D(bug("[QuickPart:Cont] InsertPartEntry: New Internal Child Node Created for end FREESPACE Object @ 0x%p\n", spacerPostNode));
            spacerPostNode->PCCN_Weight = ((spacerPostNode->PCCN_End - spacerPostNode->PCCN_Start) * 100) / (data->qpcd_End - data->qpcd_Start);
            D(bug("[QuickPart:Cont] InsertPartEntry: <Post> FREESPACE Weight = %u\n", spacerPostNode->PCCN_Weight));
        }
    }

    if (DoMethod(self, MUIM_Group_InitChange))
    {
        if ((ned->ned_InsertSpacerPre == TRUE )&&(spacerPreNode != NULL))
        {
            D(bug("[QuickPart:Cont] InsertPartEntry: Inserting New FREESPACE Object Before..\n"));
            DoMethod(self, MUIM_QPart_ccPartitionContainer_AddFree, spacerPreNode);
        }
        D(bug("[QuickPart:Cont] InsertPartEntry: Inserting New Object..\n"));

        AddTail(&data->qpcd_Part_Children, &newPartNode->PCCN_Node);
        DoMethod(self, OM_ADDMEMBER, ned->ned_Object);

        if (found_PartNode && found_PartNode->PCCN_Delete)
        {
            DoMethod(self, OM_REMMEMBER, found_PartNode->PCCN_ChildObj);
        }
        if ((ned->ned_InsertSpacerPost == TRUE)&&(spacerPostNode != NULL))
        {
            D(bug("[QuickPart:Cont] InsertPartEntry: Inserting New FREESPACE Object After..\n"));
            DoMethod(self, MUIM_QPart_ccPartitionContainer_AddFree, spacerPostNode);
        }
        DoMethod(self, MUIM_Group_ExitChange);

        D(bug("[QuickPart:Cont] InsertPartEntry: Inserted Succesfully\n"));
    }

    D(bug("[QuickPart:Cont] InsertPartEntry: All New Objects Created for Partition Object\n"));
    return TRUE;
}

/** CLASS Methods **/
static IPTR QPPartitionContainer__MUIM_QPart_ccPartitionContainer_AddPart
(
    Class *CLASS, Object *self, struct MUIP_QPart_ccPartitionContainer_AddPart *message
)
{
    SETUP_INST_DATA;

    struct PartCont_ChildIntern *found_PartNode = NULL, *tmpNode;
    Object                      *Object_Partition = NULL;
    UQUAD                      Part_Start = 0;
    UQUAD                      Part_End = 0;
    BOOL                        Part_Inserted = FALSE;

    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart(ph @ %p)\n", message->PHToAdd));

    if ((Object_Partition = NewObject(mcc_qppartition->mcc_Class, NULL,
                MUIA_QPart_Disk_Object, (IPTR)data->qpcd_Object_Disk,
                MUIA_QPart_Disk_Geometry, (IPTR)data->qpcd_Geom,
                MUIA_QPart_ccPartition_Handle, (IPTR)message->PHToAdd,
                MUIA_QPart_ccPartition_Parent, (IPTR)self,
                TAG_DONE)) != NULL)
    {
        //warning "TODO: Fix to obtain real blocksize"
        #define TMP_BLOCKSIZE  512

        get(Object_Partition, MUIA_QPart_ccPartition_Start, &Part_Start);
        get(Object_Partition, MUIA_QPart_ccPartition_End, &Part_End);

        D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Start %llu, End %llu\n", Part_Start, Part_End));

//CheckChildrenClean:
        found_PartNode = NULL;
        ForeachNodeSafe(&data->qpcd_Part_Children, found_PartNode, tmpNode)
        {
            struct NewEntryData ned;
            ned.ned_InsertBefore = NULL;
            ned.ned_InsertAfter = NULL;
            ned.ned_InsertSpacerPre = FALSE;
            ned.ned_InsertSpacerPost = FALSE;
            ned.ned_PreSpacerStart = 0;
            ned.ned_PreSpacerEnd = 0;
            ned.ned_PostSpacerStart = 0;
            ned.ned_PostSpacerEnd = 0;
            ned.ned_Object = Object_Partition;

            if ((found_PartNode->PCCN_ChildObj == Object_Partition) || (found_PartNode->PCCN_Delete == TRUE)) continue;
         
            if (((found_PartNode->PCCN_End >= Part_Start) && (found_PartNode->PCCN_Start <= Part_Start)) ||
                ((found_PartNode->PCCN_Start <= Part_End) && (found_PartNode->PCCN_End >= Part_End)))
            {
                if (found_PartNode->PCCN_Type == PCCN_TYPE_FREE)
                {
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: New Partition overlaps End of FREESPACE marker!\n"));

                    ned.ned_InsertAfter = found_PartNode->PCCN_ChildObj;

                    if (Part_Start >= (found_PartNode->PCCN_Start + (1 * TMP_BLOCKSIZE)))
                    {
#if (0)
                        ned.ned_InsertSpacerPre = TRUE;
                        ned.ned_PreSpacerStart = found_PartNode->PCCN_Start;
                        ned.ned_PreSpacerEnd   = (Part_Start - (1 * TMP_BLOCKSIZE));
#else
                        // Reduce existing spacer size
                        found_PartNode->PCCN_End = Part_Start - 1;
                        found_PartNode->PCCN_Weight = ((found_PartNode->PCCN_End - found_PartNode->PCCN_Start) * 100) / (data->qpcd_End - data->qpcd_Start);
                        D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Existing FREESPACE Weight = %u\n", found_PartNode->PCCN_Weight));
#endif
                    }
                    else
                    {
                        // Delete
                        found_PartNode->PCCN_Delete = TRUE;
                    }

                    if (Part_End <= (found_PartNode->PCCN_End - (1 * TMP_BLOCKSIZE)))
                    {
                        ned.ned_InsertSpacerPost = TRUE;
                        ned.ned_PostSpacerStart = (Part_End + (1 * TMP_BLOCKSIZE));
                        ned.ned_PostSpacerEnd   = found_PartNode->PCCN_End;
                    }
                }
                else
                {
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: ERROR: New Partition overlaps End of an existing Partition! (node @ 0x%p)\n", found_PartNode));
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Existing End = %llu, Part Start = %llu\n", found_PartNode->PCCN_End, Part_Start));
                }
            }
#if (0)
            else if ((found_PartNode->PCCN_Start <= Part_End) && (found_PartNode->PCCN_End >= Part_End))
            {
                if (found_PartNode->PCCN_Type == PCCN_TYPE_FREE)
                {
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: New Partition overlaps Start of FREESPACE marker!\n"));

                    ned.ned_InsertBefore = found_PartNode->PCCN_ChildObj;

                    if (Part_Start >= (found_PartNode->PCCN_Start + (1 * TMP_BLOCKSIZE)))
                    {

#if (0)
                        ned.ned_InsertSpacerPre = TRUE;
//warning "TODO: Adjust for block size?!?"
                        ned.ned_PreSpacerStart = found_PartNode->PCCN_Start;
                        ned.ned_PreSpacerEnd   = (Part_Start - (1 * TMP_BLOCKSIZE));
#else
                        // Reduce existing spacer size
                        found_PartNode->PCCN_Start = Part_End + 1;
                        found_PartNode->PCCN_Weight = ((found_PartNode->PCCN_End - found_PartNode->PCCN_Start) * 100) / (data->qpcd_End - data->qpcd_Start);
                        D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Existing FREESPACE Weight = %u\n", found_PartNode->PCCN_Weight));
#endif
                    }
                    else
                    {
                        // Delete
                        found_PartNode->PCCN_Delete = TRUE;
                    }

                    if (Part_End <=  (found_PartNode->PCCN_End - (1 * TMP_BLOCKSIZE)))
                    {
                        ned.ned_InsertSpacerPost = TRUE;
                        ned.ned_PostSpacerStart = (Part_End + (1 * TMP_BLOCKSIZE));
                        ned.ned_PostSpacerEnd   = found_PartNode->PCCN_End;
                    }
                }
                else
                {
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: ERROR: New Partition overlaps Start of an existing Partition! (node @ 0x%p)\n", found_PartNode));
                    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Existing Start = %llu, Part End = %llu\n", found_PartNode->PCCN_Start, Part_End));
                }
            }
#endif
            if (!Part_Inserted)
            {
                ///////////////////////////
                Part_Inserted = InsertPartEntry(CLASS, self, &ned, &Part_Start, &Part_End, found_PartNode);
                if (Part_Inserted)
                    break;
                ///////////////////////////
            }
            else
            {
#if (0)
                D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Removing Old Object 0x%p\n", found_PartNode->PCCN_ChildObj));
                if (DoMethod(self, MUIM_Group_InitChange))
                {
                    //Remove the old object...
                    DoMethod(self, OM_REMMEMBER, found_PartNode->PCCN_ChildObj);
                    DoMethod(self, MUIM_Group_ExitChange);
                    found_PartNode->PCCN_Delete = TRUE;
                }
#endif
            }
        }
    }
    if (found_PartNode && found_PartNode->PCCN_Delete)
    {
        Remove((struct Node *)found_PartNode);
    }
    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_AddPart: Finished adding children\n"));

    return (IPTR)Object_Partition;
}

static IPTR QPPartitionContainer__MUIM_QPart_ccPartitionContainer_AddFree
(
    Class *CLASS, Object *self, struct MUIP_QPart_ccPartitionContainer_AddFree *message
)
{
    SETUP_INST_DATA;
    struct PartCont_ChildIntern *partNode = NULL, *tmpNode;
    BOOL addFree = TRUE, addObj = TRUE;

    ForeachNodeSafe(&data->qpcd_Part_Children, partNode, tmpNode)
    {
        if (partNode->PCCN_Type == PCCN_TYPE_FREE)
        {
            if ((partNode->PCCN_End >= message->FSNode->PCCN_Start) && (partNode->PCCN_Start <= message->FSNode->PCCN_Start))
            {
                addFree = FALSE;
                addObj = FALSE;
                // Update free area
                partNode->PCCN_End = message->FSNode->PCCN_End;
            }
            if ((partNode->PCCN_Start <= message->FSNode->PCCN_End) && (partNode->PCCN_End >= message->FSNode->PCCN_End))
            {
                if (!addFree)
                {
                    // Merge free areas ...
                }
                addFree = FALSE;
                addObj = FALSE;
                // Update free area
                partNode->PCCN_Start = message->FSNode->PCCN_Start;
            }
            if (addFree)
            {
                if (message->FSNode->PCCN_Start < partNode->PCCN_Start)
                {
                    Insert(&data->qpcd_Part_Children, &message->FSNode->PCCN_Node, GetPred(&partNode->PCCN_Node));
                    addFree = FALSE;
                    break;
                }
            }
        }
    }

    if (addFree)
    {
        AddTail(&data->qpcd_Part_Children, &message->FSNode->PCCN_Node);
    }
    if (addObj)
    {
        DoMethod(self, OM_ADDMEMBER, message->FSNode->PCCN_ChildObj);
    }

    return TRUE;
}

static IPTR QPPartitionContainer__MUIM_QPart_ccPartitionContainer_FindParts
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    struct PartitionHandle    *found_ph = NULL;

    D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_UpdateParts(ph @ %p)\n", data->qpcd_Part_PHandle));

    if (OpenPartitionTable(data->qpcd_Part_PHandle) == 0)  
    {
        Object *newPartObj = NULL;
        for
         (
             found_ph = (struct PartitionHandle *)data->qpcd_Part_PHandle->table->list.lh_Head;
             found_ph->ln.ln_Succ;
             found_ph = (struct PartitionHandle *)found_ph->ln.ln_Succ
         )
        {
            D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_UpdateParts: Child Partition @ %p\n", found_ph));
            newPartObj = (Object *)DoMethod(self, MUIM_QPart_ccPartitionContainer_AddPart, found_ph);
        }
        if (newPartObj)
        {
            Object *newContainer = NULL;
            GET(newPartObj, MUIA_QPart_ccPartition_Container, &newContainer);
            if (newContainer)
            {
                D(bug("[QuickPart:Cont] MUIM_QPart_ccPartitionContainer_UpdateParts: Causing new child container object to scan for children ..\n"));
                DoMethod(newContainer, MUIM_QPart_ccPartitionContainer_UpdateParts);
            }
        }
        ClosePartitionTable(data->qpcd_Part_PHandle);
    }

    return TRUE;
}

static IPTR QPPartitionContainer__MUIM_QPart_ccPartitionContainer_UpdateWeights
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    D(bug("[QuickPart:Cont] %s()\n", __func__));

    struct List *grpChildren = NULL;

    GET(self, MUIA_Group_ChildList, &grpChildren);
    if (grpChildren)
    {
        Object *grpChild;

        D(bug("[QuickPart:Cont] %s: Group Children @ 0x%p\n", __func__, grpChildren));
        APTR childListState = grpChildren->lh_Head;
        while (grpChild = NextObject(&childListState))
        {
            struct PartCont_ChildIntern *childNode = NULL;

            D(bug("[QuickPart:Cont] %s:       Child @ 0x%p\n", __func__, grpChild));

            GET(grpChild, MUIA_UserData, &childNode);
            if (childNode)
            {
                D(bug("[QuickPart:Cont] %s:         data @ 0x%p\n", __func__, childNode));
                SET(grpChild, MUIA_HorizWeight, (childNode->PCCN_Weight & 0xFF));
                D(bug("[QuickPart:Cont] %s:         weight = %u\n", __func__, (childNode->PCCN_Weight & 0xFF)));
            }
        }
    }

    return TRUE;
}

static IPTR QPPartitionContainer__MUIM_QPart_ccPartitionContainer_UpdateParts
(
    Class *CLASS, Object *self, Msg message
)
{
    D(bug("[QuickPart:Cont] %s()\n", __func__));

    DoMethod(self, MUIM_QPart_ccPartitionContainer_FindParts);
    DoMethod(self, MUIM_QPart_ccPartitionContainer_UpdateWeights);

    return TRUE;
}

/** Standard Class Methods **/
static IPTR QPPartitionContainer__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct      TagItem             *tags = NULL,
                                    *tag = NULL;

    /* Tmp Object Pointers Used to create our "Partition" */
    Object                          *PartCont_Disk = NULL, *PartCont_Parent = NULL, *PartCont_Part = NULL;
    Object                          *Object_Contents = NULL, *contSelect = NULL;

    /* Tmp strings & values */
    struct PartitionHandle          *Part_PHandle = NULL;
    const struct PartitionAttribute *Part_TAttrlist = NULL; /* supported partition table attributes */
    const struct PartitionAttribute *Part_PAttrlist = NULL; /* supported partition attributes */
    struct DriveGeometry            *Disk_Geom = NULL;
    IPTR                            PartCont_BGColor = (IPTR)NULL;
    IPTR                            Part_Reserved = 0;
    IPTR                            Part_MaxPartitions = 0;
    IPTR                            Part_Type = 0;
    int                             SecCylinder = 0, SecSize = 0;
    
    tags = message->ops_AttrList;
    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Background:
            PartCont_BGColor = (IPTR)tag->ti_Data;
            D(bug("[QuickPart:Cont.I] BGColor set\n"));
            break;

        case MUIA_QPart_ccPartition_Parent:
            PartCont_Parent = (Object *)tag->ti_Data;
            break;

        case MUIA_QPart_ccPartitionContainer_PartObj:
            PartCont_Part = (Object *)tag->ti_Data;
            break;

        case MUIA_QPart_Disk_Object:
            PartCont_Disk = (Object *)tag->ti_Data;
            D(bug("[QuickPart:Cont.I] Disk Obj @ %p\n", PartCont_Disk));
            break;

        case MUIA_QPart_Disk_Geometry:
            Disk_Geom = (struct DriveGeometry *)tag->ti_Data;
            D(bug("[QuickPart:Cont.I] Disk Geom @ %p\n", Disk_Geom));
            break;

        case MUIA_QPart_ccPartition_Handle:
            Part_PHandle = (struct PartitionHandle *)tag->ti_Data;
            D(bug("[QuickPart:Cont.I] PartitionHandle @ %p\n", Part_PHandle));
            break;

        default:
            continue; /* Don't supress non-processed tags */
        }
        tag->ti_Tag = TAG_IGNORE;
    }

    if ((PartCont_Disk == NULL) || (Part_PHandle == NULL) || (Disk_Geom == NULL))
        return (IPTR)NULL;

    if (OpenPartitionTable(Part_PHandle) == 0)
    {
        struct TagItem ptaTags[] = 
        {
            { PTT_TYPE, (IPTR)&Part_Type},
            { PTT_RESERVED, (IPTR)&Part_Reserved},
            { PTT_MAX_PARTITIONS, (IPTR)&Part_MaxPartitions},
            { TAG_DONE, 0 }
        };

        D(bug("[QuickPart:Cont.I] Opened Partition Table for Handle @ %p\n", Part_PHandle));

        Part_TAttrlist = QueryPartitionTableAttrs(Part_PHandle);
        Part_PAttrlist = QueryPartitionAttrs(Part_PHandle);

        D(bug("[QuickPart:Cont.I] TAttrlist = %p PAttrlist = %p\n", Part_TAttrlist, Part_PAttrlist));

        D(bug("[QuickPart:Cont.I] Reading Table Attributes ...\n"));

        GetPartitionTableAttrs(Part_PHandle, ptaTags);

        if (PartCont_BGColor == (IPTR)NULL)
        {
            GetPartitionContainerDrawAttribs(&Part_Type, &PartCont_BGColor, NULL);
        }

        D(bug("[QPccPart.I] Creating CONTAINER Object\n"));

        self = (Object *) DoSuperNewTags
         (
            CLASS, self, NULL,

            MUIA_Frame,  MUIV_Frame_ReadList,
            MUIA_Background, (IPTR)PartCont_BGColor,

            MUIA_InnerLeft, 0,
            MUIA_InnerTop, 0,
            MUIA_InnerRight, 0,
            MUIA_InnerBottom, 0,

            MUIA_Group_Horiz, TRUE,
            MUIA_Group_SameWidth, FALSE,
            MUIA_Group_HorizSpacing, 1,

            Child,(IPTR)(contSelect = RectangleObject,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_CycleChain, 1,
                    MUIA_FixWidth, 10,
            End),
            Child,(IPTR)(Object_Contents = NewObject(mcc_qpfree->mcc_Class, NULL,
                MUIA_HorizWeight, 100,
            TAG_DONE)),

            TAG_MORE, (IPTR) message->ops_AttrList   
         );
    }
    else self = NULL;

    if (self)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        struct PartCont_ChildIntern           *contSpacerNode = NULL;
        struct TagItem paTags[] = 
        {
            { PT_DOSENVEC, (IPTR)&data->qpcd_Part_DE },
            { TAG_DONE,             0 }
        };

        GetPartitionAttrs( Part_PHandle,  paTags);

        data->qpcd_Geom = Disk_Geom;
        data->qpcd_Object_Disk = PartCont_Disk;
        data->qpcd_Object_Parent = PartCont_Parent;
        data->qpcd_Object_Select = contSelect;
        data->qpcd_Object_Part = PartCont_Part;

        SecCylinder = (Part_PHandle->dg.dg_TotalSectors / Part_PHandle->dg.dg_Cylinders);
        SecSize = Part_PHandle->dg.dg_SectorSize;

        data->qpcd_Part_PHandle        = Part_PHandle;

        NEWLIST(&data->qpcd_Part_Children);

        D(bug("[QuickPart:Cont.I] LowCyl = %u, HighCyl= %u\n", data->qpcd_Part_DE.de_LowCyl, data->qpcd_Part_DE.de_HighCyl));
        D(bug("[QuickPart:Cont.I] Surfaces = %u, BlocksPerTrack = %u\n", data->qpcd_Part_DE.de_Surfaces, data->qpcd_Part_DE.de_BlocksPerTrack));

        D(bug("[QuickPart:Cont.I] Sector Size = %u, Sectors per Cyl = %u\n", SecSize, SecCylinder));

        contSpacerNode = AllocVec(sizeof(struct PartCont_ChildIntern), MEMF_CLEAR|MEMF_PUBLIC);
        contSpacerNode->PCCN_ChildObj = Object_Contents;
        contSpacerNode->PCCN_Weight = 100;
        contSpacerNode->PCCN_Type  = PCCN_TYPE_FREE;
        contSpacerNode->PCCN_Start = data->qpcd_Part_DE.de_LowCyl * data->qpcd_Part_DE.de_Surfaces * data->qpcd_Part_DE.de_BlocksPerTrack;
        contSpacerNode->PCCN_End = (data->qpcd_Part_DE.de_HighCyl + 1) * data->qpcd_Part_DE.de_Surfaces * data->qpcd_Part_DE.de_BlocksPerTrack - 1;
        SET(Object_Contents, MUIA_QPart_ccPartition_Parent, (IPTR)self);
        SET(Object_Contents, MUIA_UserData, (IPTR)contSpacerNode);

        data->qpcd_Start = contSpacerNode->PCCN_Start;
        data->qpcd_End = contSpacerNode->PCCN_End;

        D(bug("[QuickPart:Cont.I] Initial Child Node @ 0x%p\n", contSpacerNode));
        D(bug("[QuickPart:Cont.I] Container Start = %llu, End = %llu\n", contSpacerNode->PCCN_Start, contSpacerNode->PCCN_End));

        contSpacerNode->PCCN_Delete = FALSE;
        AddTail(&data->qpcd_Part_Children, &contSpacerNode->PCCN_Node);

        data->qpcd_Object_Contents     = Object_Contents;

        data->qpcd_Part_BGColor        = PartCont_BGColor;
        data->qpcd_Part_TAttrlist      = Part_TAttrlist;
        data->qpcd_Part_PAttrlist      = Part_PAttrlist;
        data->qpcd_Part_Reserved       = Part_Reserved;
        data->qpcd_Part_MaxPartitions  = Part_MaxPartitions;
        data->qpcd_Part_Type           = Part_Type;

        D(bug("[QuickPart:Cont.I] Self @ %p\n", self));
    }

    ClosePartitionTable(Part_PHandle);

    return (IPTR)self;
}

static IPTR QPPartitionContainer__OM_SET
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
        case MUIA_HorizWeight:
            data->qpcd_Weight = tag->ti_Data;
            //fall through
        default:
            break;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

static IPTR QPPartitionContainer__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR                          *store = message->opg_Storage;
    IPTR    	      		 retval = TRUE;

    switch(message->opg_AttrID)
    {
    case MUIA_HorizWeight:
        *store = (IPTR)data->qpcd_Weight;
        break;

    case MUIA_QPart_ccPartitionContainer_Type:
        *store = (IPTR)data->qpcd_Part_Type;
        break;

    case MUIA_QPart_ccPartition_Parent:
        *store = (IPTR)data->qpcd_Object_Parent;
        break;

    case MUIA_QPart_ccPartition_Start:
        {
            ULONG SecCylinder = (data->qpcd_Geom->dg_TotalSectors / data->qpcd_Geom->dg_Cylinders), SecSize = data->qpcd_Geom->dg_SectorSize;
            D(bug("[QuickPart:Cont.G] %u sectors per cyl, %u bytes\n", SecCylinder, SecSize));
            *(UQUAD *)store = data->qpcd_Part_DE.de_LowCyl * data->qpcd_Part_DE.de_Surfaces * data->qpcd_Part_DE.de_BlocksPerTrack;
            D(bug("[QuickPart:Cont.G] Start = %llu\n", *(UQUAD *)store));
        }
        break;

    case MUIA_QPart_ccPartition_End:
        {
            ULONG SecCylinder = (data->qpcd_Geom->dg_TotalSectors / data->qpcd_Geom->dg_Cylinders), SecSize = data->qpcd_Geom->dg_SectorSize;
            D(bug("[QuickPart:Cont.G] %u sectors per cyl, %u bytes\n", SecCylinder, SecSize));
            *(UQUAD *)store = (data->qpcd_Part_DE.de_HighCyl + 1) * data->qpcd_Part_DE.de_Surfaces * data->qpcd_Part_DE.de_BlocksPerTrack - 1;
            D(bug("[QuickPart:Cont.G] End = %llu\n", *(UQUAD *)store));
        }
        break;

    default:
        retval = DoSuperMethodA(CLASS, self, (Msg)message);
        break;
    }

    return retval;
}

static IPTR QPPartitionContainer__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    D(bug("[QuickPart:Cont] DISPOSE <data @ %p>\n",data));

    return DoSuperMethodA(CLASS, self, message);
}

static IPTR QPPartitionContainer__MUIM_Setup(Class * CLASS, Object * self, struct MUIP_Setup *message)
{
    SETUP_INST_DATA;
    IPTR retval;

    D(bug("[QuickPart:Cont] %s()\n", __func__));

    retval = DoSuperMethodA(CLASS, self, message);

    if ((data->qpcd_Object_Select) && (data->qpcd_Object_Part))
        DoMethod(data->qpcd_Object_Select, MUIM_Notify, MUIA_Selected, FALSE,
            _app(self), 3, MUIM_Set, MUIA_QPart_ccApp_ActivePart, data->qpcd_Object_Part);

    return retval;
}

static IPTR QPPartitionContainer__MUIM_Cleanup(Class * CLASS, Object * self, struct MUIP_Cleanup *message)
{
    SETUP_INST_DATA;

    D(bug("[QuickPart:Cont] %s()\n", __func__));

    if ((data->qpcd_Object_Select) && (data->qpcd_Object_Part))
        DoMethod(data->qpcd_Object_Select, MUIM_KillNotify, MUIA_Selected);

    return DoSuperMethodA(CLASS, self, message);
}

BOOPSI_DISPATCHER(IPTR, QPPartitionContainer_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPPartitionContainer__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_DISPOSE: 
        return QPPartitionContainer__OM_DISPOSE(CLASS, self, message);
    case OM_SET:
        return QPPartitionContainer__OM_SET(CLASS, self, (struct opSet *) message);
    case OM_GET:
        return QPPartitionContainer__OM_GET(CLASS, self,  (struct opGet *) message);
	case MUIM_Setup:
		return QPPartitionContainer__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);
	case MUIM_Cleanup:
		return QPPartitionContainer__MUIM_Cleanup(CLASS, self, (struct MUIP_Cleanup *)message);
    case MUIM_QPart_ccPartitionContainer_FindParts:
        return QPPartitionContainer__MUIM_QPart_ccPartitionContainer_FindParts(CLASS, self, message);
    case MUIM_QPart_ccPartitionContainer_UpdateWeights:
        return QPPartitionContainer__MUIM_QPart_ccPartitionContainer_UpdateWeights(CLASS, self, message);
    case MUIM_QPart_ccPartitionContainer_UpdateParts:
        return QPPartitionContainer__MUIM_QPart_ccPartitionContainer_UpdateParts(CLASS, self, message);
    case MUIM_QPart_ccPartitionContainer_AddPart:
        return QPPartitionContainer__MUIM_QPart_ccPartitionContainer_AddPart(CLASS, self, (struct MUIP_QPart_ccPartitionContainer_AddPart *)message);
    case MUIM_QPart_ccPartitionContainer_AddFree:
        return QPPartitionContainer__MUIM_QPart_ccPartitionContainer_AddFree(CLASS, self, (struct MUIP_QPart_ccPartitionContainer_AddFree *)message);

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
