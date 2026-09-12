/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include <dos/dos.h>
#include <exec/types.h>
#include <exec/io.h>

#include <clib/alib_protos.h>

#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/colorwheel.h>

#include <libraries/asl.h>
#include <libraries/expansionbase.h>

#include <devices/trackdisk.h>
#include <devices/scsidisk.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>

#include <mui/TextEditor_mcc.h>
#include <zune/iconimage.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_option.h"
#include "ia_driveselect.h"
#include "ia_driveselect_intern.h"
#include "ia_bootloader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DDRIVE(x)

extern struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path);
extern char *FindPartition(struct PartitionHandle *root);

#define DRIVE_MODEL_DISPLAY_MAX 48
#define USB_PROBE_UNITS 32

struct DriveSelect_Record
{
    struct DriveSelect_Record *dsr_Next;
    STRPTR dsr_Device;
    ULONG dsr_Unit;
    STRPTR dsr_Label;
};

static CONST_STRPTR driveselect_empty_entries[] =
{
    "",
    NULL
};

OOP_AttrBase HiddStorageUnitAB;
const struct OOP_ABDescr install__abd[] =
{
    {IID_Hidd_StorageUnit       , &HiddStorageUnitAB    },
    {NULL                       , NULL                  }
};

Object *optObjDestDevice = NULL;
Object *optObjDestUnit = NULL;

CONST_STRPTR def_nvmedev    = "nvme.device";
CONST_STRPTR def_virtiodev  = "virtio.device";
CONST_STRPTR def_usbdev     = "usbscsi.device";
CONST_STRPTR def_imghdisk   = "PROGDIR:IA-Icons/Harddisk";
CONST_STRPTR def_imgusbdisk = "PROGDIR:IA-Icons/USBdisk";

static STRPTR DriveSelect__MakeLabel(CONST_STRPTR device, ULONG unit,
                                      CONST_STRPTR model,
                                      struct PartitionHandle *root)
{
    struct DriveGeometry dg;
    char modelText[DRIVE_MODEL_DISPLAY_MAX + 1] = "";
    char sizeText[32] = "";
    char unitText[16];
    BOOL haveModel = FALSE, haveSize = FALSE;
    size_t i = 0, length;
    STRPTR label;

    if (!device || !device[0])
        return NULL;

    if (model)
    {
        while (model[i] && (i < DRIVE_MODEL_DISPLAY_MAX))
        {
            UBYTE c = (UBYTE)model[i];
            modelText[i++] = ((c < 0x20) || (c == 0x7f)) ? ' ' : c;
        }

        while (i && (modelText[i - 1] == ' '))
            i--;

        modelText[i] = '\0';
        haveModel = i != 0;
    }

    if (root)
    {
        memset(&dg, 0, sizeof(dg));
        GetPartitionAttrsTags(root, PT_GEOMETRY, (IPTR)&dg, TAG_DONE);

        /*
         * ATA, NVMe and VirtIO use 0xffffffff when the actual
         * sector count no longer fits dg_TotalSectors.
         */
        if ((dg.dg_SectorSize != 0) &&
            (dg.dg_TotalSectors != 0) &&
            (dg.dg_TotalSectors != 0xffffffffUL))
        {
            UQUAD bytes = (UQUAD)dg.dg_TotalSectors * dg.dg_SectorSize;
            UQUAD divisor = 1;
            CONST_STRPTR suffix = "B";

            if (bytes >= (1ULL << 40))
            {
                divisor = 1ULL << 40;
                suffix = "TiB";
            }
            else if (bytes >= (1ULL << 30))
            {
                divisor = 1ULL << 30;
                suffix = "GiB";
            }
            else if (bytes >= (1ULL << 20))
            {
                divisor = 1ULL << 20;
                suffix = "MiB";
            }
            else if (bytes >= (1ULL << 10))
            {
                divisor = 1ULL << 10;
                suffix = "KiB";
            }

            if (divisor == 1)
                snprintf(sizeText, sizeof(sizeText), "%llu B",
                         (unsigned long long)bytes);
            else
            {
                UQUAD whole = bytes / divisor;
                UQUAD fraction =
                    (((bytes % divisor) * 100) + divisor / 2) / divisor;

                if (fraction == 100)
                {
                    whole++;
                    fraction = 0;
                }

                snprintf(sizeText, sizeof(sizeText), "%llu.%02llu %s",
                         (unsigned long long)whole,
                         (unsigned long long)fraction, suffix);
            }

            haveSize = TRUE;
        }
    }

    snprintf(unitText, sizeof(unitText), "%lu", (unsigned long)unit);

    length = strlen(device) + strlen(unitText) + 2; /* Flawfinder: ignore */
    if (haveModel)
        length += strlen(modelText) + 2; /* Flawfinder: ignore */
    if (haveSize)
        length += strlen(sizeText) + 2; /* Flawfinder: ignore */

    label = AllocVec((ULONG)length, MEMF_ANY);
    if (!label)
        return NULL;

    if (haveModel && haveSize)
        snprintf(label, length, "%s  %s  %s %s",
                 modelText, sizeText, device, unitText);
    else if (haveModel)
        snprintf(label, length, "%s  %s %s",
                 modelText, device, unitText);
    else if (haveSize)
        snprintf(label, length, "%s  %s %s",
                 sizeText, device, unitText);
    else
        snprintf(label, length, "%s %s", device, unitText);

    return label;
}

static void DriveSelect__FreeDrives(struct DriveSelect_Data *data)
{
    struct DriveSelect_Record *record = data->dsd_Drives;

    if (data->dsd_DriveCycle)
    {
        NNSET(data->dsd_DriveCycle, MUIA_Cycle_Entries,
              (IPTR)driveselect_empty_entries);
        NNSET(data->dsd_DriveCycle, MUIA_Disabled, TRUE);
    }

    if (data->dsd_DriveEntries)
        FreeVec((APTR)data->dsd_DriveEntries);

    while (record)
    {
        struct DriveSelect_Record *next = record->dsr_Next;

        FreeVec(record->dsr_Device);
        FreeVec(record->dsr_Label);
        FreeVec(record);
        record = next;
    }

    data->dsd_Drives = NULL;
    data->dsd_DriveTail = NULL;
    data->dsd_DriveEntries = NULL;
    data->dsd_DriveCount = 0;
    data->dsd_DriveListFailed = FALSE;
}

static BOOL DriveSelect__AddDrive(struct DriveSelect_Data *data,
                                  CONST_STRPTR device, ULONG unit,
                                  CONST_STRPTR model,
                                  struct PartitionHandle *root)
{
    struct DriveSelect_Record *record;
    size_t length;

    if (data->dsd_DriveListFailed)
        return FALSE;

    for (record = data->dsd_Drives; record; record = record->dsr_Next)
    {
        if ((record->dsr_Unit == unit) &&
            (strcmp(record->dsr_Device, device) == 0))
            return TRUE;
    }

    record = AllocVec(sizeof(*record), MEMF_CLEAR);
    if (!record)
        goto failed;

    length = strlen(device) + 1; /* Flawfinder: ignore */
    record->dsr_Device = AllocVec((ULONG)length, MEMF_ANY);
    if (!record->dsr_Device)
    {
        FreeVec(record);
        goto failed;
    }

    memcpy(record->dsr_Device, device, length);
    record->dsr_Unit = unit;
    record->dsr_Label = DriveSelect__MakeLabel(device, unit, model, root);

    if (!record->dsr_Label)
    {
        FreeVec(record->dsr_Device);
        FreeVec(record);
        goto failed;
    }

    if (data->dsd_DriveTail)
        data->dsd_DriveTail->dsr_Next = record;
    else
        data->dsd_Drives = record;

    data->dsd_DriveTail = record;
    data->dsd_DriveCount++;
    return TRUE;

failed:
    data->dsd_DriveListFailed = TRUE;
    return FALSE;
}

static LONG DriveSelect__FindDrive(struct DriveSelect_Data *data,
                                   CONST_STRPTR device, ULONG unit)
{
    struct DriveSelect_Record *record;
    LONG index = 0;

    if (!device)
        return -1;

    for (record = data->dsd_Drives; record;
         record = record->dsr_Next, index++)
    {
        if ((record->dsr_Unit == unit) &&
            (strcmp(record->dsr_Device, device) == 0))
            return index;
    }

    return -1;
}

static struct DriveSelect_Record *
DriveSelect__DriveAt(struct DriveSelect_Data *data, ULONG index)
{
    struct DriveSelect_Record *record = data->dsd_Drives;

    while (record && index--)
        record = record->dsr_Next;

    return record;
}

static BOOL DriveSelect__PublishDrives(struct DriveSelect_Data *data)
{
    struct DriveSelect_Record *record;
    ULONG i = 0;

    if (data->dsd_DriveListFailed)
    {
        DriveSelect__FreeDrives(data);
        return FALSE;
    }

    if (!data->dsd_DriveCount)
        return TRUE;

    data->dsd_DriveEntries =
        AllocVec((data->dsd_DriveCount + 1) *
                 sizeof(*data->dsd_DriveEntries), MEMF_CLEAR);

    if (!data->dsd_DriveEntries)
    {
        DriveSelect__FreeDrives(data);
        return FALSE;
    }

    for (record = data->dsd_Drives; record; record = record->dsr_Next)
        data->dsd_DriveEntries[i++] = record->dsr_Label;

    NNSET(data->dsd_DriveCycle, MUIA_Cycle_Entries,
          (IPTR)data->dsd_DriveEntries);
    NNSET(data->dsd_DriveCycle, MUIA_Disabled, FALSE);
    return TRUE;
}

static void DriveSelect__ProbeUSB(struct DriveSelect_Data *data)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *ioreq;
    ULONG unit;

    if (!port)
        return;

    ioreq = (struct IOStdReq *)CreateIORequest(port, sizeof(*ioreq));
    if (!ioreq)
    {
        DeleteMsgPort(port);
        return;
    }

    for (unit = 0; unit < USB_PROBE_UNITS; unit++)
    {
        if (OpenDevice(def_usbdev, unit, (struct IORequest *)ioreq, 0) == 0)
        {
            struct PartitionHandle *root =
                OpenRootPartition(def_usbdev, unit);

            if (root)
            {
                DriveSelect__AddDrive(data, def_usbdev, unit, NULL, root);
                CloseRootPartition(root);
            }

            CloseDevice((struct IORequest *)ioreq);

            if (data->dsd_DriveListFailed)
                break;
        }
    }

    DeleteIORequest((struct IORequest *)ioreq);
    DeleteMsgPort(port);
}

AROS_UFH3
(
    void, dsSelectHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

    struct DriveSelect_Data *data =
        (struct DriveSelect_Data *)hook->h_Data;
    struct DriveSelect_Record *record =
        param ? DriveSelect__DriveAt(data, (ULONG)param[0]) : NULL;

    if (record)
    {
        struct TagItem tags[] = {
            { MUIA_DriveSelect_Device,    (IPTR)record->dsr_Device },
            { MUIA_DriveSelect_Unit,      (IPTR)record->dsr_Unit },
            { MUIA_DriveSelect_DevProbed, FALSE },
            { TAG_DONE }
        };

        SetAttrsA(obj, tags);
    }

    AROS_USERFUNC_EXIT
}

BOOL isUSBDevice(const char *devStr)
{
    return devStr &&
           (strncmp(devStr, def_usbdev, strlen(def_usbdev)) == 0); /* Flawfinder: ignore */
}

static int checkUSBSysdrive()
{
    char sys_path[100];
    BPTR lock;
    int retval = 0;

    lock = Lock("SYS:", SHARED_LOCK);
    if (lock)
    {
        NameFromLock(lock, sys_path, 100);
        if (getDiskFSSM(USB_SYS_PART_NAME ":") != NULL)
        {
            retval = 1;
            if (strncmp(sys_path, USB_SYS_VOL_NAME ":", strlen(USB_SYS_VOL_NAME) + 1))
            {
                retval = 2;
            }
        }
        UnLock(lock);
    }
    return retval;
}

static IPTR DriveSelect__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
    struct DriveSelect_Global *dsGdata = (struct DriveSelect_Global *)GetTagData(MUIA_DriveSelect_GlobalData, 0, message->ops_AttrList);
    Object *installObj = (Object *)GetTagData(MUIA_DriveSelect_InstallInstance, 0, message->ops_AttrList);
    Object **dsSysObjPtr = (Object **)GetTagData(MUIA_DriveSelect_SysObjPtr,  0, message->ops_AttrList);
    Object **dsWorkObjPtr = (Object **)GetTagData(MUIA_DriveSelect_WorkObjPtr,  0, message->ops_AttrList);
    Object *imgObj, *imgGrpObj, *driveCycle;

    D(bug("[InstallAROS:Drive] %s()\n", __func__));

    if (!installObj || !dsGdata || !dsSysObjPtr || !dsWorkObjPtr)
        return 0;

    OOP_ObtainAttrBases(install__abd);

    optObjDestDevice = Install_MakeOption(installObj, 
                MUIA_InstallOption_ID, (IPTR)"tgtdev",
                MUIA_InstallOption_ValueTag, MUIA_String_Contents,
                MUIA_ShowMe, FALSE,
                MUIA_InstallOption_Obj, (IPTR)(StringObject,
                    MUIA_CycleChain, 1,
                    MUIA_FixWidthTxt , "xxxxxxxxxxxxx",
                    MUIA_String_Reject, " \"\'*",
                    MUIA_Frame, MUIV_Frame_String,
                    MUIA_HorizWeight, 300,
                End),
        TAG_DONE);

    if (!optObjDestDevice)
        return 0;

    optObjDestUnit = Install_MakeOption(installObj, 
                MUIA_InstallOption_ID, (IPTR)"tgtunit",
                MUIA_InstallOption_ValueTag, MUIA_String_Integer,
                MUIA_ShowMe, FALSE,
                MUIA_InstallOption_Obj, (IPTR)(StringObject,
                    MUIA_CycleChain, 1,
                    MUIA_String_Integer, 0,
                    MUIA_FixWidthTxt , "xx",
                    MUIA_String_Accept, "0123456789",
                    MUIA_Frame, MUIV_Frame_String,
                    MUIA_HorizWeight, 20,
                End),
        TAG_DONE);

    if (!optObjDestUnit)
    {
        MUI_DisposeObject(optObjDestDevice);
        return 0;
    }

    self = (Object *) DoSuperNewTags
        (
            CLASS, self, NULL,

            MUIA_Group_Horiz, TRUE,
            Child, (IPTR)(imgGrpObj = HGroup,
                MUIA_Weight, 0,
                Child, (IPTR)(imgObj = IconImageObject,
                    MUIA_IconImage_File, (IPTR) def_imghdisk,
                End),
            End),
            Child, (IPTR)(driveCycle = CycleObject,
                MUIA_CycleChain, 1,
                MUIA_Cycle_Entries, (IPTR)driveselect_empty_entries,
                MUIA_Disabled, TRUE,
                MUIA_HorizWeight, 300,
                MUIA_ShortHelp, __(MSG_HELP_DESTDRIVE),
            End),

            TAG_MORE, (IPTR) message->ops_AttrList   
        );

    if (self)
    {
        struct DriveSelect_Data *data = INST_DATA(CLASS, self);

        D(bug("[InstallAROS:Drive] %s: DriveSelect_Data @ 0x%p\n", __func__, data);)

        data->dsd_Global = dsGdata;

        data->dsd_SysObjPtr = dsSysObjPtr;
        data->dsd_WorkObjPtr = dsWorkObjPtr;

        data->dsd_ImgStr = def_imghdisk;
        data->dsd_ImgGrpObj = imgGrpObj;
        data->dsd_DevImgObj = imgObj;
        data->dsd_DriveCycle = driveCycle;

        data->dsd_SysPartName = SYS_PART_NAME;
        data->dsd_WorkPartName = WORK_PART_NAME;

        data->dsd_SelectHook.h_Entry = (APTR)dsSelectHookFunc;
        data->dsd_SelectHook.h_Data = data;
        DoMethod(driveCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                 self, 3, MUIM_CallHook, &data->dsd_SelectHook,
                 MUIV_TriggerValue);

        DoMethod(self, OM_ADDMEMBER, optObjDestDevice);
        DoMethod(self, OM_ADDMEMBER, optObjDestUnit);

        return (IPTR)self;
    }
    else
    {
        MUI_DisposeObject(optObjDestUnit);
        MUI_DisposeObject(optObjDestDevice);
    }
    return (IPTR)NULL;
}

static IPTR DriveSelect__OM_GET(Class * CLASS, Object * self, struct opGet *message)
{
    struct DriveSelect_Data *data = INST_DATA(CLASS, self);

    DDRIVE(bug("[InstallAROS:Drive] %s()\n", __func__));

    switch(message->opg_AttrID)
    {
         /* Need for notification to work */
        case MUIA_DriveSelect_DevProbed:
            *message->opg_Storage = (IPTR)0;
            return TRUE;
    }
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR DriveSelect__OM_SET(Class * CLASS, Object * self, struct opSet *message)
{
    struct TagItem         *tag, *tags;
    char *devStr = NULL, *unitStr = NULL;
    char unttmp[16];
    BOOL driveChange = FALSE;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
        case MUIA_DriveSelect_DevProbed:
            break;

        case MUIA_DriveSelect_Device:
            devStr = (char *)tag->ti_Data;
            driveChange = TRUE;
            break;

        case MUIA_DriveSelect_Unit:
            unitStr = unttmp;
            sprintf(unttmp, "%lu", (unsigned long)tag->ti_Data);
            driveChange = TRUE;
            break;
        }
    }

    if (driveChange)
    {
        struct DriveSelect_Data *data = INST_DATA(CLASS, self);
        if (devStr)
        {
            CONST_STRPTR imgStr = def_imghdisk;
            if (isUSBDevice(devStr))
                imgStr = def_imgusbdisk;
            if (data->dsd_ImgStr != imgStr)
            {
                Object *oldImgObj = data->dsd_DevImgObj, *imgObj = IconImageObject,
                        MUIA_IconImage_File, (IPTR) imgStr,
                    End;

                if (DoMethod(data->dsd_ImgGrpObj, MUIM_Group_InitChange))
                {
                    DoMethod(data->dsd_ImgGrpObj, OM_ADDMEMBER, imgObj);
                    DoMethod(data->dsd_ImgGrpObj, OM_REMMEMBER, data->dsd_DevImgObj);
                    data->dsd_DevImgObj = imgObj;
                    DoMethod(data->dsd_ImgGrpObj, MUIM_Group_ExitChange);
                }
                MUI_DisposeObject(oldImgObj);
                data->dsd_ImgStr = imgStr;
            }
            if (devStr)
            {
                OPTONNSET(optObjDestDevice, MUIA_String_Contents, (IPTR)devStr);
                DoMethod(optObjDestDevice, MUIM_InstallOption_Update);
            }
            if (isUSBDevice(devStr))
            {
                data->dsd_SysPartName = USB_SYS_PART_NAME;
                data->dsd_WorkPartName = USB_WORK_PART_NAME;
            }
            else if (strncmp(devStr, def_nvmedev, strlen(def_nvmedev)) == 0)
            {
                data->dsd_SysPartName = NVME_SYS_PART_NAME;
                data->dsd_WorkPartName = NVME_WORK_PART_NAME;
            }
            else if (strncmp(devStr, def_virtiodev, strlen(def_virtiodev)) == 0)
            {
                data->dsd_SysPartName = VIRTIO_SYS_PART_NAME;
                data->dsd_WorkPartName = VIRTIO_WORK_PART_NAME;
            }
            else
            {
                data->dsd_SysPartName = SYS_PART_NAME;
                data->dsd_WorkPartName = WORK_PART_NAME;
            }
            if (data->dsd_SysObjPtr)
            {
                SET(*data->dsd_SysObjPtr, MUIA_String_Contents, (IPTR)data->dsd_SysPartName);
            }
            if (data->dsd_WorkObjPtr)
            {
                SET(*data->dsd_WorkObjPtr, MUIA_String_Contents, (IPTR)data->dsd_WorkPartName);
            }
        }
        if (unitStr)
        {
            OPTONNSET(optObjDestUnit, MUIA_String_Contents, (IPTR)unitStr);
            DoMethod(optObjDestUnit, MUIM_InstallOption_Update);
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg)message);
}

struct dsEnumData {
    struct DriveSelect_Data    *dsed_Data;
    IPTR                    *dsed_RetVal;
};

AROS_UFH3S(BOOL, DriveSelect__StorageUnitEnum,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, unitObj, A2),
    AROS_UFHA(struct dsEnumData *, fdData, A1))
{
    AROS_USERFUNC_INIT

    IPTR suType, suDev, suUnit, suModel;

    D(bug("[InstallAROS:Drive] %s(0x%p)\n", __func__, unitObj));

    OOP_GetAttr(unitObj, aHidd_StorageUnit_Type, &suType);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Device, &suDev);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Number, &suUnit);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Model, &suModel);

    if ((suType != vHidd_StorageUnit_Type_OpticalDisc) &&
        (suType != vHidd_StorageUnit_Type_MagneticTape))
    {
        struct PartitionHandle *root;
        D(
            bug("[InstallAROS:Drive] %s: Potential Unit '%s'\n", __func__, suModel);
            bug("[InstallAROS:Drive] %s:         Device %s:%u\n", __func__, suDev, suUnit);
        )
        if ((root = OpenRootPartition((CONST_STRPTR)suDev, suUnit)) != NULL)
        {
            char *result = NULL;

            if (!fdData->dsed_Data->dsd_Global->dsg_BootDev)
            {
                fdData->dsed_Data->dsd_Global->dsg_BootDev = (char *)suDev;
                fdData->dsed_Data->dsd_Global->dsg_BootUnit = suUnit;
                bug("[InstallAROS:Drive] %s: Boot Device %s:%u\n", __func__, suDev, suUnit);
            }

            DriveSelect__AddDrive(fdData->dsed_Data, (CONST_STRPTR)suDev,
                                  suUnit, (CONST_STRPTR)suModel, root);

            D(bug("[InstallAROS:Drive] %s:     Part. Root @ 0x%p\n", __func__, root));

            if ((!*fdData->dsed_RetVal) && (OpenPartitionTable(root) == 0))
            {
                result = FindPartition(root);
                D(bug("[InstallAROS:Drive] %s: FindPartition returned 0x%p\n", __func__, result));
                if (result)
                {
                    D(bug("[InstallAROS:Drive] %s: '%s'\n", __func__, result));
                    *fdData->dsed_RetVal = (IPTR)result;
                }
                ClosePartitionTable(root);
            }
            CloseRootPartition(root);
        }
    }

    /* Continue enumeration: the selector needs every real target. */
    return FALSE;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(BOOL, DriveSelect__StorageBusEnum,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, busObj, A2),
    AROS_UFHA(struct dsEnumData *, fdData, A1))
{
    AROS_USERFUNC_INIT

    struct Hook unitenum_hook =
    {
        .h_Entry = DriveSelect__StorageUnitEnum,
        .h_Data = NULL
    };
    D(bug("[InstallAROS:Drive] %s(0x%p)\n", __func__, busObj));

    HIDD_StorageBus_EnumUnits(busObj, &unitenum_hook, fdData);

    /* Continue enumeration */
    return FALSE;

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(BOOL, DriveSelect__StorageCntrllrEnum,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, ctrllrObj, A2),
    AROS_UFHA(struct dsEnumData *, fdData, A1))
{
    AROS_USERFUNC_INIT

    struct Hook busenum_hook =
    {
        .h_Entry = DriveSelect__StorageBusEnum,
        .h_Data = NULL
    };
    D(bug("[InstallAROS:Drive] %s(0x%p)\n", __func__, ctrllrObj));

    HIDD_StorageController_EnumBuses(ctrllrObj, &busenum_hook, fdData);

    /* Continue enumeration */
    return FALSE;

    AROS_USERFUNC_EXIT
}


static IPTR DriveSelect__MUIM_DriveSelect_FindDrives(Class * CLASS, Object * self, Msg message)
{
    IPTR retval = 0L;

    struct DriveSelect_Data *data = INST_DATA(CLASS, self);
    struct dsEnumData fdData;

    DriveSelect__FreeDrives(data);
    data->dsd_Global->dsg_BootDev = NULL;
    data->dsd_Global->dsg_BootUnit = 0;

    fdData.dsed_Data = data;
    fdData.dsed_RetVal = &retval;

    struct Hook controllerenum_hook =
    {
        .h_Entry = DriveSelect__StorageCntrllrEnum,
        .h_Data = NULL
    };

    D(bug("[InstallAROS:Drive] %s()\n", __func__));

    OOP_Object *storageRoot = OOP_NewObject(NULL, CLID_Hidd_Storage, NULL);
    if (storageRoot)
    {
        HW_EnumDrivers(storageRoot, &controllerenum_hook, &fdData);
    }
    return retval;
}

static IPTR DriveSelect__MUIM_DriveSelect_Initialize(Class * CLASS, Object * self, Msg message)
{
    struct DriveSelect_Data *data = INST_DATA(CLASS, self);
    char *partvol;
    LONG selected = -1;
    BOOL selectedProbed = FALSE;
    int usbState;

    D(bug("[InstallAROS:Drive] %s()\n", __func__));

    /* This part will find two things:
        "boot device" - first device that is not an optional disk, saved in dsg_BootDev and dsg_BootUnit
        "partvol" - first partition that AROS can be installed on
        Note: currently usbscsi.device is not scanned at this step as it is not registering with HW hidd
     */
    if ((partvol = (char *)DoMethod(self, MUIM_DriveSelect_FindDrives)))
    {
        size_t namelen = strlen(partvol); /* Flawfinder: ignore */

        if (namelen < 127)
        {
            char devnamebuffer[128];
            struct FileSysStartupMsg *fssm;

            memcpy(devnamebuffer, partvol, namelen);
            devnamebuffer[namelen] = ':';
            devnamebuffer[namelen + 1] = '\0';

            fssm = getDiskFSSM(devnamebuffer);
            if (fssm)
            {
                selected = DriveSelect__FindDrive(
                    data, AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit);
                selectedProbed = selected >= 0;
            }
        }

        FreeVec(partvol);
    }

    /* usbscsi.device is not registered with HW HIDD. Probe it like HDToolBox. */
    DriveSelect__ProbeUSB(data);

    usbState = checkUSBSysdrive();
    if ((selected < 0) && (usbState == 2))
    {
        struct FileSysStartupMsg *fssm = getDiskFSSM(USB_SYS_PART_NAME ":");

        if (fssm)
        {
            selected = DriveSelect__FindDrive(
                data, AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit);
            selectedProbed = selected >= 0;
        }
    }

    if (!DriveSelect__PublishDrives(data))
        selected = -1;

    if ((selected < 0) && data->dsd_Global->dsg_BootDev)
        selected = DriveSelect__FindDrive(
            data, data->dsd_Global->dsg_BootDev,
            data->dsd_Global->dsg_BootUnit);

    if ((selected < 0) && data->dsd_DriveCount)
        selected = 0;

    if (selected >= 0)
    {
        struct DriveSelect_Record *record =
            DriveSelect__DriveAt(data, (ULONG)selected);

        if (record)
        {
            struct TagItem devTags[] = {
                { MUIA_DriveSelect_Device,    (IPTR)record->dsr_Device },
                { MUIA_DriveSelect_Unit,      (IPTR)record->dsr_Unit },
                { MUIA_DriveSelect_DevProbed, selectedProbed },
                { TAG_DONE }
            };

            NNSET(data->dsd_DriveCycle, MUIA_Cycle_Active, selected);
            SetAttrsA(self, devTags);
        }
    }
    else
    {
        SetAttrs(self,
                 MUIA_DriveSelect_Device, (IPTR)"",
                 MUIA_DriveSelect_Unit, 0,
                 MUIA_DriveSelect_DevProbed, FALSE,
                 TAG_DONE);
    }

    /* Note: fields dsg_BootDev and dsg_BootUnit are not expected to be used beyond this point, use options selected in UI */
    return 0L;
}

static IPTR DriveSelect__OM_DISPOSE(Class * CLASS, Object * self, Msg message)
{
    DriveSelect__FreeDrives(INST_DATA(CLASS, self));
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR DriveSelect__MUIM_Setup(Class * CLASS, Object * self, struct MUIP_Setup *message)
{
    DDRIVE(bug("[InstallAROS:Drive] %s()\n", __func__));

    DoMethod(_win(self), MUIM_Notify, MUIA_Window_Open, TRUE,
                    (IPTR)self, 1, MUIM_DriveSelect_Initialize);

    return DoSuperMethodA(CLASS, self, message);
}

static IPTR DriveSelect__MUIM_Cleanup(Class * CLASS, Object * self, struct MUIP_Cleanup *message)
{
    DDRIVE(bug("[InstallAROS:Drive] %s()\n", __func__));

    DoMethod(_win(self), MUIM_KillNotify, MUIA_Window_Open);

    return DoSuperMethodA(CLASS, self, message);
}

BOOPSI_DISPATCHER(IPTR, DriveSelect__Dispatcher, CLASS, self, message)
{
    DDRIVE(bug("[InstallAROS:Drive] %s(%08x)\n", __func__, message->MethodID));

    /* Handle our methods */
    switch (message->MethodID)
    {
    case OM_NEW:
        return DriveSelect__OM_NEW(CLASS, self, (struct opSet *)message);

    case OM_GET:
        return DriveSelect__OM_GET(CLASS, self, (struct opGet *)message);

    case OM_SET:
        return DriveSelect__OM_SET(CLASS, self, (struct opSet *)message);

    case OM_DISPOSE:
        return DriveSelect__OM_DISPOSE(CLASS, self, message);

    case MUIM_Setup:
        return DriveSelect__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);

    case MUIM_Cleanup:
        return DriveSelect__MUIM_Cleanup(CLASS, self, (struct MUIP_Cleanup *)message);

    case MUIM_DriveSelect_Initialize:
        return DriveSelect__MUIM_DriveSelect_Initialize(CLASS, self, message);

    case MUIM_DriveSelect_FindDrives:
        return DriveSelect__MUIM_DriveSelect_FindDrives(CLASS, self, message);
    }
    return DoSuperMethodA(CLASS, self, message);
}
BOOPSI_DISPATCHER_END
