#ifndef HID_H
#define HID_H

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

struct NepHidActionChunk
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

struct NepHidAction
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

struct NepHidGlobal
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

struct NepHidUsage
{
    struct Node nhu_Node;
    ULONG       nhu_Usage;
    ULONG       nhu_UsageMax;
};

struct NepHidCollection
{
    struct Node nhc_Node;
    struct NepHidCollection *nhc_Parent;
    struct NepHidReport *nhc_Report; /* uplink */
    ULONG       nhc_Usage;        /* Usage ID */
    STRPTR      nhc_Name;         /* Name of this top collection */
    struct List nhc_Items;        /* List of items */
};

struct NepHidItem
{
    struct Node nhi_Node;
    struct NepHidCollection *nhi_Collection; /* uplink */
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

struct NepHidGItem
{
    struct Node         nhgi_Node;
    STRPTR              nhgi_Name;
    struct NepHidItem  *nhgi_Item;
    struct List        *nhgi_ActionList;
    Object             *nhgi_GUIObj;
    UWORD               nhgi_ObjType;
};

struct NepHidReport
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
    struct NepHidItem **nhr_InItemMap;    /* Array to items at bitpos */
    struct NepHidItem **nhr_OutItemMap;   /* Array to items at bitpos */
    struct NepHidItem **nhr_FeatItemMap;  /* Array to items at bitpos */
    UWORD               nhr_ItemIDBase;   /* Starting offset for items (lh_Type) in this report */
};

struct NepClassHid
{
    struct Node         nch_Node;         /* Node linkage */
    struct NepHidBase  *nch_ClsBase;      /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct Library     *nch_HIntBase;     /* Intuition base (hid class) */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *nch_EPIn;         /* Endpoint Int In */
    struct PsdPipe     *nch_EPInPipe;     /* Endpoint Int In pipe */
    struct PsdEndpoint *nch_EPOut;        /* optional Endpoint Int Out */
    struct PsdPipe     *nch_EPOutPipe;    /* optional Endpoint Int Out pipe */
    UBYTE              *nch_EPInBuf;      /* Packet buffer for EPIn */
    UBYTE              *nch_EPOutBuf;     /* Packet buffer for EPOut */
    IPTR                nch_EPInInterval; /* Interval for polling */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct MsgPort     *nch_InpMsgPort;   /* input.device MsgPort */
    struct IOStdReq    *nch_InpIOReq;     /* input.device IORequest */
    struct InputEvent   nch_FakeEvent;    /* Input Event */
    struct IENewTablet  nch_TabletEvent;  /* Tablet Event */
    struct Library     *nch_InputBase;    /* Pointer to input.device base */
    IPTR                nch_IfNum;        /* Interface number */

    struct UsbHidDesc  *nch_HidDesc;      /* HID Descriptor */
    struct List         nch_HidReports;   /* List of reports */
    struct List         nch_HidStack;     /* Global Stack for push/pop */
    struct List         nch_HidUsages;    /* List of usages */
    struct List         nch_HidDesigns;   /* List of designators */
    struct List         nch_HidStrings;   /* List of strings */
    struct NepHidGlobal nch_HidGlobal;    /* Global variables */
    BOOL                nch_UsesReportID; /* one byte report prefix */
    UWORD               nch_MaxReportID;  /* max report ID */
    ULONG               nch_MaxReportSize; /* Size of buffer */
    ULONG               nch_MaxInSize;    /* Size of EPIn packets */
    ULONG               nch_MaxOutSize;   /* Size of EPOut packets */
    ULONG               nch_MaxFeatSize;  /* Size of EPOut packets */
    struct NepHidReport **nch_ReportMap;  /* direct report mapping */

    STRPTR              nch_DevIDString;  /* Device ID String */
    STRPTR              nch_IfIDString;   /* Interface ID String */

    BOOL                nch_OS4Hack;      /* Use ADDEVENT when possible */
    BOOL                nch_TrackKeyEvents;
    ULONG               nch_LastUSBKey;
    ULONG               nch_CurrUSBKey;
    BOOL                nch_TrackEvents;
    BOOL                nch_ReportValues;
    BOOL                nch_DisableActions;
    BOOL                nch_ItemChanged;
    struct NepHidItem  *nch_LastItem;
    struct List        *nch_LastItemAList;

    struct NepHidItem  *nch_XtraInitItem; /* item to be called at start */
    struct NepHidItem  *nch_XtraQuitItem; /* item to be called at end */

    /* variables for actions */
    BOOL                nch_OutFeatTouched; /* indicate change at output or feature items */
    BOOL                nch_NewQualifiers; /* Trigger qualifiers event */
    ULONG               nch_KeyQualifiers; /* Key qualifiers */

    BOOL                nch_NewMouseRel;  /* Trigger mousepos event */
    ULONG               nch_MouseAbsX;    /* Absolute Mouse X movement */
    ULONG               nch_MouseAbsY;    /* Absolute Mouse Y movement */
    ULONG               nch_MouseAbsZ;    /* Absolute Stylus Z movement */
    ULONG               nch_MouseRangeX;  /* Max Mouse X range */
    ULONG               nch_MouseRangeY;  /* Max Mouse Y range */
    ULONG               nch_MouseRangeZ;  /* Max Mouse Z range */
    BOOL                nch_NewMouseAbs;  /* Trigger mousepos event */
    ULONG               nch_MouseDeltaX;  /* Delta Mouse X movement */
    ULONG               nch_MouseDeltaY;  /* Delta Mouse Y movement */

    ULONG               nch_MouseButtons; /* mouse button bits */
    BOOL                nch_VldPressure;  /* Tablet Pressure valid */
    BOOL                nch_VldRotX;      /* Tablet X Rotation valid */
    BOOL                nch_VldRotY;      /* Tablet Y Rotation valid */
    BOOL                nch_VldRotZ;      /* Tablet Y Rotation valid */
    BOOL                nch_VldProx;      /* Tablet Proximits valid */

    LONG                nch_TabPressure;  /* Tablet Pressure */
    LONG                nch_TabRotX;      /* Tablet X Rotation */
    LONG                nch_TabRotY;      /* Tablet Y Rotation */
    LONG                nch_TabRotZ;      /* Tablet Z Rotation */
    BOOL                nch_TabProx;      /* Tablet Proximity */

    ULONG               nch_TabTags[20];

    struct NepHidItem  *nch_RumbleMotors[2]; /* Rumble motor item cache */
    ULONG               nch_LLPortState[4]; /* Joypad port status */
    ULONG               nch_LLHatswitch[4]; /* Hatswitch stuff */
    ULONG               nch_LLAnalogue[4]; /* Analogue joypad stuff */
    ULONG               nch_LocalVars[8]; /* local variables */

    BOOL                nch_UsingDefaultCfg;
    BOOL                nch_ReloadCfg;    /* indicator for the HidTask */
    struct ClsDevCfg   *nch_CDC;
    ULONG               nch_LastCfgCRC;   /* use this to check config change */

    struct Library     *nch_MUIBase;      /* MUI master base */
    struct Library     *nch_PsdBase;      /* Poseidon base */
    struct Library     *nch_IntBase;      /* Intuition base (GUI task) */
    struct Library     *nch_KeyBase;      /* Keymap base */
    struct Task        *nch_GUITask;      /* GUI Task */
    LONG                nch_TrackingSignal;

    BOOL                nch_HasInItems;   /* do we need to read requests at all? */
    UWORD               nch_WacomMode;    /* Use Wacom stuff */
    struct NepHidCollection *nch_WacomColl; /* Wacom items */

    BOOL                nch_IOStarted;    /* IO Running */
    BOOL                nch_Running;      /* Not suspended */

    struct Hook         nch_USBKeyListDisplayHook;
    struct Hook         nch_ReportListDisplayHook;
    struct Hook         nch_ItemListDisplayHook;
    struct Hook         nch_ActionListDisplayHook;

    struct List         nch_GUIItems;
    struct List         nch_GUIOutItems;

    struct NepHidCollection *nch_GUICurrentColl;
    struct NepHidGItem *nch_GUICurrentItem;
    struct NepHidAction *nch_GUICurrentAction;
    BOOL                nch_SilentActionUpdate;

    struct MUI_CustomClass *nch_ActionClass;

    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_ActionObj;
    Object             *nch_ConWindowObj;
    Object             *nch_ShellStackObj;
    Object             *nch_EnableKBResetObj;
    Object             *nch_EnableRHObj;
    Object             *nch_ResetDelayObj;
    Object             *nch_TurboMouseObj;

    Object             *nch_HIDCtrlAutoObj;
    Object             *nch_HIDCtrlOpenObj;
    Object             *nch_HIDCtrlRexxObj;
    Object             *nch_HIDCtrlTitleObj;

    Object             *nch_LLPortModeObj[4];
    Object             *nch_LLRumblePortObj;

    Object             *nch_USBKeymapLVObj;
    Object             *nch_RawKeymapLVObj;
    Object             *nch_RestoreDefKeymapObj;
    Object             *nch_TrackKeyEventsObj;

    Object             *nch_ActionPageObj;
    Object             *nch_ActionSelectorObj;
    Object             *nch_ActionTriggerObj;

    Object             *nch_ReportLVObj;
    Object             *nch_FillDefObj;
    Object             *nch_ClearActionsObj;
    Object             *nch_TrackEventsObj;
    Object             *nch_DisableActionsObj;
    Object             *nch_ReportValuesObj;

    Object             *nch_ItemLVObj;

    Object             *nch_ActionLVObj;
    Object             *nch_ActionNewObj;
    Object             *nch_ActionCopyObj;
    Object             *nch_ActionDelObj;
    Object             *nch_ActionUpObj;
    Object             *nch_ActionDownObj;

    Object             *nch_ActionAbsToRelObj;
    Object             *nch_ActionClipEnableObj;
    Object             *nch_ActionScaleEnableObj;
    Object             *nch_ActionCCEnableObj;
    Object             *nch_ActionValEnableObj;

    Object             *nch_A_ClipGroupObj;
    Object             *nch_A_ClipMinObj;
    Object             *nch_A_ClipMaxObj;
    Object             *nch_A_ClipStretchObj;

    Object             *nch_A_ScaleGroupObj;
    Object             *nch_A_ScaleMinObj;
    Object             *nch_A_ScaleMaxObj;

    Object             *nch_A_CCGroupObj;
    Object             *nch_A_CCVar1Obj;
    Object             *nch_A_CCCondObj;
    Object             *nch_A_CCVar2Obj;
    Object             *nch_A_CCConst1Obj;
    Object             *nch_A_CCConst2Obj;

    Object             *nch_A_ValGroupObj;
    Object             *nch_A_ValVarObj;
    Object             *nch_A_ValConstObj;

    Object             *nch_ActionAreaObj;

    Object             *nch_A_KeyQualOpObj;
    Object             *nch_A_KeyQualObj;
    Object             *nch_A_RawKeyObj;
    Object             *nch_A_RawKeyUpObj;
    Object             *nch_A_VanillaStrObj;
    Object             *nch_A_KeyStringObj;
    Object             *nch_A_MousePosOpObj;
    Object             *nch_A_MouseButOpObj;
    Object             *nch_A_MouseButObj;
    Object             *nch_A_TabletAxisObj;
    Object             *nch_A_JoypadOpObj;
    Object             *nch_A_JoypadFeatObj;
    Object             *nch_A_JoypadPortObj;
    Object             *nch_A_APadFeatObj;
    Object             *nch_A_APadPortObj;
    Object             *nch_A_WheelOpObj;
    Object             *nch_A_WheelDistObj;
    Object             *nch_A_SoundFileObj;
    Object             *nch_A_SoundVolObj;
    Object             *nch_A_ShellComObj;
    Object             *nch_A_ShellAsyncObj;
    Object             *nch_A_MiscOpObj;
    Object             *nch_A_TarVarObj;
    Object             *nch_A_TarVarOpObj;

    Object             *nch_A_OutItemLVObj;
    Object             *nch_A_OutArrayObj;
    Object             *nch_A_OutOpObj;
    Object             *nch_A_FeatItemLVObj;
    Object             *nch_A_FeatArrayObj;
    Object             *nch_A_FeatOpObj;
    Object             *nch_A_ExtRawKeyObj;
    Object             *nch_A_ExtRawKeyUpObj;

    Object             *nch_UseObj;
    Object             *nch_SetDefaultObj;
    Object             *nch_CloseObj;

    Object             *nch_AboutMI;
    Object             *nch_UseMI;
    Object             *nch_SetDefaultMI;
    Object             *nch_MUIPrefsMI;
    Object             *nch_SwapLMBRMBMI;
    Object             *nch_MouseAccel100MI;
    Object             *nch_MouseAccel150MI;
    Object             *nch_MouseAccel200MI;
    Object             *nch_JoyPort0MI;
    Object             *nch_JoyPort1MI;
    Object             *nch_JoyPort2MI;
    Object             *nch_JoyPort3MI;
    Object             *nch_JoyAutofireMI;
    Object             *nch_DebugReportMI;

    BOOL                nch_QuitGUI;

    struct KeymapCfg    nch_KeymapCfg;
    char                nch_TmpStrBuf0[128];
    char                nch_TmpStrBufReport[128];
    char                nch_TmpStrBufItem[128];
    char                nch_TmpStrBufAction[128];
    struct HidUsageIDMap *nch_USBKeyArray[257]; /* Pointer to array of all USB Keymap strings */
    STRPTR              nch_RawKeyArray[129]; /* Pointer to array of all converted ANSI strings */
    STRPTR              nch_ExtRawKeyArray[129]; /* Pointer to array of all converted ANSI strings */

    struct Library     *nch_HCMUIBase;    /* MUI master base */
    struct Library     *nch_HCPsdBase;    /* Poseidon base */
    struct Library     *nch_HCIntBase;    /* Intuition base (GUI task) */
    struct Task        *nch_HCGUITask;    /* GUI Task */

    struct List         nch_HCGUIItems;

    struct MUI_CustomClass *nch_HCActionClass;

    Object             *nch_HCApp;
    Object             *nch_HCMainWindow;
    Object             *nch_HCActionObj;
    Object             *nch_HCGroupObj;
    Object             *nch_HCCloseObj;

    Object             *nch_HCAboutMI;
    Object             *nch_HCCloseMI;
    Object             *nch_HCMUIPrefsMI;
};

struct NepHidBase
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

    struct NepClassHid  nh_DummyNCH;      /* Dummy NCH for default config */

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

struct WacomCaps
{
    UWORD  wc_ProdID;
    UWORD  wc_Type;
    ULONG  wc_ResX;
    ULONG  wc_ResY;
    UWORD  wc_PressBits;
    STRPTR wc_Name;
};

struct WacomReport
{
    ULONG wr_PosX;
    ULONG wr_PosY;
    UWORD wr_Pressure;
    UBYTE wr_Buttons;
    UBYTE wr_InProximity;
    UBYTE wr_TiltX;
    UBYTE wr_TiltY;
    BYTE  wr_Wheel;
    UBYTE wr_PadButtons;
};

struct UsbToPs2Map
{
    ULONG utp_UsageID;
    UWORD utp_ExtCode;
};

struct NepHidSound
{
    struct Node nhs_Node;
    Object     *nhs_DTHandle;
};

struct ActionMsg
{
    struct Message am_Msg;
    struct NepClassHid *am_NCH;
    struct NepHidAction *am_Action;
};

struct ActionData
{
    struct NepClassHid *ad_NCH;
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

#endif /* HID_H */
