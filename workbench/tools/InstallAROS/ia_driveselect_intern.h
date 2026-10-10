#ifndef IA_DRIVESELECT_INTERN_H
#define IA_DRIVESELECT_INTERN_H

/* Private composition attribute; not part of ia_driveselect.h. */
#define MUIA_DriveSelect_DeveloperToggle (MUIA_ID_BASE + 0x8)

struct DriveSelect_Global
{
    char                        *dsg_BootDev;
    ULONG                       dsg_BootUnit;
};

struct DriveSelect_Record;

struct DriveSelect_Data
{
    struct Hook                 dsd_SelectHook;
    struct Hook                 dsd_DeveloperHook;
    struct Hook                 dsd_ManualHook;

    Object                      **dsd_SysObjPtr;
    Object                      **dsd_WorkObjPtr;

    CONST_STRPTR                dsd_ImgStr;
    Object                      *dsd_ImgGrpObj;
    Object                      *dsd_DevImgObj;
    Object                      *dsd_DriveCycle;
    Object                      *dsd_ManualDeviceObj;
    Object                      *dsd_ManualUnitObj;
    BOOL                         dsd_DeveloperMode;

    struct DriveSelect_Record   *dsd_Drives;
    struct DriveSelect_Record   *dsd_DriveTail;
    CONST_STRPTR                *dsd_DriveEntries;
    ULONG                        dsd_DriveCount;
    BOOL                         dsd_DriveListFailed;

    char                        *dsd_SysPartName;
    char                        *dsd_WorkPartName;

    struct DriveSelect_Global   *dsd_Global;
};


BOOPSI_DISPATCHER_PROTO(IPTR, DriveSelect__Dispatcher, CLASS, self, message);

#endif /* IA_DRIVESELECT_INTERN_H */
