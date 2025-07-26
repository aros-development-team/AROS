/*
    Copyright © 2014-2025, The AROS Development Team. All rights reserved.
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
#include <hidd/hidd.h>

#include   <zune/systemprefswindow.h>
#include   <zune/prefseditor.h>
#include   <zune/aboutwindow.h>

#include   <stdlib.h>
#include   <stdio.h>
#include   <strings.h>

#include <aros/locale.h>

#define _QP_MAIN_C

#include "QP_Intern.h"

#include "QP_ccDisk.h"
#include "QP_ccPartition.h"
#include "QP_ccPartitionContainer.h"

extern OOP_AttrBase HiddStorageUnitAB;

extern struct MUI_CustomClass *mcc_qpdisk;
extern Object *AQPart_grp_Drives;
extern Object *AQPart_VOID;
extern Object *AQPart_gge_Progress;
extern Object *AQPart_chk_Scale;

struct   Hook   Scalehook;

struct qpEnumData
{
    struct List *qped_Drives;
    UQUAD       qped_DriveLargest;
};

struct qpFoundDrive
{
    struct Node qpfd_Node;
    char            *qpfd_Dev;
    char            *qpfd_Model;
    UQUAD           qpfd_Size;
    ULONG           qpfd_Unit;
};

AROS_UFH3S(BOOL, unitenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct qpEnumData *qpED = (struct qpEnumData *)h->h_Data;
    IPTR val;

    D(bug("[QuickPart] %s()\n", __func__));

    OOP_GetAttr(obj, aHidd_StorageUnit_Type, &val);
    D(bug("[QuickPart] %s: UnitType = %u\n", __func__, val));
    if ((val == vHidd_StorageUnit_Type_FixedDisk) ||
         (val == vHidd_StorageUnit_Type_SolidStateDisk) ||
         (val == vHidd_StorageUnit_Type_RaidArray))
    {
        D(bug("[QuickPart] %s: Disk Unit Object @ %p\n", __func__, obj));

        struct qpFoundDrive *newDrive = AllocVec(sizeof(struct qpFoundDrive), MEMF_CLEAR);
        if (newDrive) {
            struct PartitionHandle       *diskPH;

            OOP_GetAttr(obj, aHidd_StorageUnit_Device, (IPTR *)&newDrive->qpfd_Dev);
            OOP_GetAttr(obj, aHidd_StorageUnit_Model, (IPTR *)&newDrive->qpfd_Model);
            OOP_GetAttr(obj, aHidd_StorageUnit_Number, &val);
            newDrive->qpfd_Unit = (ULONG)val;

            D(bug("[QuickPart] %s: Disk device = %s:%u\n", __func__, newDrive->qpfd_Dev, newDrive->qpfd_Unit));
            diskPH = OpenRootPartition(newDrive->qpfd_Dev, newDrive->qpfd_Unit);
            if (diskPH)
            {
                struct DriveGeometry diskGeom;
                GetPartitionAttrsTags
                (
                    diskPH,
                    PT_GEOMETRY, (IPTR) &diskGeom,
                    TAG_DONE
                );
                newDrive->qpfd_Size = ((UQUAD)diskGeom.dg_SectorSize * diskGeom.dg_TotalSectors);
                if (newDrive->qpfd_Size > qpED->qped_DriveLargest)
                    qpED->qped_DriveLargest = newDrive->qpfd_Size;
                CloseRootPartition(diskPH);
            }
    
            AddTail(qpED->qped_Drives, &newDrive->qpfd_Node);
        }
    }
    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(BOOL, busenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct Hook unitenum_hook =
    {
        .h_Entry = unitenumFunc,
        .h_Data = h->h_Data
    };
    D(bug("[QuickPart] %s()\n", __func__));

    HIDD_StorageBus_EnumUnits(obj, &unitenum_hook, parent);

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(BOOL, controllerenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct Hook busenum_hook =
    {
        .h_Entry = busenumFunc,
        .h_Data = h->h_Data
    };
    D(bug("[QuickPart] %s()\n", __func__));

    HIDD_StorageController_EnumBuses(obj, &busenum_hook, parent);

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, driveScaleHookFunc,
    AROS_UFHA(struct Hook *, this_hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(char *, c, A1))
{
    AROS_USERFUNC_INIT

    IPTR    scaleVal;
    IPTR    scaleDrive;

    get(AQPart_chk_Scale, MUIA_Selected, &scaleDrive);
    get(obj, MUIA_UserData, &scaleVal);

    if (scaleDrive)
        set(obj, MUIA_QPart_Disk_Scale, scaleVal);
    else
        set(obj, MUIA_QPart_Disk_Scale, 100);

    AROS_USERFUNC_EXIT
}

int FindDisks()
{
    struct List FoundDrives;
    struct qpEnumData qpED =
    {
        .qped_Drives = &FoundDrives,
        .qped_DriveLargest = 0
    };
    struct Hook controllerenum_hook =
    {
        .h_Entry = controllerenumFunc,
        .h_Data = &qpED
    };
    struct qpFoundDrive *newDrive, *tmpNode;

    D(bug("[QuickPart] %s()\n", __func__));

    NEWLIST(&FoundDrives);

    OOP_Object *storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (storageRoot)
    {
        HW_EnumDrivers(storageRoot, &controllerenum_hook, NULL);
    }

    int diskCount, diskCur = 0;
    ListLength(&FoundDrives, diskCount);
    ForeachNodeSafe(&FoundDrives, newDrive, tmpNode)
    {
        Object *diskObj = NewObject(mcc_qpdisk->mcc_Class, NULL,
                    (newDrive->qpfd_Model) ? MUIA_QPart_Disk_Name : TAG_IGNORE,
                        (IPTR)newDrive->qpfd_Model,
                    MUIA_QPart_Disk_Handler, (IPTR)newDrive->qpfd_Dev,
                    MUIA_QPart_Disk_Unit, newDrive->qpfd_Unit,
                TAG_DONE);

        D(bug("[QuickPart] %s: Created New DiskObj @ %p\n", __func__, diskObj));

        Object *qpt_VOID_spacer2 = HGroup,
                MUIA_Weight, 5,
                Child, HVSpace,
            End;

        Remove(&newDrive->qpfd_Node);
        set(AQPart_gge_Progress, MUIA_Gauge_Current, (diskCur * 100) / diskCount);
        set(diskObj, MUIA_UserData, (ULONG)((newDrive->qpfd_Size * 100) /qpED.qped_DriveLargest));
//        set(diskObj, MUIA_QPart_Disk_Scale, (ULONG)((newDrive->qpfd_Size * 100) /qpED.qped_DriveLargest));

        Scalehook.h_Entry = (APTR)driveScaleHookFunc;
        DoMethod(AQPart_chk_Scale, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, diskObj, 3, MUIM_CallHook, &Scalehook, MUIV_TriggerValue);

        if (diskObj && qpt_VOID_spacer2)
        {
            if (DoMethod(AQPart_grp_Drives, MUIM_Group_InitChange))
            {
                if (diskCur==0)
                    DoMethod(AQPart_grp_Drives, OM_REMMEMBER, AQPart_VOID);

                DoMethod(AQPart_grp_Drives, OM_ADDMEMBER, diskObj);

                DoMethod(AQPart_grp_Drives, OM_ADDMEMBER, qpt_VOID_spacer2);

                DoMethod(AQPart_grp_Drives, MUIM_Group_ExitChange);
            }

            D(bug("[QuickPart] %s: DiskObj inserted in display - causing scan for partitions ..\n", __func__));

            DoMethod(diskObj, MUIM_QPart_Disk_ScanForParts);
        }
        FreeVec(newDrive);
        diskCur++;
    }
    return (int)diskCount;
}
