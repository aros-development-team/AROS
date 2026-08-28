#ifndef BTHID_H
#define BTHID_H

/* bthid.class - see bthid.class.c. Structures and protos of the HID report
   parser, action engine and configuration GUI (from Poseidon's hid.class). */

#define BTHID_MAXCHANNELS   8   /* input report channels per binding */
#define BTHID_MAXREPORTEPS 16   /* Report characteristics remembered (HOGP) */
#define BTHID_MAXREPORT   256   /* largest report handled (bytes) */
#define BTHID_MAXREPORTMAP 2048 /* largest report map read */


#include LC_LIBDEFS_FILE

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>

#include <devices/timer.h>
#include <devices/input.h>
#include <utility/utility.h>
#include <dos/dos.h>
#include <intuition/intuition.h>

#include <libraries/bluetooth.h>
#include <libraries/btclass.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <proto/bluetooth.h>
#include <proto/utility.h>
#include <proto/keymap.h>
#include <proto/layers.h>
#include <proto/input.h>
#include <proto/expansion.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#define NewList NEWLIST

#include <stdarg.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))



#include <devices/keyboard.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_EnableKBReset;
    ULONG cdc_EnableRH;
    ULONG cdc_ResetDelay;
    ULONG cdc_ShellStack;
    char  cdc_ShellCon[128];
    ULONG cdc_PollingMode;
    ULONG cdc_LLPortMode[4];
    ULONG cdc_HIDCtrlOpen;
    char  cdc_HIDCtrlRexx[32];
    char  cdc_HIDCtrlTitle[32];
    ULONG cdc_LLRumblePort;
    ULONG cdc_TurboMouse;
};

struct KeymapCfg
{
    ULONG kmc_ChunkID;
    ULONG kmc_Length;
    UBYTE kmc_Keymap[256];
};

struct BtHidActionChunk
{
    ULONG       nhac_ID;
    ULONG       nhac_Length;
    UWORD       nhac_Type;
    UBYTE       nhac_QualMode;
    UBYTE       nhac_Qualifier;
    UBYTE       nhac_MouseAxis;
    UBYTE       nhac_WheelMode;
    UBYTE       nhac_WheelDist;
    UBYTE       nhac_ButtonMode;
    UBYTE       nhac_ButtonNo;
    UBYTE       nhac_RawKey;
    UBYTE       nhac_TabletAxis;
    UBYTE       nhac_SoundVolume;
    UBYTE       nhac_ShellAsync;
    UBYTE       nhac_MiscMode;
    UBYTE       nhac_AbsToRel;
    UBYTE       nhac_ScaleEnable;
    UBYTE       nhac_ClipEnable;
    UBYTE       nhac_CCEnable;
    UBYTE       nhac_ClipMin;
    UBYTE       nhac_ClipMax;
    UBYTE       nhac_ClipStretch;
    UBYTE       nhac_CCVar1;
    UBYTE       nhac_CCCond;
    UBYTE       nhac_CCVar2;
    UBYTE       nhac_ValEnable;
    UBYTE       nhac_ValVar;
    LONG        nhac_ScaleMin;
    LONG        nhac_ScaleMax;
    LONG        nhac_CCConst1;
    LONG        nhac_CCConst2;
    LONG        nhac_ValConst;
    UBYTE       nhac_JoypadOp;
    UBYTE       nhac_JoypadFeat;
    UBYTE       nhac_JoypadPort;
    UBYTE       nhac_TarVar;
    UBYTE       nhac_TarVarOp;
    UBYTE       nhac_OutOp;
    UWORD       nhac_OutItem;
    UWORD       nhac_FeatItem;
    UBYTE       nhac_FeatOp;
    UBYTE       nhac_APadFeat;
};

struct BtHidAction
{
    struct Node nha_Node;
    UWORD       nha_Type;
    /* HUA_QUALIFIER */
    UBYTE       nha_QualMode;
    UBYTE       nha_Qualifier;
    /* HUA_MOUSEPOS */
    UBYTE       nha_MouseAxis;
    UBYTE       nha_WheelMode;
    UBYTE       nha_WheelDist;
    /* HUA_BUTTONS */
    UBYTE       nha_ButtonMode;
    UBYTE       nha_ButtonNo;
    /* HUA_RAWKEY */
    UBYTE       nha_RawKey;
    /* HUA_TABLET */
    UBYTE       nha_TabletAxis;
    /* HUA_SOUND */
    UBYTE       nha_SoundVolume;
    UBYTE       nha_ShellAsync;
    UBYTE       nha_MiscMode;
    /* action options */
    UBYTE       nha_AbsToRel;
    UBYTE       nha_ScaleEnable;
    UBYTE       nha_ClipEnable;
    UBYTE       nha_CCEnable;
    UBYTE       nha_ClipMin;
    UBYTE       nha_ClipMax;
    UBYTE       nha_ClipStretch;
    UBYTE       nha_CCVar1;
    UBYTE       nha_CCCond;
    UBYTE       nha_CCVar2;
    UBYTE       nha_ValEnable;
    UBYTE       nha_ValVar;
    LONG        nha_ScaleMin;
    LONG        nha_ScaleMax;
    LONG        nha_CCConst1;
    LONG        nha_CCConst2;
    LONG        nha_ValConst;
    /* HUA_DIGJOY */
    UBYTE       nha_JoypadOp;
    UBYTE       nha_JoypadFeat;
    UBYTE       nha_JoypadPort;
    /* HUA_VARIABLES */
    UBYTE       nha_TarVar;
    UBYTE       nha_TarVarOp;
    /* HUA_OUTPUT */
    UBYTE       nha_OutOp;
    UWORD       nha_OutItem;
    /* HUA_FEATURE */
    UWORD       nha_FeatItem;
    UBYTE       nha_FeatOp;
    UBYTE       nha_APadFeat;

    char        nha_SoundFile[256];
    /* HUA_VANILLA */
    char        nha_VanillaString[80];
    /* HUA_KEYSTRING */
    char        nha_KeyString[80];
    /* HUA_SHELL */
    char        nha_ExeString[80];
    char        nha_OutArray[256];
    BOOL        nha_IsDefault;
};

#if defined(__GNUC__)
# pragma pack()
#endif

/* Action Types */
#define HUA_NOP       0x0000 /* implemented (haha!) */
#define HUA_QUALIFIER 0x0001 /* implemented */
#define HUA_KEYMAP    0x0002 /* implemented */
#define HUA_RAWKEY    0x0003 /* implemented */
#define HUA_VANILLA   0x0004 /* implemented */
#define HUA_KEYSTRING 0x0005 /* implemented */
#define HUA_MOUSEPOS  0x0006 /* implemented */
#define HUA_BUTTONS   0x0007 /* implemented */
#define HUA_TABLET    0x0008 /* implemented */
#define HUA_DIGJOY    0x0009 /* implemented */
#define HUA_ANALOGJOY 0x000a /* implemented */
#define HUA_WHEEL     0x000b /* implemented */
#define HUA_SOUND     0x000c /* implemented */
#define HUA_SHELL     0x000d /* implemented */
#define HUA_AREXX     0x000e
#define HUA_OUTPUT    0x000f /* implemented */
#define HUA_FEATURE   0x0010 /* implemented */
#define HUA_MISC      0x0011 /* implemented */
#define HUA_VARIABLES 0x0012 /* implemented */
#define HUA_EXTRAWKEY 0x0013 /* implemented */
#define HUA_ATYPEMASK 0x0fff

#define HUA_DOWNEVENT 0x4000
#define HUA_UPEVENT   0x8000
#define HUA_ANY       (HUA_DOWNEVENT|HUA_UPEVENT)
#define HUA_ALWAYS    (0x2000|HUA_ANY)
#define HUA_NAN       0x1000
#define HUA_TRIGMASK  0xf000

#define HUAT_SET      0x01
#define HUAT_CLEAR    0x02
#define HUAT_TOGGLE   0x03
#define HUAT_ASSIGN   0x04
#define HUAT_ADD      0x05
#define HUAT_SUB      0x06
#define HUAT_MULTIPLY 0x07
#define HUAT_DIVIDE   0x08
#define HUAT_MODULO   0x09
#define HUAT_ASSNOT   0x0a

#define HUAT_AND      0x10
#define HUAT_OR       0x11
#define HUAT_XOR      0x12
#define HUAT_BWAND    0x13
#define HUAT_BWOR     0x14
#define HUAT_BWXOR    0x15
#define HUAT_ASL      0x16
#define HUAT_ASR      0x17
#define HUAT_ANDNOT   0x18
#define HUAT_BWANDNOT 0x19
#define HUAT_NAND     0x1a
#define HUAT_BWNAND   0x1b

#define HUAT_EQ       0x60
#define HUAT_NE       0x61
#define HUAT_LT       0x62
#define HUAT_LE       0x63
#define HUAT_GT       0x64
#define HUAT_GE       0x65

#define HUAT_DELTAX   0x20
#define HUAT_DELTAY   0x21
#define HUAT_LEFT     0x22
#define HUAT_RIGHT    0x23
#define HUAT_UP       0x24
#define HUAT_DOWN     0x25
#define HUAT_RED      0x26
#define HUAT_BLUE     0x27
#define HUAT_GREEN    0x28
#define HUAT_YELLOW   0x29
#define HUAT_FORWARD  0x2a
#define HUAT_REVERSE  0x2b
#define HUAT_PLAY     0x2c
#define HUAT_HATSWITCH 0x2d
#define HUAT_ABSX     0x30
#define HUAT_ABSY     0x31

#define HUAT_PRESSURE 0x40
#define HUAT_XROT     0x41
#define HUAT_YROT     0x42
#define HUAT_ZROT     0x43
#define HUAT_PROX     0x44
#define HUAT_ABSZ     0x45

#define HUAT_ACTWINDOW    0x80
#define HUAT_WIN2FRONT    0x81
#define HUAT_WIN2BACK     0x82
#define HUAT_CLOSEWINDOW  0x83
#define HUAT_ZIPWINDOW    0x84
#define HUAT_SCREENCYCLE  0x85
#define HUAT_WB2FRONT     0x86
#define HUAT_DISPLAYBEEP  0x87
#define HUAT_REBOOT       0x88
#define HUAT_FLUSHEVENTS  0x89

#define HUAT_GLOBVARA   0xa0
#define HUAT_GLOBVARB   0xa1
#define HUAT_GLOBVARC   0xa2
#define HUAT_GLOBVARD   0xa3
#define HUAT_GLOBVARE   0xa4
#define HUAT_GLOBVARF   0xa5
#define HUAT_GLOBVARG   0xa6
#define HUAT_GLOBVARH   0xa7

#define HUAT_LOCALVAR1  0xb1
#define HUAT_LOCALVAR2  0xb2
#define HUAT_LOCALVAR3  0xb3
#define HUAT_LOCALVAR4  0xb4
#define HUAT_LOCALVAR5  0xb5
#define HUAT_LOCALVAR6  0xb6
#define HUAT_LOCALVAR7  0xb7
#define HUAT_LOCALVAR8  0xb8

#define HUAT_EITEMVALUE 0xc0
#define HUAT_OITEMVALUE 0xc1
#define HUAT_CONST      0xc2
#define HUAT_CLICKCOUNT 0xc3
#define HUAT_QUALIFIERS 0xc4
#define HUAT_RANDOMBIT  0xc5
#define HUAT_RANDOMVAL  0xc6
#define HUAT_CLICKTIME  0xc7
#define HUAT_TIMER      0xc8
#define HUAT_ALLQUAL    0xc9


/* Annotations for the stuff

  Global interface stuff:
  Change keymap
  Shell con window, shell stack

  Reset handler, Reboot delay

  * HUA_QUALIFIER:
    {set|clear|toggle|assign} qualifier {X}
    [{report state} to LED page]

  * HUA_KEYMAP:
    1:1 mapping of keyboard to the global interface keymap

  * HUA_RAWKEY:
    Send {keyvalue} {down|up|both}
    [Record key]

  * HUA_VANILLA:
    Send {vanilla key} to keyboard

  * HUA_KEYSTRING:
    Send {keystring} to keyboard

  * HUA_MOUSEPOS:
    [CM] Send {X|Y} {relative|absolute} mousepos
    [CM] Move {X|Y} relative {delta}

  * HUA_BUTTONS:
    {set|clear|toggle|assign} {lmb|rmb|mmb|4b|5b}

  * HUA_TABLET:
    Send {X axis|Y axis|Z axis|X rot.|Y rot.|Z rot.|Pressure|Proximity}
    Pressure threshold for LMB: {percentage}

  * HUA_DIGJOY:
    Send {left|right|up|down|fire1|fire2} to port {0|1}
    Threshold {percentage}

  * HUA_ANALOGJOY:

  * HUA_WHEEL:
    Send {deltax|deltay|left|right|up|down}. {WheelDist}

  * HUA_SOUND:
    Play {soundfile} at {volume}

  * HUA_SHELL:
    Launch {commandline} {synchroneously|async}

  * HUA_MISC:
    {Activate Window|WindowToFront|WindowToBack|Screencycle|WB2Front|CloseWindow|ZoomWindow|Reboot}

  * HUA_AREXX:

  * HUA_OUTPUT:

  * HUA_EXTRAWKEY:
    Send {keyvalue} {down|up|both}

*/

#define HID_PARAM_UNDEF 0x80000000

struct BtHidGlobal
{
    struct Node nhg_Node;
    ULONG       nhg_UsagePage;
    LONG        nhg_LogicalMin;
    LONG        nhg_LogicalMax;
    LONG        nhg_PhysicalMin;
    LONG        nhg_PhysicalMax;
    LONG        nhg_UnitExp;
    ULONG       nhg_Unit;
    ULONG       nhg_ReportID;
    ULONG       nhg_ReportSize;
    ULONG       nhg_ReportCount;
};

struct BtHidUsage
{
    struct Node nhu_Node;
    ULONG       nhu_Usage;
    ULONG       nhu_UsageMax;
};

struct BtHidCollection
{
    struct Node nhc_Node;
    struct BtHidCollection *nhc_Parent;
    struct BtHidReport *nhc_Report; /* uplink */
    ULONG       nhc_Usage;        /* Usage ID */
    STRPTR      nhc_Name;         /* Name of this top collection */
    struct List nhc_Items;        /* List of items */
};

struct BtHidItem
{
    struct Node nhi_Node;
    struct BtHidCollection *nhi_Collection; /* uplink */
    UWORD       nhi_Type;
    ULONG       nhi_Flags;
    UWORD       nhi_Offset;
    UWORD       nhi_Size;
    BOOL        nhi_IsSigned;

    ULONG       nhi_Count;        /* only for arrays */
    ULONG       nhi_MapSize;      /* only for arrays */
    ULONG      *nhi_UsageMap;     /* only for arrays */
    struct List *nhi_ActionMap;   /* only for arrays */
    LONG       *nhi_Buffer;       /* only for arrays */
    LONG       *nhi_OldBuffer;    /* only for arrays */

    LONG        nhi_LogicalMin;
    LONG        nhi_LogicalMax;
    LONG        nhi_PhysicalMin;
    LONG        nhi_PhysicalMax;
    LONG        nhi_UnitExp;
    ULONG       nhi_Unit;

    LONG        nhi_RealMin;      /* after abs->rel conversion */
    LONG        nhi_RealMax;      /* after abs->rel conversion */

    ULONG       nhi_Usage;        /* only for variables */
    ULONG       nhi_DesignIndex;
    ULONG       nhi_StringIndex;

    LONG        nhi_OldValue;     /* old value */

    ULONG       nhi_LastMicros;   /* time of last up->down transition */
    ULONG       nhi_LastSeconds;
    ULONG       nhi_HoldMicros;   /* time of last up->down transition (until up event) */
    ULONG       nhi_HoldSeconds;
    ULONG       nhi_ClickCount;

    struct List nhi_ActionList;   /* Action (or default action for arrays) */
    BOOL        nhi_SameUsages;   /* For arrays, if all usageIDs are the same */
};

/* nhgi_ObjTypes */
#define NHGIOT_SHOTBUTTON   1
#define NHGIOT_TOGGLEBUTTON 2
#define NHGIOT_SLIDER       3
#define NHGIOT_SLIDERIMM    4

struct BtHidGItem
{
    struct Node         nhgi_Node;
    STRPTR              nhgi_Name;
    struct BtHidItem  *nhgi_Item;
    struct List        *nhgi_ActionList;
    Object             *nhgi_GUIObj;
    UWORD               nhgi_ObjType;
};

struct BtHidReport
{
    struct Node         nhr_Node;         /* Node linkage */
    ULONG               nhr_ReportLength; /* Report Buffer Length */
    UBYTE              *nhr_ReportBuf;    /* Report descriptor buffer */

    ULONG               nhr_ReportInSize; /* Size of report in bits */
    ULONG               nhr_ReportOutSize; /* Size of report in bits */
    ULONG               nhr_ReportFeatSize; /* Size of report in bits */
    ULONG               nhr_InItemCount;  /* Number of in items */
    ULONG               nhr_OutItemCount; /* Number of out items */
    ULONG               nhr_FeatItemCount; /* Number of feature items */
    ULONG               nhr_ReportID;     /* Report ID */
    BOOL                nhr_OutTouched;   /* Some output item was touched */
    BOOL                nhr_FeatTouched;  /* Some feature item was touched */
    struct List         nhr_Collections;  /* Top collections */
    struct BtHidItem **nhr_InItemMap;    /* Array to items at bitpos */
    struct BtHidItem **nhr_OutItemMap;   /* Array to items at bitpos */
    struct BtHidItem **nhr_FeatItemMap;  /* Array to items at bitpos */
    UWORD               nhr_ItemIDBase;   /* Starting offset for items (lh_Type) in this report */
};

struct BTHidBinding
{
    struct Node         nhb_Node;         /* Node linkage */
    struct BTHidBase  *nhb_ClsBase;      /* Up linkage */
    struct Library     *nhb_Base;         /* bluetooth.library base (binding task) */
    struct Library     *nhb_HIntBase;     /* Intuition base (hid task) */
    struct BtDevice    *nhb_Device;       /* Up linkage */
    struct BtService   *nhb_Service;      /* the HID service bound - NULL for the class defaults */
    BOOL                nhb_Classic;      /* HIDP over L2CAP rather than HID over GATT */
    /* input report channels: one per notifying HOGP Report characteristic, or
       the HIDP interrupt channel */
    APTR                nhb_ReadCh[BTHID_MAXCHANNELS];
    UBYTE              *nhb_ReadBuf[BTHID_MAXCHANNELS];
    UBYTE               nhb_ReadID[BTHID_MAXCHANNELS]; /* report id of each channel (HOGP) */
    UWORD               nhb_NumCh;
    ULONG               nhb_ReadBufSize;
    LONG                nhb_LastErr;
    /* every Report characteristic with its Report Reference (HOGP), for
       output/feature reports */
    struct BtEndpoint  *nhb_RepEP[BTHID_MAXREPORTEPS];
    UBYTE               nhb_RepEPID[BTHID_MAXREPORTEPS];
    UBYTE               nhb_RepEPType[BTHID_MAXREPORTEPS];
    UWORD               nhb_NumRepEP;
    UBYTE              *nhb_EPOutBuf;     /* output/feature report assembly buffer */
    char                nhb_WinTitle[96]; /* config window title */
    struct Task        *nhb_ReadySigTask; /* Task to send ready signal to */
    LONG                nhb_ReadySignal;  /* Signal to send when ready */
    struct Task        *nhb_Task;         /* Subtask */
    struct MsgPort     *nhb_TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *nhb_InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *nhb_InpIOReq;     /* input.device IORequest */
    struct InputEvent   nhb_FakeEvent;    /* Input Event */
    struct IENewTablet  nhb_TabletEvent;  /* Tablet Event */
    struct Library     *nhb_InputBase;    /* Pointer to input.device base */

    struct List         nhb_HidReports;   /* List of reports */
    struct List         nhb_HidStack;     /* Global Stack for push/pop */
    struct List         nhb_HidUsages;    /* List of usages */
    struct List         nhb_HidDesigns;   /* List of designators */
    struct List         nhb_HidStrings;   /* List of strings */
    struct BtHidGlobal nhb_HidGlobal;    /* Global variables */
    BOOL                nhb_UsesReportID; /* one byte report prefix */
    UWORD               nhb_MaxReportID;  /* max report ID */
    ULONG               nhb_MaxReportSize; /* Size of buffer */
    ULONG               nhb_MaxInSize;    /* Size of EPIn packets */
    ULONG               nhb_MaxOutSize;   /* Size of EPOut packets */
    ULONG               nhb_MaxFeatSize;  /* Size of EPOut packets */
    struct BtHidReport **nhb_ReportMap;  /* direct report mapping */

    STRPTR              nhb_DevIDString;  /* Device ID String */
    STRPTR              nhb_SvcIDString;  /* Service ID String */

    BOOL                nhb_OS4Hack;      /* Use ADDEVENT when possible */
    BOOL                nhb_TrackKeyEvents;
    ULONG               nhb_LastUSBKey;
    ULONG               nhb_CurrUSBKey;
    BOOL                nhb_TrackEvents;
    BOOL                nhb_ReportValues;
    BOOL                nhb_DisableActions;
    BOOL                nhb_ItemChanged;
    struct BtHidItem  *nhb_LastItem;
    struct List        *nhb_LastItemAList;

    struct BtHidItem  *nhb_XtraInitItem; /* item to be called at start */
    struct BtHidItem  *nhb_XtraQuitItem; /* item to be called at end */

    /* variables for actions */
    BOOL                nhb_OutFeatTouched; /* indicate change at output or feature items */
    BOOL                nhb_NewQualifiers; /* Trigger qualifiers event */
    ULONG               nhb_KeyQualifiers; /* Key qualifiers */

    BOOL                nhb_NewMouseRel;  /* Trigger mousepos event */
    ULONG               nhb_MouseAbsX;    /* Absolute Mouse X movement */
    ULONG               nhb_MouseAbsY;    /* Absolute Mouse Y movement */
    ULONG               nhb_MouseAbsZ;    /* Absolute Stylus Z movement */
    ULONG               nhb_MouseRangeX;  /* Max Mouse X range */
    ULONG               nhb_MouseRangeY;  /* Max Mouse Y range */
    ULONG               nhb_MouseRangeZ;  /* Max Mouse Z range */
    BOOL                nhb_NewMouseAbs;  /* Trigger mousepos event */
    ULONG               nhb_MouseDeltaX;  /* Delta Mouse X movement */
    ULONG               nhb_MouseDeltaY;  /* Delta Mouse Y movement */

    ULONG               nhb_MouseButtons; /* mouse button bits */
    BOOL                nhb_VldPressure;  /* Tablet Pressure valid */
    BOOL                nhb_VldRotX;      /* Tablet X Rotation valid */
    BOOL                nhb_VldRotY;      /* Tablet Y Rotation valid */
    BOOL                nhb_VldRotZ;      /* Tablet Y Rotation valid */
    BOOL                nhb_VldProx;      /* Tablet Proximits valid */

    LONG                nhb_TabPressure;  /* Tablet Pressure */
    LONG                nhb_TabRotX;      /* Tablet X Rotation */
    LONG                nhb_TabRotY;      /* Tablet Y Rotation */
    LONG                nhb_TabRotZ;      /* Tablet Z Rotation */
    BOOL                nhb_TabProx;      /* Tablet Proximity */

    ULONG               nhb_TabTags[20];

    struct BtHidItem  *nhb_RumbleMotors[2]; /* Rumble motor item cache */
    ULONG               nhb_LLPortState[4]; /* Joypad port status */
    ULONG               nhb_LLHatswitch[4]; /* Hatswitch stuff */
    ULONG               nhb_LLAnalogue[4]; /* Analogue joypad stuff */
    ULONG               nhb_LocalVars[8]; /* local variables */

    BOOL                nhb_UsingDefaultCfg;
    BOOL                nhb_ReloadCfg;    /* indicator for the HidTask */
    struct ClsDevCfg   *nhb_CDC;
    ULONG               nhb_LastCfgCRC;   /* use this to check config change */

    struct Library     *nhb_MUIBase;      /* MUI master base */
    struct Library     *nhb_BtBase;      /* Poseidon base */
    struct Library     *nhb_IntBase;      /* Intuition base (GUI task) */
    struct Library     *nhb_KeyBase;      /* Keymap base */
    struct Task        *nhb_GUITask;      /* GUI Task */
    LONG                nhb_TrackingSignal;

    BOOL                nhb_HasInItems;   /* do we need to read requests at all? */

    BOOL                nhb_IOStarted;    /* IO Running */
    BOOL                nhb_Running;      /* Not suspended */

    struct Hook         nhb_USBKeyListDisplayHook;
    struct Hook         nhb_ReportListDisplayHook;
    struct Hook         nhb_ItemListDisplayHook;
    struct Hook         nhb_ActionListDisplayHook;

    struct List         nhb_GUIItems;
    struct List         nhb_GUIOutItems;

    struct BtHidCollection *nhb_GUICurrentColl;
    struct BtHidGItem *nhb_GUICurrentItem;
    struct BtHidAction *nhb_GUICurrentAction;
    BOOL                nhb_SilentActionUpdate;

    struct MUI_CustomClass *nhb_ActionClass;

    Object             *nhb_App;
    Object             *nhb_MainWindow;
    Object             *nhb_ActionObj;
    Object             *nhb_ConWindowObj;
    Object             *nhb_ShellStackObj;
    Object             *nhb_EnableKBResetObj;
    Object             *nhb_EnableRHObj;
    Object             *nhb_ResetDelayObj;
    Object             *nhb_TurboMouseObj;

    Object             *nhb_HIDCtrlAutoObj;
    Object             *nhb_HIDCtrlOpenObj;
    Object             *nhb_HIDCtrlRexxObj;
    Object             *nhb_HIDCtrlTitleObj;

    Object             *nhb_LLPortModeObj[4];
    Object             *nhb_LLRumblePortObj;

    Object             *nhb_USBKeymapLVObj;
    Object             *nhb_RawKeymapLVObj;
    Object             *nhb_RestoreDefKeymapObj;
    Object             *nhb_TrackKeyEventsObj;

    Object             *nhb_ActionPageObj;
    Object             *nhb_ActionSelectorObj;
    Object             *nhb_ActionTriggerObj;

    Object             *nhb_ReportLVObj;
    Object             *nhb_FillDefObj;
    Object             *nhb_ClearActionsObj;
    Object             *nhb_TrackEventsObj;
    Object             *nhb_DisableActionsObj;
    Object             *nhb_ReportValuesObj;

    Object             *nhb_ItemLVObj;

    Object             *nhb_ActionLVObj;
    Object             *nhb_ActionNewObj;
    Object             *nhb_ActionCopyObj;
    Object             *nhb_ActionDelObj;
    Object             *nhb_ActionUpObj;
    Object             *nhb_ActionDownObj;

    Object             *nhb_ActionAbsToRelObj;
    Object             *nhb_ActionClipEnableObj;
    Object             *nhb_ActionScaleEnableObj;
    Object             *nhb_ActionCCEnableObj;
    Object             *nhb_ActionValEnableObj;

    Object             *nhb_A_ClipGroupObj;
    Object             *nhb_A_ClipMinObj;
    Object             *nhb_A_ClipMaxObj;
    Object             *nhb_A_ClipStretchObj;

    Object             *nhb_A_ScaleGroupObj;
    Object             *nhb_A_ScaleMinObj;
    Object             *nhb_A_ScaleMaxObj;

    Object             *nhb_A_CCGroupObj;
    Object             *nhb_A_CCVar1Obj;
    Object             *nhb_A_CCCondObj;
    Object             *nhb_A_CCVar2Obj;
    Object             *nhb_A_CCConst1Obj;
    Object             *nhb_A_CCConst2Obj;

    Object             *nhb_A_ValGroupObj;
    Object             *nhb_A_ValVarObj;
    Object             *nhb_A_ValConstObj;

    Object             *nhb_ActionAreaObj;

    Object             *nhb_A_KeyQualOpObj;
    Object             *nhb_A_KeyQualObj;
    Object             *nhb_A_RawKeyObj;
    Object             *nhb_A_RawKeyUpObj;
    Object             *nhb_A_VanillaStrObj;
    Object             *nhb_A_KeyStringObj;
    Object             *nhb_A_MousePosOpObj;
    Object             *nhb_A_MouseButOpObj;
    Object             *nhb_A_MouseButObj;
    Object             *nhb_A_TabletAxisObj;
    Object             *nhb_A_JoypadOpObj;
    Object             *nhb_A_JoypadFeatObj;
    Object             *nhb_A_JoypadPortObj;
    Object             *nhb_A_APadFeatObj;
    Object             *nhb_A_APadPortObj;
    Object             *nhb_A_WheelOpObj;
    Object             *nhb_A_WheelDistObj;
    Object             *nhb_A_SoundFileObj;
    Object             *nhb_A_SoundVolObj;
    Object             *nhb_A_ShellComObj;
    Object             *nhb_A_ShellAsyncObj;
    Object             *nhb_A_MiscOpObj;
    Object             *nhb_A_TarVarObj;
    Object             *nhb_A_TarVarOpObj;

    Object             *nhb_A_OutItemLVObj;
    Object             *nhb_A_OutArrayObj;
    Object             *nhb_A_OutOpObj;
    Object             *nhb_A_FeatItemLVObj;
    Object             *nhb_A_FeatArrayObj;
    Object             *nhb_A_FeatOpObj;
    Object             *nhb_A_ExtRawKeyObj;
    Object             *nhb_A_ExtRawKeyUpObj;

    Object             *nhb_UseObj;
    Object             *nhb_SetDefaultObj;
    Object             *nhb_CloseObj;

    Object             *nhb_AboutMI;
    Object             *nhb_UseMI;
    Object             *nhb_SetDefaultMI;
    Object             *nhb_MUIPrefsMI;
    Object             *nhb_SwapLMBRMBMI;
    Object             *nhb_MouseAccel100MI;
    Object             *nhb_MouseAccel150MI;
    Object             *nhb_MouseAccel200MI;
    Object             *nhb_JoyPort0MI;
    Object             *nhb_JoyPort1MI;
    Object             *nhb_JoyPort2MI;
    Object             *nhb_JoyPort3MI;
    Object             *nhb_JoyAutofireMI;
    Object             *nhb_DebugReportMI;

    BOOL                nhb_QuitGUI;

    struct KeymapCfg    nhb_KeymapCfg;
    char                nhb_TmpStrBuf0[128];
    char                nhb_TmpStrBufReport[128];
    char                nhb_TmpStrBufItem[128];
    char                nhb_TmpStrBufAction[128];
    struct HidUsageIDMap *nhb_USBKeyArray[257]; /* Pointer to array of all USB Keymap strings */
    STRPTR              nhb_RawKeyArray[129]; /* Pointer to array of all converted ANSI strings */
    STRPTR              nhb_ExtRawKeyArray[129]; /* Pointer to array of all converted ANSI strings */

    struct Library     *nhb_HCMUIBase;    /* MUI master base */
    struct Library     *nhb_HCBtBase;    /* Poseidon base */
    struct Library     *nhb_HCIntBase;    /* Intuition base (GUI task) */
    struct Task        *nhb_HCGUITask;    /* GUI Task */

    struct List         nhb_HCGUIItems;

    struct MUI_CustomClass *nhb_HCActionClass;

    Object             *nhb_HCApp;
    Object             *nhb_HCMainWindow;
    Object             *nhb_HCActionObj;
    Object             *nhb_HCGroupObj;
    Object             *nhb_HCCloseObj;

    Object             *nhb_HCAboutMI;
    Object             *nhb_HCCloseMI;
    Object             *nhb_HCMUIPrefsMI;
};

struct BTHidBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* Utility base */

    struct Task        *nh_DispatcherTask; /* external task to do all the dirty stuff */
    struct Task        *nh_ReadySigTask;  /* task to respond to */
    ULONG               nh_ReadySignal;   /* signal to use */

    struct Library     *nh_DOSBase;
    struct IntuitionBase *nh_IntBase;
    struct Library     *nh_DTBase;
    struct Library     *nh_CxBase;
    struct Library     *nh_LayersBase;
    struct Library     *nh_LowLevelBase;  /* lowlevel library base for patching */
    struct MsgPort     *nh_DTaskMsgPort;
    struct MsgPort     *nh_InpMsgPort;    /* input.device MsgPort */
    struct IOStdReq    *nh_InpIOReq;      /* input.device IORequest */
    struct InputEvent   nh_FakeEvent;     /* Input Event */

    APTR                nh_LLOldReadJoyPort; /* old vector */
    APTR                nh_LLOldSetJoyPortAttrsA; /* old vector */
    BOOL                nh_LLAnalogueOverride[4]; /* override prefs */
    BOOL                nh_OS4Hack;       /* Use ADDEVENT when possible */

    struct List         nh_Sounds;

    struct List         nh_Interfaces;    /* list of interfaces */

    LONG                nh_GlobalVars[8]; /* global variables */
    ULONG              *nh_IntFuncTable;

    struct BTHidBinding  nh_DefaultBinding;      /* Dummy NCH for default config */

    LONG                nh_Seed;          /* seed variable for random numbers */
};

#define WACOM_HASTILT    1
#define WACOM_HASWHEEL   2
#define WACOM_HASBUTTONS 4

#define WACOM_PENPARTNER 1
#define WACOM_GRAPHIRE   2
#define WACOM_GRAPHIRE4  3
#define WACOM_CINTIQ     4
#define WACOM_INTUOS     5
#define WACOM_INTUOS3    6
#define WACOM_PL         7
#define WACOM_PL2        8
#define WACOM_PLX        9

struct UsbToPs2Map
{
    ULONG utp_UsageID;
    UWORD utp_ExtCode;
};

struct BtHidSound
{
    struct Node nhs_Node;
    Object     *nhs_DTHandle;
};

struct ActionMsg
{
    struct Message am_Msg;
    struct BTHidBinding *am_NCH;
    struct BtHidAction *am_Action;
};

struct ActionData
{
    struct BTHidBinding *ad_NCH;
};

#define TAGBASE_Action (TAG_USER | 23<<16)
#define MUIM_Action_About           (TAGBASE_Action | 0x0001)
#define MUIM_Action_StoreConfig     (TAGBASE_Action | 0x0002)
#define MUIM_Action_DefaultConfig   (TAGBASE_Action | 0x0003)
#define MUIM_Action_UseConfig       (TAGBASE_Action | 0x0004)
#define MUIM_Action_UpdateDevPrefs  (TAGBASE_Action | 0x0008)
#define MUIM_Action_SelectReport    (TAGBASE_Action | 0x0010)
#define MUIM_Action_FillDefReport   (TAGBASE_Action | 0x0011)
#define MUIM_Action_ClearReport     (TAGBASE_Action | 0x0012)
#define MUIM_Action_SetTracking     (TAGBASE_Action | 0x0013)
#define MUIM_Action_SelectItem      (TAGBASE_Action | 0x0018)
#define MUIM_Action_SelectAction    (TAGBASE_Action | 0x0020)
#define MUIM_Action_NewAction       (TAGBASE_Action | 0x0021)
#define MUIM_Action_CopyAction      (TAGBASE_Action | 0x0022)
#define MUIM_Action_DelAction       (TAGBASE_Action | 0x0023)
#define MUIM_Action_MoveActionUp    (TAGBASE_Action | 0x0024)
#define MUIM_Action_MoveActionDown  (TAGBASE_Action | 0x0025)
#define MUIM_Action_UpdateAction    (TAGBASE_Action | 0x0026)
#define MUIM_Action_SetActionType   (TAGBASE_Action | 0x0027)
#define MUIM_Action_UpdateAOptions  (TAGBASE_Action | 0x0028)
#define MUIM_Action_KeymapSelectUSB (TAGBASE_Action | 0x0030)
#define MUIM_Action_KeymapSelectRaw (TAGBASE_Action | 0x0031)
#define MUIM_Action_RestDefKeymap   (TAGBASE_Action | 0x0032)

#define MUIM_Action_SwapLMBRMB      (TAGBASE_Action | 0x0050)
#define MUIM_Action_SetMouseAccel   (TAGBASE_Action | 0x0051)
#define MUIM_Action_SetJoyPort      (TAGBASE_Action | 0x0052)
#define MUIM_Action_AddAutofire     (TAGBASE_Action | 0x0053)

#define MUIM_Action_DebugReport     (TAGBASE_Action | 0x0060)

#define MUIM_Action_ShowHIDControl  (TAGBASE_Action | 0x0040)
#define MUIM_Action_HideHIDControl  (TAGBASE_Action | 0x0041)
#define MUIM_Action_UpdateHIDCtrl   (TAGBASE_Action | 0x0042)



/*
 *----------------------------------------------------------------------------
 *                         Includes for HID class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */


#include <devices/rawkeycodes.h>

#include <datatypes/soundclass.h>

#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/lowlevel_ext.h>
#include <graphics/layers.h>

#include <devices/usb_hid.h>   /* HID report descriptor item constants */

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


/* Protos */

struct BTHidBinding * GM_UNIQUENAME(bAttemptServiceBinding)(struct BTHidBase *nh, struct BtService *bsv);
struct BTHidBinding * GM_UNIQUENAME(bForceServiceBinding)(struct BTHidBase *nh, struct BtService *bsv);
void GM_UNIQUENAME(bReleaseServiceBinding)(struct BTHidBase *nh, struct BTHidBinding *nhb);

extern UBYTE usbkeymap[];

BOOL GM_UNIQUENAME(bLoadClassConfig)(struct BTHidBase *nh);
BOOL GM_UNIQUENAME(bLoadBindingConfig)(struct BTHidBinding *nhb, BOOL gui);
LONG GM_UNIQUENAME(bOpenBindingCfgWindow)(struct BTHidBase *nh, struct BTHidBinding *nhb);

void bInstallLLPatch(struct BTHidBase *nh);
struct BTHidBinding * GM_UNIQUENAME(bAllocHid)(void);
void GM_UNIQUENAME(bFreeHid)(struct BTHidBinding *nhb);

struct BtHidItem * bFindItemID(struct BTHidBinding *nhb, UWORD id, UWORD itype, ULONG *pos);
UWORD bFindItemUsage(struct BTHidBinding *nhb, ULONG usage, UWORD itype);
BOOL bFindCollID(struct BTHidBinding *nhb, struct BtHidCollection *nhc, ULONG collidmin, ULONG collidmax);
BOOL bDetectDefaultAction(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *lst, struct BtHidCollection *nhc, ULONG uid);
BOOL bCheckForDefaultAction(struct BTHidBinding *nhb,  struct BtHidItem *nhi, struct List *lst, struct BtHidCollection *nhc, ULONG uid);
struct BtHidAction * bAllocAction(struct BTHidBinding *nhb, struct List *lst, UWORD utype);

void bCheckReset(struct BTHidBinding *nhb);

BOOL bProcessItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, UBYTE *buf);
BOOL bDoAction(struct BTHidBinding *nhb, struct BtHidAction *nha, struct BtHidItem *nhi, ULONG uid, LONG value, BOOL downevent);
void bFlushEvents(struct BTHidBinding *nhb);
STRPTR bGetUsageName(struct BTHidBinding *nhb, ULONG uid);
void bCleanCollection(struct BTHidBinding *nhb, struct BtHidCollection *nhc);
void bSendRawKey(struct BTHidBinding *nhb, UWORD key);

void bFreeReport(struct BTHidBinding *nhb, struct BtHidReport *nhr);
BOOL bReadReports(struct BTHidBinding *nhb);
BOOL bParseReport(struct BTHidBinding *nhb, struct BtHidReport *nhrptr);
void bLoadActionConfig(struct BTHidBinding *nhb);
BOOL bAddExtraReport(struct BTHidBinding *nhb);

BOOL bAddUsage(struct BTHidBinding *nhb, struct List *list, ULONG umin, ULONG umax);

void bGenerateOutReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf);
void bGenerateFeatReport(struct BTHidBinding *nhb, struct BtHidReport *nhr, UBYTE *buf);
void bEncodeItemBuffer(struct BTHidBinding *nhb, struct BtHidItem *nhi, UBYTE *buf);

void bInstallLastActionHero(struct BTHidBinding *nhb);

void GM_UNIQUENAME(bGUITaskCleanup)(struct BTHidBinding *nhb);

struct BtHidGItem * bAllocGOutItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid);
struct BtHidGItem * bAllocGItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid);
void bFreeGItem(struct BTHidBinding *nhb, struct BtHidGItem *nhgi);

BOOL bLoadItem(struct BTHidBinding *nhb, struct BtIFFContext *rppic, struct List *lst, UWORD idbase);
struct BtIFFContext * bSaveItem(struct BTHidBinding *nhb, struct BtIFFContext *rppic, struct List *lst, UWORD idbase);

struct InputEvent *bInvertString(struct BTHidBase *nh, STRPTR str, struct KeyMap *km);
void bFreeIEvents(struct BTHidBase *nh, struct InputEvent *event);
BOOL bSendKeyString(struct BTHidBase *nh, STRPTR str);

void bLastActionHero(struct BTHidBase *nh);

void bDebugReport(struct BTHidBinding *nhb, struct BtHidReport *nhr);

struct BtHidSound * bLoadSound(struct BTHidBase *nh, STRPTR name);
BOOL bPlaySound(struct BTHidBase *nh, struct BtHidAction *nha);
void bFreeSound(struct BTHidBase *nh, struct BtHidSound *nhs);

LONG bEasyRequestA(struct BTHidBase *nh, STRPTR body, STRPTR gadgets, RAWARG params);

// FIXME
LONG bEasyRequest(struct BTHidBase *nh, STRPTR body, STRPTR gadgets, ...);

AROS_UFP0(void, GM_UNIQUENAME(bHidTask));
AROS_UFP0(void, GM_UNIQUENAME(bGUITask));
AROS_UFP0(void, GM_UNIQUENAME(bHIDCtrlGUITask));
AROS_UFP0(void, GM_UNIQUENAME(bDispatcherTask));

AROS_LD1(ULONG, bReadJoyPort,
         AROS_LDA(ULONG, port, D0),
         struct Library *, LowLevelBase, 5, bthid);
          
AROS_LD2(ULONG, bSetJoyPortAttrsA,
         AROS_LDA(ULONG, port, D0),
         AROS_LDA(struct TagItem *, tags, A1),
         struct Library *, LowLevelBase, 22, bthid);

AROS_UFP3(LONG, GM_UNIQUENAME(USBKeyListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct HidUsageIDMap *, hum, A1));

AROS_UFP3(LONG, GM_UNIQUENAME(ReportListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct BtHidCollection *, nhc, A1));
          
AROS_UFP3(LONG, GM_UNIQUENAME(ItemListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct BtHidGItem *, nhgi, A1));
          
AROS_UFP3(LONG, GM_UNIQUENAME(ActionListDisplayHook),
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct BtHidAction *, nha, A1));

AROS_UFP3(IPTR, GM_UNIQUENAME(ActionDispatcher),
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

AROS_UFP3(IPTR, GM_UNIQUENAME(HCActionDispatcher),
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

void GM_UNIQUENAME(bHIDCtrlGUITaskCleanup)(struct BTHidBinding *nhb);
struct BtHidGItem * bAllocGHCItem(struct BTHidBinding *nhb, struct BtHidItem *nhi, struct List *actionlist, ULONG usageid);

static inline UWORD GET_WTYPE(struct List *list)
{
    UWORD *w = (UWORD *)(&list->lh_Type);
    return *w;
}

static inline void SET_WTYPE(struct List *list, UWORD val)
{
    UWORD *w = (UWORD *)(&list->lh_Type);
    *w = val;
}


#endif /* BTHID_H */
