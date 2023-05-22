/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
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

extern void SetOptObjNotificationFromObj(Object *objSrc, Object *optobjTgt, IPTR trigTag, IPTR trigVal, IPTR setTag, IPTR setVal);
extern struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path);
extern char *FindPartition(struct PartitionHandle *root);

static char *opt_drivetypes[] = {
	"NVME",
	"AHCI/SATA",
	"IDE",
	"SCSI",
	"USB",
	NULL
};

OOP_AttrBase HiddStorageUnitAB;
const struct OOP_ABDescr install__abd[] =
{
    {IID_Hidd_StorageUnit       , &HiddStorageUnitAB    },
    {NULL                       , NULL                  }
};

#define OPT_DTYPE_NVME	0
#define OPT_DTYPE_AHCI	1
#define OPT_DTYPE_IDE	2
#define OPT_DTYPE_SCSI	3
#define OPT_DTYPE_USB	4

Object *cycle_drivetype = NULL;
Object *optObjDestDevice = NULL;
Object *optObjDestUnit = NULL;

CONST_STRPTR	def_atadev	= "ata.device";
CONST_STRPTR	def_ahcidev	= "ahci.device";
CONST_STRPTR	def_nvmedev	= "nvme.device";
CONST_STRPTR	def_scsidev	= "scsi.device";
CONST_STRPTR	def_usbdev	= "usbscsi.device";
CONST_STRPTR strZero = "0";
CONST_STRPTR	def_imghdisk = "PROGDIR:IA-Icons/Harddisk";
CONST_STRPTR	def_imgusbdisk = "PROGDIR:IA-Icons/USBdisk";

AROS_UFH3
(
    void, dsActivateHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

	struct DriveSelect_Data *data = (struct DriveSelect_Data *)hook->h_Data;

	D(bug("[InstallAROS:Drive] %s: DriveSelect_Data @ 0x%p\n", __func__, data);)

	WORD winx, winy;
	WORD winw = _mright(obj) - _mleft(obj), winh = 70;

    winx = _window(obj)->LeftEdge + _mleft(obj);
	winy = _window(obj)->TopEdge + _mbottom(obj);

	if (!data->dsd_PopObj)
	{
		Object *PopContents = VGroup,
					MUIA_Frame, MUIV_Frame_ReadList,
					MUIA_Background, MUII_ButtonBack,
					Child, (IPTR) HVSpace,
					Child, (IPTR) HGroup,
						MUIA_Group_SameWidth, FALSE,
						Child, (IPTR) HVSpace,
						Child, HGroup,
							MUIA_Weight, 40,
							Child, (IPTR) LLabel(_(MSG_TYPE)),
							Child, (IPTR) (cycle_drivetype = CycleObject,
								MUIA_CycleChain, 1,
								MUIA_Cycle_Entries, (IPTR) opt_drivetypes,
								MUIA_Cycle_Active, data->dsd_CycActive,
							End),
						End,
						Child, (IPTR) HVSpace,
					End,
					Child, (IPTR) HGroup,
						MUIA_Group_SameWidth, FALSE,
						Child, (IPTR) HVSpace,
						Child, HGroup,
							Child, (IPTR) LLabel(_(MSG_DEVICE)),
							Child, (IPTR) optObjDestDevice,
						End,
						Child, HGroup,
							MUIA_Weight, 0,
							Child, (IPTR) LLabel(_(MSG_UNIT)),
							Child, (IPTR) optObjDestUnit,
						End,
						Child, (IPTR) HVSpace,
					End,
					Child, (IPTR) HVSpace,
				End;

		data->dsd_PopObj = WindowObject,
				MUIA_Background, MUII_ButtonBack,
				MUIA_Window_Height, winh,
				MUIA_Window_Width, winw,
				MUIA_Window_LeftEdge, winx,
				MUIA_Window_TopEdge, winy,
				MUIA_Window_CloseGadget, FALSE,
				MUIA_Window_DepthGadget, FALSE,
				MUIA_Window_SizeGadget, FALSE,
				MUIA_Window_Borderless, TRUE,
				MUIA_Window_DragBar, FALSE,
				WindowContents, (IPTR)PopContents,
			End;

		if (data->dsd_PopObj)
		{
			D(bug("[InstallAROS:Drive] %s: new window created @ 0x%p\n", __func__, data->dsd_PopObj);)

			DoMethod(_app(obj), OM_ADDMEMBER, (IPTR) data->dsd_PopObj);
			data->dsd_EHNode.ehn_Object = obj;
			data->dsd_EHNode.ehn_Events = IDCMP_INACTIVEWINDOW;

			/* Notifications upon selection of drive type */
			SetOptObjNotificationFromObj(cycle_drivetype, optObjDestDevice,
															MUIA_Cycle_Active, OPT_DTYPE_NVME, MUIA_String_Contents, (IPTR)def_nvmedev);
			SetOptObjNotificationFromObj(cycle_drivetype, optObjDestDevice,
															MUIA_Cycle_Active, OPT_DTYPE_AHCI, MUIA_String_Contents, (IPTR)def_ahcidev);
			SetOptObjNotificationFromObj(cycle_drivetype, optObjDestDevice,
															MUIA_Cycle_Active, OPT_DTYPE_IDE, MUIA_String_Contents, (IPTR)def_atadev);
			SetOptObjNotificationFromObj(cycle_drivetype,  optObjDestDevice,
															MUIA_Cycle_Active, OPT_DTYPE_SCSI, MUIA_String_Contents, (IPTR)def_scsidev);
			SetOptObjNotificationFromObj(cycle_drivetype, optObjDestDevice,
															MUIA_Cycle_Active, OPT_DTYPE_USB, MUIA_String_Contents, (IPTR)def_usbdev);
			SetOptObjNotificationFromObj(cycle_drivetype, optObjDestUnit,
															MUIA_Cycle_Active, MUIV_EveryTime, MUIA_String_Integer, 0);
			DoMethod(cycle_drivetype, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
							data->dsd_UnitTxtObj, 3, MUIM_Set, MUIA_Text_Contents, (IPTR)strZero);
		}
	}
	else
	{
		struct TagItem windowTags[] = {
		        { MUIA_Window_LeftEdge, winx},
		        { MUIA_Window_TopEdge, winy},
				{ MUIA_Window_Width, winw},
		        { TAG_DONE }
			};
		SetAttrsA(data->dsd_PopObj, windowTags);
	}
	if (data->dsd_PopObj)
	{
		D(bug("[InstallAROS:Drive] %s: dsd_PopObj @ 0x%p\n", __func__, data->dsd_PopObj);)
		SET(data->dsd_PopObj, MUIA_Window_Open, TRUE);
		DoMethod(data->dsd_PopObj, MUIM_Window_AddEventHandler, &data->dsd_EHNode);
	}
    AROS_USERFUNC_EXIT
}

static BYTE DriveSelect__InitDriveTypeCycle(struct DriveSelect_Data *data, Object *dtCycle, char *driveDev)
{
    /* default to scsi device ... */
	BYTE retval = -1;
	data->dsd_CycActive = OPT_DTYPE_SCSI;

	D(bug("[InstallAROS:Drive] %s(0x%p, 0x%p, 0x%p)\n", __func__, data, dtCycle, driveDev);)

	if (driveDev)
	{
		/* match the boot device to one of our known "types" ... */
		if (!strncmp(driveDev, def_nvmedev, strlen(def_nvmedev)))
		{
			data->dsd_CycActive = OPT_DTYPE_NVME;
			retval = (BYTE)data->dsd_CycActive;
		}
		else if (!strncmp(driveDev, def_ahcidev, strlen(def_ahcidev)))
		{
			data->dsd_CycActive = OPT_DTYPE_AHCI;
			retval = (BYTE)data->dsd_CycActive;
		}
		else if (!strncmp(driveDev, def_atadev, strlen(def_atadev)))
		{
			data->dsd_CycActive = OPT_DTYPE_IDE;
			retval = (BYTE)data->dsd_CycActive;
		}
		else if (!strncmp(driveDev, def_scsidev, strlen(def_scsidev)))
		{
			data->dsd_CycActive = OPT_DTYPE_SCSI;
			retval = (BYTE)data->dsd_CycActive;
		}
	}
	if (dtCycle)
	{
		SET(dtCycle, MUIA_Cycle_Active, data->dsd_CycActive);
	}

	D(bug("[InstallAROS:Drive] %s: returning %d\n", __func__, retval);)

	return retval;
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
			if (strncmp(sys_path, USB_SYS_VOL_NAME ":",
				strlen(USB_SYS_VOL_NAME) + 1))
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
	Object *imgObj, *txtObjDev, *txtObjUnit;
	Object *imgGrpObj, *txtGrpObj;
	CONST_STRPTR	def_dev;

    D(bug("[InstallAROS:Drive] %s()\n", __func__));

	if (!installObj || !dsGdata || !dsSysObjPtr || !dsWorkObjPtr)
		return 0;

	OOP_ObtainAttrBases(install__abd);

#ifdef __mc68000__
#define DS_DEF_CYCLE	OPT_DTYPE_IDE
	def_dev = def_atadev;
#else
#define DS_DEF_CYCLE	OPT_DTYPE_AHCI
	def_dev = def_ahcidev;
#endif

	optObjDestDevice = Install_MakeOption(installObj, 
				MUIA_InstallOption_ID, (IPTR)"tgtdev",
				MUIA_InstallOption_ValueTag, MUIA_String_Contents,
				MUIA_InstallOption_Obj, (IPTR)(StringObject,
					MUIA_CycleChain, 1,
				    MUIA_FixWidthTxt , "xxxxxxxxxxxxx",
					MUIA_String_Contents, def_dev,
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

			MUIA_Frame,  MUIV_Frame_ImageButton,
            MUIA_Background, MUII_ButtonBack,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_ShortHelp, __(MSG_HELP_DESTDRIVE),
            Child, HGroup,
				Child, HVSpace,
				Child, (IPTR)(imgGrpObj = HGroup,
					Child, (IPTR)(imgObj = IconImageObject,
						MUIA_IconImage_File, (IPTR) def_imghdisk,
					End),
				End),
				Child, (IPTR)(txtGrpObj = HGroup,
					MUIA_FixHeightTxt, "\n\n",
					Child, (IPTR)(txtObjDev = TextObject,
						MUIA_Text_Contents, (IPTR)def_dev,
					End),
					Child, (IPTR)(txtObjUnit = TextObject,
						MUIA_Text_Contents, (IPTR)strZero,
					End),
					Child, HVSpace,
				End),
				Child, HVSpace,
            End,

            TAG_MORE, (IPTR) message->ops_AttrList   
        );

	if (self)
	{
		struct DriveSelect_Data *data = INST_DATA(CLASS, self);

		D(bug("[InstallAROS:Drive] %s: DriveSelect_Data @ 0x%p\n", __func__, data);)

		data->dsd_Global = dsGdata;
		data->dsd_Global->dsg_DefDev = def_dev;

		data->dsd_EHNode.ehn_Class = CLASS;

		data->dsd_SysObjPtr = dsSysObjPtr;
		data->dsd_WorkObjPtr = dsWorkObjPtr;

		data->dsd_ImgGrpObj  = imgGrpObj;
		data->dsd_TxtGrpObj  = txtGrpObj;

		data->dsd_DevImgObj = imgObj;
		data->dsd_DevTxtObj = txtObjDev;
		data->dsd_UnitTxtObj = txtObjUnit;

		data->dsd_SysPartName = SYS_PART_NAME;
		data->dsd_WorkPartName = WORK_PART_NAME;

		data->dsd_ActivateHook.h_Entry = (APTR)dsActivateHookFunc;
		data->dsd_ActivateHook.h_Data = data;
		DoMethod(self, MUIM_Notify, MUIA_Pressed, FALSE,
				 MUIV_Notify_Self, 2, MUIM_CallHook, &data->dsd_ActivateHook);

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
		case MUIA_DriveSelect_DevProbed:
			*message->opg_Storage = (IPTR)0;
            return TRUE;
        case MUIA_DriveSelect_Device:
			GET(data->dsd_DevTxtObj, MUIA_Text_Contents, message->opg_Storage);
            return TRUE;
        case MUIA_DriveSelect_Unit:
			GET(data->dsd_UnitTxtObj, MUIA_Text_Contents, message->opg_Storage);
            return TRUE;
#if (0)
        case MUIA_DriveSelect_SysName:
			*message->opg_Storage = (IPTR)data->dsd_SysPartName;
            return TRUE;
        case MUIA_DriveSelect_WorkName:
			*message->opg_Storage = (IPTR)data->dsd_WorkPartName;
            return TRUE;
#endif
    }
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR DriveSelect__OM_SET(Class * CLASS, Object * self, struct opSet *message)
{
    struct TagItem         *tag, *tags;
	char *devStr = NULL, *unitStr = NULL;
	char unttmp[4];
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
			sprintf(unttmp, "%u", (UBYTE)tag->ti_Data);
			driveChange = TRUE;
            break;
        }
    }

	if (driveChange)
	{
		struct DriveSelect_Data *data = INST_DATA(CLASS, self);
        if (DoMethod(data->dsd_TxtGrpObj, MUIM_Group_InitChange))
        {
			if (devStr)
			{
				SET(data->dsd_DevTxtObj, MUIA_Text_Contents, devStr);
			}
			if (unitStr)
			{
				SET(data->dsd_UnitTxtObj, MUIA_Text_Contents, unitStr);
			}
			DoMethod(data->dsd_TxtGrpObj, MUIM_Group_ExitChange);
		}
		if (devStr)
		{
			CONST_STRPTR imgStr = def_imghdisk;
			if (strncmp(devStr, def_usbdev, strlen(def_usbdev)) == 0)
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
			}
			if (unitStr)
			{
				OPTONNSET(optObjDestUnit, MUIA_String_Contents, (IPTR)unitStr);
			}
			if (strncmp(devStr, def_usbdev, strlen(def_usbdev)) == 0)
			{
				data->dsd_SysPartName = USB_SYS_PART_NAME;
				data->dsd_WorkPartName = USB_WORK_PART_NAME;
			}
			else if (strncmp(devStr, def_nvmedev, strlen(def_nvmedev)) == 0)
			{
				data->dsd_SysPartName = NVME_SYS_PART_NAME;
				data->dsd_WorkPartName = NVME_WORK_PART_NAME;
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
	}

    return DoSuperMethodA(CLASS, self, (Msg)message);
}

static IPTR DriveSelect__MUIM_HandleEvent(Class * CLASS, Object * self, struct MUIP_HandleEvent *message)
{
    struct DriveSelect_Data *data = INST_DATA(CLASS, self);

    DDRIVE(bug("[InstallAROS:Drive] %s()\n", __func__));

	if (data->dsd_PopObj)
	{
		struct TagItem devTags[] = {
			{ MUIA_DriveSelect_Device, 0},
			{ MUIA_DriveSelect_Unit, 0 },
			{ TAG_DONE }
		};
		Object *tmpObj;
		GET(optObjDestDevice, MUIA_InstallOption_Obj, &tmpObj);
		GET(tmpObj, MUIA_String_Contents, &devTags[0].ti_Data);
		GET(optObjDestUnit, MUIA_InstallOption_Obj, &tmpObj);
		GET(tmpObj, MUIA_String_Integer, &devTags[1].ti_Data);
		if ((BOOL)XGET(data->dsd_PopObj, MUIA_Window_Open) == TRUE)
		{
			DoMethod(data->dsd_PopObj, MUIM_Window_RemEventHandler, &data->dsd_EHNode);
			SET(data->dsd_PopObj, MUIA_Window_Open, FALSE);
		}
		SetAttrsA(self, devTags);
	}

	return MUI_EventHandlerRC_Eat;
}

struct dsEnumData {
	struct DriveSelect_Data	*dsed_Data;
	IPTR					*dsed_RetVal;
};

AROS_UFH3S(BOOL, DriveSelect__StorageUnitEnum,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, unitObj, A2),
    AROS_UFHA(struct dsEnumData *, fdData, A1))
{
    AROS_USERFUNC_INIT

    IPTR suType, suDev, suUnit, suModel, suRem;
    BOOL enumstop = FALSE;

    D(bug("[InstallAROS:Drive] %s(0x%p)\n", __func__, unitObj));

    OOP_GetAttr(unitObj, aHidd_StorageUnit_Type, &suType);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Device, &suDev);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Number, &suUnit);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Model, &suModel);
    OOP_GetAttr(unitObj, aHidd_StorageUnit_Removable, &suRem);

    if ((suType != vHidd_StorageUnit_Type_OpticalDisc) &&
        (suType != vHidd_StorageUnit_Type_MagneticTape))
    {
        struct PartitionHandle *root;
        D(
            bug("[InstallAROS:Drive] %s: Potential Unit '%s'\n", __func__, suModel);
            bug("[InstallAROS:Drive] %s:         Device %s:%u\n", __func__, suDev, suUnit);
        )
        if (!fdData->dsed_Data->dsd_Global->dsg_BootDev)
        {
            fdData->dsed_Data->dsd_Global->dsg_BootDev = (char *)suDev;
            fdData->dsed_Data->dsd_Global->dsg_BootUnit = suUnit;
            bug("[InstallAROS:Drive] %s: Boot Device %s:%u\n", __func__, suDev, suUnit);
        }
        if ((root = OpenRootPartition((CONST_STRPTR)suDev, suUnit)) != NULL)
        {
            char *result = NULL;

            D(bug("[InstallAROS:Drive] %s:     Part. Root @ 0x%p\n", __func__, root));

            if (OpenPartitionTable(root) == 0)
            {
                result = FindPartition(root);
                D(bug("[InstallAROS:Drive] %s: FindPartition returned 0x%p\n", __func__, result));
                if (result)
                {
                    D(bug("[InstallAROS:Drive] %s: '%s'\n", __func__, result));
                    *fdData->dsed_RetVal = (IPTR)result;
                    enumstop = TRUE;
                }
                ClosePartitionTable(root);
            }
            CloseRootPartition(root);
        }
    }

    /* Continue enumeration */
    return enumstop;

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

	struct dsEnumData fdData;
	fdData.dsed_Data = INST_DATA(CLASS, self);
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
    IPTR retval = 0L;
	BOOL devset = FALSE;

    D(bug("[InstallAROS:Drive] %s()\n", __func__));

	if ((partvol = (char *)DoMethod(self, MUIM_DriveSelect_FindDrives)))
	{
		BYTE namelen = (BYTE)strlen(partvol);

		if (namelen > 1)
		{
			struct TagItem devTags[] = {
				{ MUIA_DriveSelect_Device, 		0 		},
				{ MUIA_DriveSelect_Unit, 		0		},
				{ MUIA_DriveSelect_DevProbed, 	TRUE	},
				{ TAG_DONE }
			};
			char devnamebuffer[128];
			struct FileSysStartupMsg *fssm;

			D(bug("[InstallAROS:Drive] %s: Suitable Existing Device '%s' (%u)\n", __func__, partvol, namelen));

			devnamebuffer[namelen--] = ':';
			while (namelen >= 0)
			{
				devnamebuffer[namelen] = partvol[namelen];
				namelen--;
			}
			D(bug("[InstallAROS:Drive] %s: Checking '%s'\n", __func__, devnamebuffer));

			fssm = getDiskFSSM(devnamebuffer);
			if (fssm != NULL)
			{
				D(
					bug("[InstallAROS:Drive] %s: FSSM @ 0x%p\n", __func__, fssm);
					bug("[InstallAROS:Drive] %s:      %s:%u\n", __func__, AROS_BSTR_ADDR(fssm->fssm_Device), AROS_BSTR_ADDR(fssm->fssm_Unit));
				)

				/* use the partitions device ... */
				DriveSelect__InitDriveTypeCycle(data, cycle_drivetype, AROS_BSTR_ADDR(fssm->fssm_Device));
				devTags[0].ti_Data = (IPTR)AROS_BSTR_ADDR(fssm->fssm_Device);
				devTags[1].ti_Data = fssm->fssm_Unit;
				devset = TRUE;
			}
			else
			{
				devTags[0].ti_Tag = TAG_IGNORE;
				devTags[1].ti_Tag = TAG_IGNORE;
			}
			SetAttrsA(self, devTags);
		}
	}

	if (!devset)
	{
		if (data->dsd_Global->dsg_BootDev)
		{
			struct TagItem devTags[] = {
				{ MUIA_DriveSelect_Device, 0},
				{ MUIA_DriveSelect_Unit, 0 },
				{ TAG_DONE }
			};

			D(bug("[InstallAROS:Drive] %s: Using boot device '%s'\n", __func__, data->dsd_Global->dsg_BootDev));

			/* use boot device ... */
			DriveSelect__InitDriveTypeCycle(data, cycle_drivetype, data->dsd_Global->dsg_BootDev);
			devTags[0].ti_Data = (IPTR)data->dsd_Global->dsg_BootDev;
			devTags[1].ti_Data = data->dsd_Global->dsg_BootUnit;
			SetAttrsA(self, devTags);
		}
		else
		{
			/* couldnt detect anything so just show the default options ... */
			data->dsd_CycActive = DS_DEF_CYCLE;
		}
	}

	/* Default to USB if a USB system volume appears to be present and we
	 * haven't booted from it */
	switch (checkUSBSysdrive())
	{
		case 2:
			data->dsd_Global->dsg_DefDev = def_usbdev;
			data->dsd_CycActive = OPT_DTYPE_USB;
			break;
		default:
			break;
	}
    return retval;
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

	case MUIM_Setup:
		return DriveSelect__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);

	case MUIM_Cleanup:
		return DriveSelect__MUIM_Cleanup(CLASS, self, (struct MUIP_Cleanup *)message);

    case MUIM_HandleEvent:
        return DriveSelect__MUIM_HandleEvent(CLASS, self, (struct MUIP_HandleEvent *)message);

	case MUIM_DriveSelect_Initialize:
        return DriveSelect__MUIM_DriveSelect_Initialize(CLASS, self, message);

    case MUIM_DriveSelect_FindDrives:
        return DriveSelect__MUIM_DriveSelect_FindDrives(CLASS, self, message);
    }
    return DoSuperMethodA(CLASS, self, message);
}
BOOPSI_DISPATCHER_END
