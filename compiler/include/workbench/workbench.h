#ifndef WORKBENCH_WORKBENCH_H
#define WORKBENCH_WORKBENCH_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

/*** Workbench library name *************************************************/
#define WORKBENCH_NAME           "workbench.library"

/*** Structures and associated definitions **********************************/
struct OldDrawerData
{
    struct NewWindow    dd_NewWindow;
    LONG                dd_CurrentX;
    LONG                dd_CurrentY;
};

#define OLDDRAWERDATAFILESIZE (sizeof(struct OldDrawerData))

struct DrawerData
{
    struct NewWindow dd_NewWindow;
    LONG             dd_CurrentX;
    LONG             dd_CurrentY;
    ULONG            dd_Flags;
    UWORD            dd_ViewModes;
};

#define DRAWERDATAFILESIZE (sizeof(struct DrawerData))

/* Definitions for dd_ViewModes */
#define DDVM_BYDEFAULT      0 /* Default (inherit parent's view mode) */
#define DDVM_BYICON         1 /* View as icons */
#define DDVM_BYNAME         2 /* View as text, sorted by name */
#define DDVM_BYDATE         3 /* View as text, sorted by date */
#define DDVM_BYSIZE         4 /* View as text, sorted by size */
#define DDVM_BYTYPE         5 /* View as text, sorted by type */

/* Definitions for dd_Flags */
#define DDFLAGS_SHOWDEFAULT 0 /* Default (show only icons) */
#define DDFLAGS_SHOWICONS   1 /* Show only icons */
#define DDFLAGS_SHOWALL     2 /* Show all files */

struct DiskObject
{
    UWORD              do_Magic;
    UWORD              do_Version;
    struct Gadget      do_Gadget;
    UBYTE              do_Type;        /* see below */
    STRPTR             do_DefaultTool;
    STRPTR            *do_ToolTypes;
    LONG               do_CurrentX;
    LONG               do_CurrentY;
    struct DrawerData *do_DrawerData;
    STRPTR             do_ToolWindow;
    LONG               do_StackSize;
};

#define WBDISK    (1)
#define WBDRAWER  (2)
#define WBTOOL    (3)
#define WBPROJECT (4)
#define WBGARBAGE (5)
#define WBDEVICE  (6)
#define WBKICK    (7)
#define WBAPPICON (8)

#define WB_DISKVERSION  (1)
#define WB_DISKREVISION (1)
#define WB_DISKREVISIONMASK (0xFF)

#define WB_DISKMAGIC (0xE310)

struct FreeList
{
    WORD        fl_NumFree;
    struct List fl_MemList;
};

/* Icons */
#define GFLG_GADGBACKFILL 0x0001
#define NO_ICON_POSITION  0x80000000

struct AppMessage
{
    struct Message am_Message;
    UWORD          am_Type;     /* see below */
    IPTR           am_UserData;
    IPTR           am_ID;
    LONG           am_NumArgs;
    struct WBArg * am_ArgList;
    UWORD          am_Version;  /* see below */
    UWORD          am_Class;
    WORD           am_MouseX;
    WORD           am_MouseY;
    ULONG          am_Seconds;
    ULONG          am_Micros;
    ULONG          am_Reserved[8];
};

/* Definition for am_Version */
#define AM_VERSION              1

/* Definitions for am_Type */
#define AMTYPE_APPWINDOW        7
#define AMTYPE_APPICON          8
#define AMTYPE_APPMENUITEM      9
#define AMTYPE_APPWINDOWZONE    10

/* Definitions for am_Class */
#define AMCLASSICON_Open        0
#define AMCLASSICON_Copy        1
#define AMCLASSICON_Rename      2
#define AMCLASSICON_Information 3
#define AMCLASSICON_Snapshot    4
#define AMCLASSICON_UnSnapshot  5
#define AMCLASSICON_LeaveOut    6
#define AMCLASSICON_PutAway     7
#define AMCLASSICON_Delete      8
#define AMCLASSICON_FormatDisk  9
#define AMCLASSICON_EmptyTrash 10
#define AMCLASSICON_Selected   11
#define AMCLASSICON_Unselected 12

struct AppIconRenderMsg
{
    struct RastPort   *arm_RastPort;
    struct DiskObject *arm_Icon;
    STRPTR             arm_Label;
    struct TagItem    *arm_Tags;
    WORD               arm_Left;
    WORD               arm_Top;
    WORD               arm_Width;
    WORD               arm_Height;
    ULONG              arm_State;
};

struct AppWindowDropZoneMsg
{
    struct RastPort *adzm_RastPort;
    struct IBox      adzm_DropZoneBox;
    IPTR             adzm_ID;
    IPTR             adzm_UserData;
    LONG             adzm_Action;  /* see below */
};

/* Definitions for adzm_Action */
#define ADZMACTION_Enter (0)
#define ADZMACTION_Leave (1)

struct IconSelectMsg
{
    ULONG           ism_Length;
    BPTR            ism_Drawer;
    STRPTR          ism_Name;
    UWORD           ism_Type;
    BOOL            ism_Selected;
    struct TagItem *ism_Tags;
    struct Window  *ism_DrawerWindow;
    struct Window  *ism_ParentWindow;
    WORD            ism_Left;
    WORD            ism_Top;
    WORD            ism_Width;
    WORD            ism_Height;
};

/* Hook return values */
#define ISMACTION_Unselect (0)
#define ISMACTION_Select   (1)
#define ISMACTION_Ignore   (2)
#define ISMACTION_Stop     (3)

/****************************************************************************/

struct CopyBeginMsg
{
    ULONG cbm_Length;
    LONG  cbm_Action;
    BPTR  cbm_SourceDrawer;
    BPTR  cbm_DestinationDrawer;
};

struct CopyDataMsg
{
    ULONG  cdm_Length;
    LONG   cdm_Action;
    BPTR   cdm_SourceLock;
    STRPTR cdm_SourceName;
    BPTR   cdm_DestinationLock;
    STRPTR cdm_DestinationName;
    LONG   cdm_DestinationX;
    LONG   cdm_DestinationY;
};

struct CopyEndMsg
{
    ULONG cem_Length;
    LONG  cem_Action;
};

#define CPACTION_Begin (0)
#define CPACTION_Copy  (1)
#define CPACTION_End   (2)

/****************************************************************************/

struct DeleteBeginMsg
{
    ULONG       dbm_Length;
    LONG        dbm_Action;
};

struct DeleteDataMsg
{
    ULONG       ddm_Length;
    LONG        ddm_Action;
    BPTR        ddm_Lock;
    STRPTR      ddm_Name;
};

struct DeleteEndMsg
{
    ULONG       dem_Length;
    LONG        dem_Action;
};

#define DLACTION_BeginDiscard           (0)
#define DLACTION_BeginEmptyTrash        (1)
#define DLACTION_DeleteContents         (3)
#define DLACTION_DeleteObject           (4)
#define DLACTION_End                    (5)

/****************************************************************************/

struct SetupCleanupHookMsg
{
    ULONG       schm_Length;
    LONG        schm_State;
};

#define SCHMSTATE_TryCleanup    (0)
#define SCHMSTATE_Cleanup       (1)
#define SCHMSTATE_Setup         (2)

/****************************************************************************/

struct TextInputMsg
{
    ULONG       tim_Length;
    LONG        tim_Action;
    STRPTR      tim_Prompt;
};

#define TIACTION_Rename         (0)
#define TIACTION_RelabelVolume  (1)
#define TIACTION_NewDrawer      (2)
#define TIACTION_Execute        (3)

/*** Private structures *****************************************************/
struct AppWindow;
struct AppWindowDropZone;
struct AppIcon;
struct AppMenuItem;

/*** Start of workbench.library tags ****************************************/
#define WBA_BASE                          (TAG_USER+0xA000)

/*** Tags for use with AddAppIconA() ****************************************/
/* The different menu items the AppIcon responds to (BOOL) */
#define WBAPPICONA_SupportsOpen            (WBA_BASE+1)
#define WBAPPICONA_SupportsCopy            (WBA_BASE+2)
#define WBAPPICONA_SupportsRename          (WBA_BASE+3)
#define WBAPPICONA_SupportsInformation     (WBA_BASE+4)
#define WBAPPICONA_SupportsSnapshot        (WBA_BASE+5)
#define WBAPPICONA_SupportsUnSnapshot      (WBA_BASE+6)
#define WBAPPICONA_SupportsLeaveOut        (WBA_BASE+7)
#define WBAPPICONA_SupportsPutAway         (WBA_BASE+8)
#define WBAPPICONA_SupportsDelete          (WBA_BASE+9)
#define WBAPPICONA_SupportsFormatDisk      (WBA_BASE+10)
#define WBAPPICONA_SupportsEmptyTrash      (WBA_BASE+11)

/* Propagate the AppIcons position back to original DiskObject (BOOL) */
#define WBAPPICONA_PropagatePosition       (WBA_BASE+12)

/* Call this hook when rendering this AppIcon (struct Hook *) */
#define WBAPPICONA_RenderHook              (WBA_BASE+13)

/* Notify the AppIcon when it's select state changes (BOOL) */
#define WBAPPICONA_NotifySelectState        (WBA_BASE+14)

/*** Tags for use with AddAppMenuItemA() ************************************/
#define WBAPPMENUA_CommandKeyString         (WBA_BASE+15)
#define	WBAPPMENUA_GetKey                   (WBA_BASE+65)
#define	WBAPPMENUA_UseKey                   (WBA_BASE+66)
#define	WBAPPMENUA_GetTitleKey              (WBA_BASE+77)

/*** Tags for use with OpenWorkbenchObjectA() *******************************/
#define WBOPENA_ArgLock                     (WBA_BASE+16)
#define WBOPENA_ArgName                     (WBA_BASE+17)
#define WBOPENA_Show                        (WBA_BASE+75)
#define WBOPENA_ViewBy                      (WBA_BASE+76)

/*** Tags for use with WorkbenchControlA() **********************************/
#define WBCTRLA_IsOpen                      (WBA_BASE+18)
#define WBCTRLA_DuplicateSearchPath         (WBA_BASE+19)
#define WBCTRLA_FreeSearchPath              (WBA_BASE+20)
#define WBCTRLA_GetDefaultStackSize         (WBA_BASE+21)
#define WBCTRLA_SetDefaultStackSize         (WBA_BASE+22)
#define WBCTRLA_RedrawAppIcon               (WBA_BASE+23)
#define WBCTRLA_GetProgramList              (WBA_BASE+24)
#define WBCTRLA_FreeProgramList             (WBA_BASE+25)
#define WBCTRLA_GetSelectedIconList         (WBA_BASE+36)
#define WBCTRLA_FreeSelectedIconList        (WBA_BASE+37)
#define WBCTRLA_GetOpenDrawerList           (WBA_BASE+38)
#define WBCTRLA_FreeOpenDrawerList          (WBA_BASE+39)
#define WBCTRLA_GetHiddenDeviceList         (WBA_BASE+42)
#define WBCTRLA_FreeHiddenDeviceList        (WBA_BASE+43)
#define WBCTRLA_AddHiddenDeviceName         (WBA_BASE+44)
#define WBCTRLA_RemoveHiddenDeviceName      (WBA_BASE+45)
#define WBCTRLA_GetTypeRestartTime          (WBA_BASE+47)
#define WBCTRLA_SetTypeRestartTime          (WBA_BASE+48)
#define WBCTRLA_GetCopyHook                 (WBA_BASE+69)
#define WBCTRLA_SetCopyHook                 (WBA_BASE+70)
#define WBCTRLA_GetDeleteHook               (WBA_BASE+71)
#define WBCTRLA_SetDeleteHook               (WBA_BASE+72)
#define WBCTRLA_GetTextInputHook            (WBA_BASE+73)
#define WBCTRLA_SetTextInputHook            (WBA_BASE+74)
#define WBCTRLA_AddSetupCleanupHook         (WBA_BASE+78)
#define WBCTRLA_RemSetupCleanupHook         (WBA_BASE+79)

/*** Tags for use with AddAppWindowDropZoneA() ******************************/
#define WBDZA_Left                          (WBA_BASE+26)
#define WBDZA_RelRight                      (WBA_BASE+27)
#define WBDZA_Top                           (WBA_BASE+28)
#define WBDZA_RelBottom                     (WBA_BASE+29)
#define WBDZA_Width                         (WBA_BASE+30)
#define WBDZA_RelWidth                      (WBA_BASE+31)
#define WBDZA_Height                        (WBA_BASE+32)
#define WBDZA_RelHeight                     (WBA_BASE+33)
#define WBDZA_Box                           (WBA_BASE+34)
#define WBDZA_Hook                          (WBA_BASE+35)

/*** Reserved tags **********************************************************/
#define WBA_Reserved1                       (WBA_BASE+40)
#define WBA_Reserved2                       (WBA_BASE+41)
#define WBA_Reserved3                       (WBA_BASE+46)
#define WBA_Reserved4                       (WBA_BASE+49)
#define WBA_Reserved5                       (WBA_BASE+50)
#define WBA_Reserved6                       (WBA_BASE+51)
#define WBA_Reserved7                       (WBA_BASE+52)
#define WBA_Reserved8                       (WBA_BASE+53)
#define WBA_Reserved9                       (WBA_BASE+54)
#define WBA_Reserved10                      (WBA_BASE+55)
#define WBA_Reserved11                      (WBA_BASE+56)
#define WBA_Reserved12                      (WBA_BASE+57)
#define WBA_Reserved13                      (WBA_BASE+58)
#define WBA_Reserved14                      (WBA_BASE+59)
#define WBA_Reserved15                      (WBA_BASE+60)
#define WBA_Reserved16                      (WBA_BASE+61)
#define WBA_Reserved17                      (WBA_BASE+62)
#define WBA_Reserved18                      (WBA_BASE+63)
#define WBA_Reserved19                      (WBA_BASE+64)

/*** Last tag ***************************************************************/

#define WBA_LAST_TAG                        (WBA_BASE+64)

/* Parameters for the UpdateWorkbench() function */
#define UPDATEWB_ObjectRemoved              0
#define UPDATEWB_ObjectAdded                1

#endif /* WORKBENCH_WORKBENCH_H */
