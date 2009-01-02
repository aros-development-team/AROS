
#include <libraries/mui.h>

/* Some internal macros */

#undef BOOLSAME
#define BOOLSAME(a,b) (((a) ? TRUE : FALSE)==((b) ? TRUE : FALSE))

/***********************************************************************/

/*
** bigint
*/

typedef struct
{
    ULONG hi;
    ULONG lo;
} bigint;

/***********************************************************************/
/*
** Messages
*/

/* Message used to wake-up the proc */
struct MPS_AppMsg
{
    struct Message link;
    void           *synccb;
    void           *asynccb;
    long           flags;
    UBYTE          *font;
    UBYTE          *screen;
    long           xo;
    long           yo;
    ULONG          *sigbit;
    ULONG          res;
};

/* Message prototype */
struct MPS_Msg
{
    struct Message link;
    ULONG          type;
    ULONG          size;
    ULONG          flags;
};

enum
{
    MPV_Msg_Flags_Reply = 1<<0, /* The message must be replied */
};

/* Messages types */
enum
{
    MPV_Msg_Type_Cleanup = 0,
    MPV_Msg_Type_AddInterface,
    MPV_Msg_Type_DelInterface,
    MPV_Msg_Type_SetInterfaceState,
    MPV_Msg_Type_SetInterfaceSpeed,
    MPV_Msg_Type_InterfaceReport,
    MPV_Msg_Type_ToFront,
    MPV_Msg_Type_InhibitRefresh,
    MPV_Msg_Type_GetCoord,
    MPV_Msg_Type_Event,
    MPV_Msg_Type_RefreshName,

    MPV_Msg_Type_Last
};

/* Message used to add an inteerface */
struct MPS_Msg_AddInterface
{
    struct MPS_Msg link;
    long           unit;
    UBYTE          *name;
    long           state;
    long           ontime;
    UBYTE          *speed;
};

/* Message used to delete an interface */
struct MPS_Msg_DelInterface
{
    struct MPS_Msg link;
    long           unit;
};

/* Message used to change state/ontime */
struct MPS_Msg_SetInterfaceState
{
    struct MPS_Msg link;
    long           unit;
    long           state;
    long           ontime;
};

/* Message used to change the speed */
struct MPS_Msg_SetInterfaceSpeed
{
    struct MPS_Msg link;
    long           unit;
    UBYTE          *speed;
};

/* Message used to change rate/ontime/traffic */
struct MPS_Msg_InterfaceReport
{
    struct MPS_Msg link;
    long           unit;
    long           rate;
    long           now;
    bigint         total;
};

/* Message used to start/stop an interface add/delete phase */
struct MPS_Msg_InhibitRefresh
{
    struct MPS_Msg link;
    long           val;
};

/* Message used to get the window x/y - Unused */
struct MPS_Msg_GetCoord
{
    struct MPS_Msg link;
    long           *xp;
    long           *yp;
};

/* Message used to report a sinal bit event - Unused */
struct MPS_Msg_Event
{
    struct MPS_Msg link;
    ULONG          sigs;
};

/* Message used to change an interface name */
struct MPS_Msg_RefreshName
{
    struct MPS_Msg  link;
    long            unit;
    UBYTE           *name;
};

/***********************************************************************/
/*
** MCCs tag base
*/

#define MUIMPANEL_TAG(n) ((int)(0xfec90000+(n)))

/***********************************************************************/
/*
** Preferences
*/

/* The actual preferences version */
enum
{
    MPV_Prefs_Version = 1,
};

struct MPS_Prefs
{
    ULONG          layout;
    ULONG          flags;

    ULONG          barLayout;
    ULONG          viewMode;
    ULONG          labelPos;
    ULONG          btflags;

    struct MinList iflist;
};

struct MPS_TinyPrefs
{
    ULONG          layout;
    ULONG          flags;

    ULONG          barLayout;
    ULONG          viewMode;
    ULONG          labelPos;
    ULONG          btflags;
};

/* Layout */
enum
{
    MPV_Layout_Horiz      = 1<<0,
    MPV_Layout_Left       = 1<<1,
    MPV_Layout_Right      = 1<<2,
    MPV_Layout_PureTop    = 1<<3,
    MPV_Layout_PureBottom = 1<<4,
    MPV_Layout_Top        = MPV_Layout_PureTop|MPV_Layout_Horiz,
    MPV_Layout_Bottom     = MPV_Layout_PureBottom|MPV_Layout_Horiz,
};

/* flags */
enum
{
    MPV_Flags_TrafficShort      = 1<<0,
    MPV_Flags_TrafficNoGrouping = 1<<1,
    MPV_Flags_RateShort         = 1<<2,
    MPV_Flags_RateNoGrouping    = 1<<3,
    MPV_Flags_UseBusyBar        = 1<<4,
    MPV_Flags_Iconify           = 1<<5,
    MPV_Flags_BWin              = 1<<6,
    MPV_Flags_BWinBorders       = 1<<7,
    MPV_Flags_UseTransparency   = 1<<8,
};

/* btflags */
enum
{
    MPV_BTFlags_Borderless = 1<<0,
    MPV_BTFlags_Sunny      = 1<<1,
    MPV_BTFlags_Raised     = 1<<2,
    MPV_BTFlags_Scaled     = 1<<3,
    MPV_BTFlags_Underscore = 1<<4,
    MPV_BTFlags_Frame      = 1<<5,
    MPV_BTFlags_DragBar    = 1<<6,
};

#define PREFS(p) ((struct MPS_Prefs *)(p))

/* An interface in iflist */
struct ifnode
{
    struct MinNode link;
    UBYTE      name[16];
    ULONG          scale;
};


/* To use with loadIFFPrefs() */
enum
{
    MPV_LoadPrefs_Env      = 1<<0,
    MPV_LoadPrefs_EnvArc   = 1<<1,
    MPV_LoadPrefs_FallBack = 1<<2,
};

/***********************************************************************/
/*
** App - Application.mui subclass
** The core.
**/

/* Methods */
enum
{
    MPM_Rebuild    = MUIMPANEL_TAG(0),
    MPM_Quit       = MUIMPANEL_TAG(1),
    MPM_DisposeWin = MUIMPANEL_TAG(2),
    MPM_Save       = MUIMPANEL_TAG(3),
    MPM_Load       = MUIMPANEL_TAG(4),
    MPM_About      = MUIMPANEL_TAG(5),
    MPM_Miami      = MUIMPANEL_TAG(6),
    MPM_Prefs      = MUIMPANEL_TAG(7),
};

struct MPP_DisposeWin
{
    ULONG  MethodID;
    Object *win;
};

struct MPP_Save
{
    ULONG MethodID;
    ULONG save;
};

struct MPP_Load
{
    ULONG MethodID;
    ULONG envarc;
};

struct MPP_Miami
{
    ULONG MethodID;
    ULONG cmd;
};

enum
{
    MPV_Miami_Show,
    MPV_Miami_Hide,
    MPV_Miami_Quit,
};

/* Attributes */
enum
{
    MPA_Application = MUIMPANEL_TAG(0),
    MPA_Show        = MUIMPANEL_TAG(1),
    MPA_Prefs       = MUIMPANEL_TAG(2),
    MPA_OneWay      = MUIMPANEL_TAG(3),
    MPA_Bar         = MUIMPANEL_TAG(4),
    MPA_Value       = MUIMPANEL_TAG(5),
    MPA_NoIfList    = MUIMPANEL_TAG(6),
    MPA_SkipBar     = MUIMPANEL_TAG(7),
};

/***********************************************************************/
/*
** Prefs - Window.mui subclass
** Edit preferences.
*/

/* Methods */
enum
{
    MPM_Prefs_UsePrefs = MUIMPANEL_TAG(15),
};

struct MPP_Prefs_UsePrefs
{
    ULONG  MethodID;
    ULONG  mode;
};

enum
{
    MPV_Prefs_UsePrefs_Save,
    MPV_Prefs_UsePrefs_Use,
    MPV_Prefs_UsePrefs_Apply,
    MPV_Prefs_UsePrefs_Test,
    MPV_Prefs_UsePrefs_Cancel,
};

/* No attribute defined */

/***********************************************************************/
/*
** MGroup - Group.mui subclass
** Made of IFGroup & CGroup.
** May change layout on the fly.
*/

/* Methods */
enum
{
    MPM_MGroup_GrabIFList     = MUIMPANEL_TAG(16),
    MPM_MGroup_UpdateTransparency = MUIMPANEL_TAG(17),
};

/* No attribute defined */

/***********************************************************************/
/*
** IfGroup - Virtgroup.mui subclass
** Interfaces list.
**/

/* Methods */
enum
{
    MPM_IfGroup_HandleEvent = MUIMPANEL_TAG(20),
    MPM_IfGroup_GrabIFList,
};

/* No attribute defined */

enum
{
    TAG_SCALE = MUIMPANEL_TAG(30),

    TAG_SCALE_1,
    TAG_SCALE_2,
    TAG_SCALE_3,
    TAG_SCALE_4,
    TAG_SCALE_5,
    TAG_SCALE_6,
    TAG_SCALE_7,
    TAG_SCALE_8,

    TAG_SCALE_LAST
};

/***********************************************************************/
/*
** If - Group.mui subclass
** Rappresents an interface.
*/

/* Methods */
enum
{
    MPM_If_Switch = MUIMPANEL_TAG(50),
};

/* Attributes */
enum
{
    MPA_If_Unit     = MUIMPANEL_TAG(50),
    MPA_If_Name     = MUIMPANEL_TAG(51),
    MPA_If_State    = MUIMPANEL_TAG(52),
    MPA_If_Ontime   = MUIMPANEL_TAG(53),
    MPA_If_Traffic  = MUIMPANEL_TAG(54),
    MPA_If_Rate     = MUIMPANEL_TAG(55),
    MPA_If_Now      = MUIMPANEL_TAG(56),
    MPA_If_Speed    = MUIMPANEL_TAG(57),
    MPA_If_PMExLlst = MUIMPANEL_TAG(58),
    MPA_If_Scale    = MUIMPANEL_TAG(59),
};

/***********************************************************************/
/*
** LButton - Group.mui subclass
** Made of a Lamp and a Text.
*/

/* No method defined */

/* Attributes */
enum
{
    MPA_LButton_State = MUIMPANEL_TAG(70),
};

/***********************************************************************/
/*
** MUI Macros
**/

/* Menus */
#define MTITLE(t)  {NM_TITLE,NULL,0,0,0,(APTR)(t)}
#define MITEM(t)   {NM_ITEM,NULL,0,0,0,(APTR)(t)}
#define MBAR       {NM_ITEM,(UBYTE *)NM_BARLABEL,0,0,0,NULL}
#define MXSUB(t,m) {NM_SUB,NULL,0,CHECKIT,(LONG)(m),(APTR)(t)}
#define MEND       {NM_END,NULL,0,0,0,NULL}

/* PopupMenu */
#define PMMenuIDNoTitle(id)  PM_MakeMenu(\
                             PM_Item, PM_MakeItem(PM_Hidden,TRUE,TAG_DONE),\
                             PM_Item, PM_MakeItem(PM_Title,"",PM_NoSelect,TRUE,PM_ShinePen,TRUE,PM_Shadowed,TRUE,PM_Center,TRUE,PM_ID,id,TAG_DONE),\
                             PM_Item, PM_MakeItem(PM_WideTitleBar,TRUE,TAG_DONE)
#define _pmenu(pm)           ((struct PopupMenu *)(pm))
#define pmset(pm,attr,value) PM_SetItemAttrs(_pmenu(pm),attr,value,TAG_DONE)

/* MCCs */
#if !defined(__AROS__)
#define appObject            NewObject(MiamiPanelBaseIntern->mpb_appClass->mcc_Class,NULL
#define aboutObject         NewObject(MiamiPanelBaseIntern->mpb_aboutClass->mcc_Class,NULL
#define prefsObject          NewObject(MiamiPanelBaseIntern->mpb_prefsClass->mcc_Class,NULL
#define mgroupObject      NewObject(MiamiPanelBaseIntern->mpb_mgroupClass->mcc_Class,NULL
#define ifGroupObject      NewObject(MiamiPanelBaseIntern->mpb_ifGroupClass->mcc_Class,NULL
#define ifObject               NewObject(MiamiPanelBaseIntern->mpb_ifClass->mcc_Class,NULL
#define lbuttonObject       NewObject(MiamiPanelBaseIntern->mpb_lbuttonClass->mcc_Class,NULL
#define rateObject           NewObject(MiamiPanelBaseIntern->mpb_rateClass->mcc_Class,NULL
#define trafficObject        NewObject(MiamiPanelBaseIntern->mpb_trafficClass->mcc_Class,NULL
#define timeTextObject    NewObject(MiamiPanelBaseIntern->mpb_timeTextClass->mcc_Class,NULL
#endif

/* Various */
#define IDS(s) ((ULONG *)(s+sizeof(s)/sizeof(UBYTE *)/2))

/***********************************************************************/
/*
** Default preferecences
*/
#define DEF_Layout            MPV_Layout_Top
#define DEF_Flags             (MPV_Flags_TrafficShort|MPV_Flags_RateShort|MPV_Flags_UseBusyBar|MPV_Flags_BWin|MPV_Flags_BWinBorders)
#define DEF_TBLayout          MUIV_TheBar_BarPos_Left
#define DEF_TBVMode           MUIV_TheBar_ViewMode_TextGfx
#define DEF_TBLPos            MUIV_TheBar_LabelPos_Right
#define DEF_TBFlags           (MPV_BTFlags_Underscore|MPV_BTFlags_Frame|MPV_BTFlags_DragBar)
#define DEF_Scale             (scales[0].ti_Data)

/***********************************************************************/
/*
** Global definitions
*/

#define DEF_Author                 "Alfonso Ranieri"
#define DEF_EMail                   "alforan@tin.it"
#define DEF_HomePage          "http://alfie.altervista.org"
#define DEF_Base                   "MUI.MiamiPanel"
#define DEF_Guide                  "MIAMI:"DEF_Base".guide"
#define DEF_Icon                    "ENVARC:Sys/MiamiPanel"
#define DEF_Catalog               DEF_Base".catalog"
#define DEF_ENVFILE               "ENV:MUI/"DEF_Base
#define DEF_ENVARCFILE         "ENVARC:MUI/"DEF_Base

/*****************************************************************************/
/*
** Callback types definitions
*/

typedef long (MiamiPanelCallBackType)(APTR code,LONG count,UBYTE *args);

/*****************************************************************************/
/*
** Commands codes
*/

enum
{
    MIAMIPANELV_CallBack_Code_UnitOnline  = 112,
    MIAMIPANELV_CallBack_Code_UnitOffline = 113,

    MIAMIPANELV_CallBack_Code_ShowMainGUI = 56,
    MIAMIPANELV_CallBack_Code_HideMainGUI = 19,
    MIAMIPANELV_CallBack_Code_ClosePanel  = 110,
    MIAMIPANELV_CallBack_Code_QuitMiami   = 123,

    MIAMIPANELV_CallBack_Code_Localize    = 27,
};

/*****************************************************************************/
/*
** Interface states
*/

enum
{
    MIAMIPANELV_AddInterface_State_GoingOnline  = 1<<8,
    MIAMIPANELV_AddInterface_State_GoingOffline = 1<<9,
    MIAMIPANELV_AddInterface_State_Suspending   = 1<<10,
    MIAMIPANELV_AddInterface_State_Offline      = 1<<0,
    MIAMIPANELV_AddInterface_State_Online       = 1<<1,
    MIAMIPANELV_AddInterface_State_Suspended    = 1<<2,
};

/*****************************************************************************/
/*
** Flags defining the appearance of the control panel
*/

enum
{
    MIAMIPANELV_Init_Flags_ShowSpeed            = 1<<0,
    MIAMIPANELV_Init_Flags_ShowDataTransferRate = 1<<1,
    MIAMIPANELV_Init_Flags_ShowUpTime           = 1<<2,
    MIAMIPANELV_Init_Flags_ShowTotal            = 1<<3,
    MIAMIPANELV_Init_Flags_ShowStatusButton     = 1<<4,
    MIAMIPANELV_Init_Flags_Control              = 1<<5,
};

/*****************************************************************************/
/*
** String codes
*/

enum
{
    MIAMIPANELV_String_Status_GoingOnline  = 5000, /* ">On"  */
    MIAMIPANELV_String_Status_GoingOffline = 5001, /* ">Of"  */
    MIAMIPANELV_String_Status_Suspending   = 5002, /* ">Su"  */
    MIAMIPANELV_String_Status_Online       = 5003, /* "Onl"  */
    MIAMIPANELV_String_Status_Offline      = 5004, /* "Off"  */
    MIAMIPANELV_String_Status_Suspended    = 5005, /* "Sus"  */

    MIAMIPANELV_String_Button_Show         = 5006, /* "Show" */
    MIAMIPANELV_String_Button_Hide         = 5007, /* "Hide" */
    MIAMIPANELV_String_Button_Quit         = 5008, /* "Quit" */
    MIAMIPANELV_String_Button_Online       = 5009, /* "Onl"  */
    MIAMIPANELV_String_Button_Offline      = 5010, /* "Off"  */
};

/*****************************************************************************/
