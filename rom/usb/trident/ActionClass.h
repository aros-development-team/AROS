/*****************************************************************************
** This is the Action custom class, a sub class of Group.mui.
******************************************************************************/
#ifndef ACTIONCLASS_H
#define ACTIONCLASS_H
/* private structures */

struct DefListEntry
{
    struct Node  node;
    Object      *infowindow;
};

struct HWListEntry
{
    struct Node  node;
    Object      *infowindow;
    Object      *classpopup;
    struct Node *phw;
    STRPTR       devname;
    IPTR         unit;
    STRPTR       prodname;
};

struct PrefsListEntry
{
    struct Node  node;
    Object      *infowindow;
    APTR        *pic;
    STRPTR       id;
    STRPTR       owner;
    STRPTR       type;
    ULONG        chunkid;
    STRPTR       devid;
    STRPTR       ifid;
    ULONG        size;
};

struct DevListEntry
{
    struct Node  node;
    Object      *infowindow;
    struct Node *pd;
    struct DevWinData *devdata;
    struct ActionData *adata;
};

struct ClsListEntry
{
    struct Node  node;
    Object      *infowindow;
    struct Node *puc;
    STRPTR       desc;
};

struct ErrListEntry
{
    struct Node  node;
    struct Node *pem;
};


struct ActionData
{
    struct Hook HardwareDisplayHook;
    struct Hook PrefsDisplayHook;
    struct Hook DeviceDisplayHook;
    struct Hook ClassDisplayHook;
    struct Hook ErrorDisplayHook;
    struct Hook IconDisplayHook;

    struct List hwlist;
    struct List prefslist;
    struct HWListEntry *acthlnode;
    struct List devlist;
    struct List clslist;
    struct List errlist;
    ULONG errorlevel;
    struct MsgPort *eventmsgport;
    APTR            eventhandler;

    ULONG  *configroot;
    STRPTR  devidstr;
    STRPTR  ifidstr;

    BOOL    autoclassesadded;
    BOOL    swallowconfigevent;

    struct Task *OnlUpdTask;

    struct MUI_InputHandlerNode eventihn;

    Object *selfobj;
    Object *appobj;
    Object *winobj;
    Object *mainobj;
    Object *cfgpagelv;
    Object *cfgpagegrp;
    Object *cfgcntobj[7];

    Object *hwlistobj;
    Object *hwdevgrpobj;
    Object *hwdevaslobj;
    Object *hwdevobj;
    Object *hwunitobj;
    Object *hwnewobj;
    Object *hwcopyobj;
    Object *hwdelobj;
    Object *hwinfoobj;
    Object *hwonlineobj;
    Object *hwofflineobj;

    Object *onlineobj;
    Object *offlineobj;
    Object *restartobj;
    Object *saveobj;
    Object *useobj;

    Object *devlistobj;
    Object *devbindobj;
    Object *devunbindobj;
    Object *devpowercycleobj;
    Object *devdisableobj;
    Object *devsuspendobj;
    Object *devresumeobj;
    Object *devinfoobj;
    Object *devcfgobj;

    Object *clslistobj;
    Object *clsnameobj;
    Object *clsaddobj;
    Object *clsremobj;
    Object *clscfgobj;
    Object *clsscanobj;
    Object *errlvlobj;
    Object *errsaveobj;
    Object *errflushobj;
    Object *errlistobj;

    Object *cfgtaskpriobj;
    Object *cfgbootdelayobj;
    Object *cfgloginfoobj;
    Object *cfglogwarnobj;
    Object *cfglogerrobj;
    Object *cfglogfailobj;
    Object *cfgpopupnewobj;
    Object *cfgpopupgoneobj;
    Object *cfgpopupdeathobj;
    Object *cfgpopupdelayobj;
    Object *cfgpopupactivateobj;
    Object *cfgpopuptofrontobj;
    Object *cfgdevdtxsoundobj;
    Object *cfgdevremsoundobj;

    Object *cfgautolpobj;
    Object *cfgautodeadobj;
    Object *cfgautopcobj;

    Object *cfgpowersavingobj;
    Object *cfgforcesuspendobj;
    Object *cfgsuspendtimeoutobj;

    Object *mempoolobj;

    Object *prefslistobj;
    Object *prefsexportobj;
    Object *prefsimportobj;
    Object *prefsremoveobj;
    Object *prefssaveasobj;

    Object *mi_classpopup;
};

#define TAGBASE_Action (TAG_USER | 23<<16)
#define MUIM_Action_HW_New          (TAGBASE_Action | 0x0001)
#define MUIM_Action_HW_Copy         (TAGBASE_Action | 0x0002)
#define MUIM_Action_HW_Del          (TAGBASE_Action | 0x0003)
#define MUIM_Action_HW_Info         (TAGBASE_Action | 0x0004)
#define MUIM_Action_HW_InfoClose    (TAGBASE_Action | 0x0005)
#define MUIM_Action_HW_Activate     (TAGBASE_Action | 0x0006)
#define MUIM_Action_HW_Update       (TAGBASE_Action | 0x0007)
#define MUIM_Action_HW_Online       (TAGBASE_Action | 0x0008)
#define MUIM_Action_HW_Offline      (TAGBASE_Action | 0x0009)
#define MUIM_Action_Online          (TAGBASE_Action | 0x0010)
#define MUIM_Action_Offline         (TAGBASE_Action | 0x0011)
#define MUIM_Action_ChgErrLevel     (TAGBASE_Action | 0x0012)
#define MUIM_Action_FlushErrors     (TAGBASE_Action | 0x0013)
#define MUIM_Action_Use             (TAGBASE_Action | 0x0014)
#define MUIM_Action_LoadPrefs       (TAGBASE_Action | 0x0015)
#define MUIM_Action_SavePrefs       (TAGBASE_Action | 0x0016)
#define MUIM_Action_SavePrefsAs     (TAGBASE_Action | 0x0017)
#define MUIM_Action_SaveQuit        (TAGBASE_Action | 0x0018)
#define MUIM_Action_UseQuit         (TAGBASE_Action | 0x0019)
#define MUIM_Action_SaveErrors      (TAGBASE_Action | 0x001b)
#define MUIM_Action_SaveDeviceList  (TAGBASE_Action | 0x001c)
#define MUIM_Action_Restart         (TAGBASE_Action | 0x001d)
#define MUIM_Action_Dev_Activate    (TAGBASE_Action | 0x0020)
#define MUIM_Action_Dev_Bind        (TAGBASE_Action | 0x0021)
#define MUIM_Action_Dev_Unbind      (TAGBASE_Action | 0x0022)
#define MUIM_Action_Dev_Info        (TAGBASE_Action | 0x0023)
#define MUIM_Action_Dev_Configure   (TAGBASE_Action | 0x0025)
#define MUIM_Action_Dev_ForceBind   (TAGBASE_Action | 0x0026)
#define MUIM_Action_Dev_Suspend     (TAGBASE_Action | 0x0027)
#define MUIM_Action_Dev_Resume      (TAGBASE_Action | 0x0028)
#define MUIM_Action_Dev_PowerCycle  (TAGBASE_Action | 0x0029)
#define MUIM_Action_Dev_Disable     (TAGBASE_Action | 0x002a)
#define MUIM_Action_Cls_Activate    (TAGBASE_Action | 0x0030)
#define MUIM_Action_Cls_Add         (TAGBASE_Action | 0x0031)
#define MUIM_Action_Cls_Remove      (TAGBASE_Action | 0x0032)
#define MUIM_Action_Cls_Configure   (TAGBASE_Action | 0x0033)
#define MUIM_Action_Cls_Scan        (TAGBASE_Action | 0x0034)
#define MUIM_Action_Cfg_Changed     (TAGBASE_Action | 0x0040)
#define MUIM_Action_Cfg_Snd_Changed (TAGBASE_Action | 0x0041)
#define MUIM_Action_Info_MemPool    (TAGBASE_Action | 0x0050)
#define MUIM_Action_CloseSubWinReq  (TAGBASE_Action | 0x0051)
#define MUIM_Action_About           (TAGBASE_Action | 0x0060)
#define MUIM_Action_WakeUp          (TAGBASE_Action | 0x0061)
#define MUIM_Action_HandlePsdEvents (TAGBASE_Action | 0x0062)
#define MUIM_Action_Cfg_Activate    (TAGBASE_Action | 0x0070)
#define MUIM_Action_Cfg_Export      (TAGBASE_Action | 0x0071)
#define MUIM_Action_Cfg_Import      (TAGBASE_Action | 0x0072)
#define MUIM_Action_Cfg_Remove      (TAGBASE_Action | 0x0073)

/* MPREFS */
#define MUIM_Action_LoadPrefsFrom   (TAGBASE_Action | 0x0080)
#define MUIM_Action_SavePrefsTo     (TAGBASE_Action | 0x0081)
#define MUIM_Action_Prefs_Changed   (TAGBASE_Action | 0x0082)


/* prototypes */

struct HWListEntry * AllocHWEntry(struct ActionData *data, struct Node *phw);
void FreeHWEntry(struct ActionData *data, struct HWListEntry *hlnode);
struct DevListEntry * AllocDevEntry(struct ActionData *data, struct Node *pd);
void FreeDevEntry(struct ActionData *data, struct DevListEntry *dlnode);
struct ClsListEntry * AllocClsEntry(struct ActionData *data, struct Node *puc);
void FreeClsEntry(struct ActionData *data, struct ClsListEntry *clnode);
void FreeErrorList(struct ActionData *data);
void CreateErrorList(struct ActionData *data);
Object * CreateClassPopup(void);
void CleanupEventHandler(struct ActionData *data);
BOOL SetupEventHandler(struct ActionData *data);
void UpdateConfigToGUI(struct ActionData *data);

BOOL InternalCreateConfig(void);
BOOL InternalCreateConfigGUI(struct ActionData *data);
BOOL WriteSysConfig(STRPTR name);
BOOL LoadConfigFromFile(STRPTR name);

IPTR Action_HW_Activate(struct IClass *cl, Object *obj, Msg msg);

AROS_UFP3(LONG, HardwareListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct HWListEntry *, hlnode, A1));

AROS_UFP3(LONG, PrefsListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct PrefsListEntry *, plnode, A1));

AROS_UFP3(LONG, DeviceListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct DevListEntry *, dlnode, A1));

AROS_UFP3(LONG, ClassListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct ClsListEntry *, clnode, A1));

AROS_UFP3(LONG, ErrorListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(struct ErrListEntry *, elnode, A1));

AROS_UFP3(LONG, IconListDisplayHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(char **, strarr, A2),
          AROS_UFPA(STRPTR, str, A1));

AROS_UFP3(IPTR, ActionDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));
                   
#endif
