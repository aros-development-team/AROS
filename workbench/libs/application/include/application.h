#ifndef LIBRARIES_APPLICATION_H
#define LIBRARIES_APPLICATION_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: application.library - registered applications, messages between
          them, pop-up notifications and PrefsObjects preferences.

    The names follow the AmigaOS 4 application.library, so its programs
    build unchanged. The tag and message type VALUES are AROS's own.
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <utility/tagitem.h>

#define APPLICATIONNAME "application.library"

/* An opaque PrefsObjects object; see the PrefsObjects section below. */
typedef ULONG PrefsObject;

/* struct ApplicationIconInfo.iconType */
#define APPICONT_None                   0   /* no icon, "invisible" application  */
#define APPICONT_ProgramIcon            1   /* the program's .info               */
#define APPICONT_CustomIcon             2   /* info.customIcon                   */
#define APPICONT_Docky                  3   /* info.dockyBase                    */

struct ApplicationIconInfo
{
    LONG iconType;
    union
    {
        struct DiskObject *customIcon;
        struct Library    *dockyBase;
        IPTR               reserved[8];
    } info;
};

/* RegisterApplication() */
#define REGAPP_Dummy                    (TAG_USER + 0x4A0000)
#define REGAPP_URLIdentifier            (REGAPP_Dummy + 1)  /* STRPTR: "supercoders.com"         */
#define REGAPP_WBStartup                (REGAPP_Dummy + 2)  /* struct WBStartup *                */
#define REGAPP_FileName                 (REGAPP_Dummy + 3)  /* STRPTR: program path              */
#define REGAPP_FileLock                 (REGAPP_Dummy + 4)  /* BPTR: lock on the program         */
#define REGAPP_Description              (REGAPP_Dummy + 5)  /* STRPTR, fixed once set            */
#define REGAPP_UniqueApplication        (REGAPP_Dummy + 6)  /* BOOL, fixed once set              */
#define REGAPP_HasIconifyFeature        (REGAPP_Dummy + 7)  /* BOOL                              */
#define REGAPP_HasPrefsWindow           (REGAPP_Dummy + 8)  /* BOOL                              */
#define REGAPP_CanCreateNewDocs         (REGAPP_Dummy + 9)  /* BOOL                              */
#define REGAPP_CanPrintDocs             (REGAPP_Dummy + 10) /* BOOL                              */
#define REGAPP_AppNotifications         (REGAPP_Dummy + 11) /* BOOL                              */
#define REGAPP_BlankerNotifications     (REGAPP_Dummy + 12) /* BOOL                              */
#define REGAPP_AllowsBlanker            (REGAPP_Dummy + 13) /* BOOL, default TRUE                */
#define REGAPP_SavePrefs                (REGAPP_Dummy + 14) /* BOOL: write prefs on unregister   */
#define REGAPP_LoadPrefs                (REGAPP_Dummy + 15) /* BOOL: read prefs on register      */
#define REGAPP_CustomPrefsFileName      (REGAPP_Dummy + 16) /* STRPTR: prefs file instead of     */
                                                            /*   ENV:[ENVDir/]ident.xml          */
#define REGAPP_ENVDir                   (REGAPP_Dummy + 17) /* STRPTR: drawer below ENV:/ENVARC: */
#define REGAPP_CustomPrefsBaseName      (REGAPP_Dummy + 18) /* STRPTR: base name instead of ident*/
#define REGAPP_NoIcon                   (REGAPP_Dummy + 19) /* BOOL                              */
#define REGAPP_Hidden                   (REGAPP_Dummy + 20) /* BOOL: starts iconified            */
#define REGAPP_AppIconInfo              (REGAPP_Dummy + 21) /* struct ApplicationIconInfo *      */
#define REGAPP_NeedsGameMode            (REGAPP_Dummy + 22) /* BOOL                              */
#define REGAPP_IdenticalNameCounter     (REGAPP_Dummy + 23) /* BOOL, default TRUE                */

/* UnregisterApplication() */
#define UNREGAPP_Dummy                  (TAG_USER + 0x4A0800)
#define UNREGAPP_WaitForUnlock          (UNREGAPP_Dummy + 1) /* BOOL, default TRUE */

/* Get/SetApplicationAttrs(). Pointer values are read into an APTR or
   STRPTR, everything else into a ULONG. */
#define APPATTR_Dummy                   (TAG_USER + 0x4A1000)
#define APPATTR_Port                    (APPATTR_Dummy + 1)  /* (G)  struct MsgPort *          */
#define APPATTR_Name                    (APPATTR_Dummy + 2)  /* (G)  STRPTR                    */
#define APPATTR_AppIdentifier           (APPATTR_Dummy + 3)  /* (G)  STRPTR                    */
#define APPATTR_URLIdentifier           (APPATTR_Dummy + 4)  /* (G)  STRPTR                    */
#define APPATTR_Description             (APPATTR_Dummy + 5)  /* (G)  STRPTR                    */
#define APPATTR_FileName                (APPATTR_Dummy + 6)  /* (G)  STRPTR                    */
#define APPATTR_UniqueApplication       (APPATTR_Dummy + 7)  /* (G)  BOOL                      */
#define APPATTR_Hidden                  (APPATTR_Dummy + 8)  /* (SG) BOOL                      */
#define APPATTR_HasIconifyFeature       (APPATTR_Dummy + 9)  /* (SG) BOOL                      */
#define APPATTR_HasPrefsWindow          (APPATTR_Dummy + 10) /* (SG) BOOL                      */
#define APPATTR_CanCreateNewDocs        (APPATTR_Dummy + 11) /* (SG) BOOL                      */
#define APPATTR_CanPrintDocs            (APPATTR_Dummy + 12) /* (SG) BOOL                      */
#define APPATTR_AppNotifications        (APPATTR_Dummy + 13) /* (SG) BOOL                      */
#define APPATTR_BlankerNotifications    (APPATTR_Dummy + 14) /* (SG) BOOL                      */
#define APPATTR_AllowsBlanker           (APPATTR_Dummy + 15) /* (SG) BOOL                      */
#define APPATTR_NeedsGameMode           (APPATTR_Dummy + 16) /* (SG) BOOL                      */
#define APPATTR_SavePrefs               (APPATTR_Dummy + 17) /* (SG) BOOL                      */
#define APPATTR_AppOpenedDocument       (APPATTR_Dummy + 18) /* (S)  STRPTR: document path     */
#define APPATTR_ClearLastUsedDocs       (APPATTR_Dummy + 19) /* (S)  BOOL                      */
#define APPATTR_LastUsedDocsArray       (APPATTR_Dummy + 20) /* (G)  PrefsObject *: a copy,    */
                                                             /*      ALPO_Release it          */
#define APPATTR_FlushPrefs              (APPATTR_Dummy + 21) /* (S)  write the prefs now       */
#define APPATTR_IconType                (APPATTR_Dummy + 22) /* (SG) struct ApplicationIconInfo * */
                                                             /*      (G only while locked)    */
#define APPATTR_MainPrefsDict           (APPATTR_Dummy + 23) /* (SG) PrefsObject *             */
#define APPATTR_AppIdentifierCounter    (APPATTR_Dummy + 24) /* (G)  ULONG: the _n, 0 = first  */
#define APPATTR_NoIcon                  (APPATTR_Dummy + 25) /* (G)  BOOL                      */

/* Get/SetAppLibAttrs() */
#define APPLIBATTR_Dummy                (TAG_USER + 0x4A1800)
#define APPLIBATTR_LastUsedAppsArray    (APPLIBATTR_Dummy + 1)  /* (G)  PrefsObject *: a copy */
#define APPLIBATTR_LastUsedAppsMaxCount (APPLIBATTR_Dummy + 2)  /* (SG) ULONG, 0..25          */
#define APPLIBATTR_ClearLastUsedApps    (APPLIBATTR_Dummy + 3)  /* (S)  BOOL                  */
#define APPLIBATTR_LastUsedDocsArray    (APPLIBATTR_Dummy + 4)  /* (G)  PrefsObject *: a copy */
#define APPLIBATTR_LastUsedDocsMaxCount (APPLIBATTR_Dummy + 5)  /* (SG) ULONG, 0..25          */
#define APPLIBATTR_ClearLastUsedDocs    (APPLIBATTR_Dummy + 6)  /* (S)  BOOL                  */
#define APPLIBATTR_BlankerIsAllowed     (APPLIBATTR_Dummy + 7)  /* (G)  BOOL                  */
#define APPLIBATTR_GameModeIsActive     (APPLIBATTR_Dummy + 8)  /* (G)  BOOL                  */

#define APPLIB_LASTUSED_MAX             25

/* Keys of the dictionaries in the last used applications/documents arrays */
#define LUA_BASE_KEY                    "LastUsedApplications"
#define LUA_OPENDATE_KEY                "openDate"
#define LUA_APPIDENTIFIER_KEY           "appIdentifier"
#define LUA_APPNAME_KEY                 "appName"
#define LUA_APPPATH_KEY                 "appPath"
#define LUD_BASE_KEY                    "LastUsedDocuments"
#define LUD_OPENDATE_KEY                "openDate"
#define LUD_DOCPATH_KEY                 "docPath"
#define LUD_APPIDENTIFIER_KEY           "appIdentifier"
#define LUD_APPPATH_KEY                 "appPath"

/* FindApplication() */
#define FINDAPP_Dummy                   (TAG_USER + 0x4A2000)
#define FINDAPP_Name                    (FINDAPP_Dummy + 1)  /* "AudioMonster"                   */
#define FINDAPP_AppIdentifier           (FINDAPP_Dummy + 2)  /* "AudioMonster_1.supercoders.com" */
#define FINDAPP_FileName                (FINDAPP_Dummy + 3)  /* "Work:Utils/AudioMonster"        */

/* Notify() */
#define APPNOTIFY_Dummy                 (TAG_USER + 0x4A3000)
#define APPNOTIFY_Title                 (APPNOTIFY_Dummy + 1)  /* STRPTR, up to 64 chars          */
#define APPNOTIFY_Text                  (APPNOTIFY_Dummy + 2)  /* STRPTR, up to 160 chars         */
#define APPNOTIFY_PubScreenName         (APPNOTIFY_Dummy + 3)  /* STRPTR, "FRONT" = frontmost     */
#define APPNOTIFY_ImageFile             (APPNOTIFY_Dummy + 4)  /* STRPTR, full path, 32x32        */
#define APPNOTIFY_BackMsg               (APPNOTIFY_Dummy + 5)  /* STRPTR, "URL:..." opens it      */
#define APPNOTIFY_CloseOnDC             (APPNOTIFY_Dummy + 6)  /* BOOL                            */
#define APPNOTIFY_ImageVertAlign        (APPNOTIFY_Dummy + 7)  /* ULONG 0 top, 1 middle, 2 bottom */
#define APPNOTIFY_Update                (APPNOTIFY_Dummy + 8)  /* BOOL: replace this app's others */
#define APPNOTIFY_Pri                   (APPNOTIFY_Dummy + 9)  /* ULONG 0..10: >8 beeps, 10 stays */
#define APPNOTIFY_LogOnly               (APPNOTIFY_Dummy + 10) /* BOOL: log, show nothing         */
#define APPNOTIFY_SoundFile             (APPNOTIFY_Dummy + 11) /* STRPTR: sound to play           */
#define APPNOTIFY_ConfirmClose          (APPNOTIFY_Dummy + 12) /* BOOL: APPLIBMT_ConfirmClose     */
                                                               /*   when the pop-up closes       */
#define APPNOTIFY_IconFile              (APPNOTIFY_Dummy + 13) /* STRPTR: icon, if no ImageFile   */
/* AROS: take back this application's pop-ups whose BackMsg is this string,
   shown or waiting; nothing new is shown. */
#define APPNOTIFY_Withdraw              (APPNOTIFY_Dummy + 14) /* STRPTR                          */
/* AROS: bars under the text, one per line: "<percent>;#RRGGBB;<label>", such
   as a printer's ink levels or a download's progress. At most
   APPNOTIFY_GAUGES_MAX. */
#define APPNOTIFY_Gauges                (APPNOTIFY_Dummy + 15) /* STRPTR                          */

#define APPNOTIFY_GAUGES_MAX            8

#define APPNOTIFY_TITLE_MAX             64
#define APPNOTIFY_TEXT_MAX              160

/* Notify() results */
#define APPNOTIFY_OK_MSGQUEUED                  0
#define APPNOTIFY_OK_APPREGISTERED              1
#define APPNOTIFY_ERROR_OUTOFMEM                10
#define APPNOTIFY_ERROR_TOOMANYMESSAGESQUEUED   20
#define APPNOTIFY_ERROR_APPALREADYREGISTERED    30
#define APPNOTIFY_ERROR_NOAPPNAME               40
#define APPNOTIFY_ERROR_APPNOTALLOWED           50
#define APPNOTIFY_ERROR_APPNOTREGISTERED        60
#define APPNOTIFY_ERROR_GAMEMODEACTIVE          70
#define APPNOTIFY_ERROR_TAGS                    100
#define APPNOTIFY_ERROR_TIMEOUT                 110
#define APPNOTIFY_ERROR_SERVERNOTRUNNING        120
#define APPNOTIFY_ERROR_GENERIC                 300

/* Message types, in struct ApplicationMsg.type */
#define APPLIBMT_Quit                   1
#define APPLIBMT_ForceQuit              2   /* quit without asking to save           */
#define APPLIBMT_Hide                   3
#define APPLIBMT_Unhide                 4
#define APPLIBMT_ToFront                5
#define APPLIBMT_OpenPrefs              6
#define APPLIBMT_ReloadPrefs            7
#define APPLIBMT_NewBlankDoc            8
#define APPLIBMT_OpenDoc                9   /* struct ApplicationOpenPrintDocMsg     */
#define APPLIBMT_PrintDoc               10  /* struct ApplicationOpenPrintDocMsg     */
#define APPLIBMT_Unique                 11
#define APPLIBMT_GameModeEntered        12  /* to every application                  */
#define APPLIBMT_GameModeLeft           13  /* to every application                  */
#define APPLIBMT_CustomMsg              14  /* struct ApplicationCustomMsg           */
#define APPLIBMT_ConfirmClose           15  /* a pop-up with APPNOTIFY_ConfirmClose  */
                                            /*   closed. AROS: a struct              */
                                            /*   ApplicationCustomMsg, customMsg its */
                                            /*   BackMsg or NULL                     */
/* To applications with AppNotifications set; senderAppID is the application
   concerned, or 0 for LastAppDocChange. */
#define APPLIBMT_AppRegister            20
#define APPLIBMT_AppUnregister          21  /* unlock its icon now                   */
#define APPLIBMT_AppWantsChangeIconType 22  /* unlock its icon now                   */
#define APPLIBMT_AppDidChangeIconType   23
#define APPLIBMT_AppDidChangeHidden     24
#define APPLIBMT_LastAppDocChange       25
/* To applications with BlankerNotifications set */
#define APPLIBMT_BlankerAllow           30
#define APPLIBMT_BlankerDisallow        31  /* stop blanking now, and stay off       */
#define APPLIBMT_BlankerBlank           32
#define APPLIBMT_BlankerMonitorOff      33
#define APPLIBMT_BlankerUnBlank         34

/*
 * Messages arrive on the port from APPATTR_Port. Reply every one with
 * ReplyMsg(); the library frees it then, so copy anything to be kept.
 */
struct ApplicationMsg
{
    struct Message msg;
    ULONG          senderAppID;     /* 0 = the system */
    ULONG          type;
};

struct ApplicationOpenPrintDocMsg
{
    struct ApplicationMsg almsg;
    STRPTR                fileName;
};

struct ApplicationCustomMsg
{
    struct ApplicationMsg almsg;
    STRPTR                customMsg;
};

/* GetApplicationList() entries */
struct ApplicationNode
{
    struct MinNode node;
    ULONG          appID;
    STRPTR         name;            /* "AudioMonster"                       */
    /* AROS extensions */
    STRPTR         identifier;      /* "AudioMonster_1.supercoders.com"     */
    STRPTR         description;     /* or NULL                              */
    STRPTR         fileName;        /* or NULL                              */
    BOOL           hidden;
};

/***************************************************************************
 * PrefsObjects
 *
 * Every Prefs<Type>A(obj, error, tags) call runs the methods in tags on
 * obj in order and returns obj, or the object an Alloc or Copy method made.
 * *error, if given, gets the ALPOEF_ flags of what failed (0 = all fine).
 * Objects are reference counted: Alloc and Copy give a count of 1,
 * ALPO_Retain adds one and ALPO_Release frees the object at 0. Arrays and
 * dictionaries own what is put in them and release it when it is removed.
 ***************************************************************************/

#define TAGBUFFER_SIZE  150

#define ALPOENC_ISO8859 0
#define ALPOENC_UTF8    1

/* For ALPO_Deserialize, BeginDeserialization() and READPREFS_GetProcInfo */
struct ALPOGetProcInfo
{
    LONG   (*getChProc)(struct ALPOGetProcInfo *pi);         /* -1 at the end */
    LONG   (*unGetChProc)(LONG c, struct ALPOGetProcInfo *pi);
    APTR   basePtr;
    APTR   handlePtr;
    STRPTR key_otag;        /* TAGBUFFER_SIZE bytes, for the library */
    ULONG  encoding;
};

/* For ALPO_Serialize and WRITEPREFS_PutProcInfo */
struct ALPOPutProcInfo
{
    void   (*putChProc)(LONG c, struct ALPOPutProcInfo *pi);
    APTR   basePtr;
    APTR   handlePtr;
    LONG   tabDepth;        /* set to 0 */
    ULONG  encoding;
};

struct ALPOString
{
    STRPTR string;
    ULONG  length;
};

/* ALPO_Identify */
#define ALPOT_None          0
#define ALPOT_String        1
#define ALPOT_Number        2
#define ALPOT_Date          3
#define ALPOT_Binary        4
#define ALPOT_Array         5
#define ALPOT_Dictionary    6

/* *error */
#define ALPOEF_INTERNAL_ERROR       0x00000001
#define ALPOEF_UNKNOWN_METHOD       0x00000002
#define ALPOEF_MISSING_OBJECT       0x00000004
#define ALPOEF_IMPOSSIBLE_OP        0x00000008
#define ALPOEF_WRONG_OBJECTTYPE     0x00000010
#define ALPOEF_ILLEGAL_PARAMETER    0x00000020
#define ALPOEF_MEMORY               0x00000040
#define ALPOEF_NOTFOUND             0x00000080
#define ALPOEF_DUPLICATE            0x00000100
#define ALPOEF_DESERIALIZATION      0x00000200
#define ALPOEF_READTAG              0x00000400
#define ALPOEF_READDATA             0x00000800
#define ALPOEF_WRONG_OTAG           0x00001000
#define ALPOEF_WRONG_CTAG           0x00002000
#define ALPOEF_WRONG_SERDATA        0x00004000
#define ALPOEF_FILEERROR            0x00008000
#define ALPOEF_XMLERROR             0x00010000
#define ALPOEF_WRONG_OBJECT         0x00020000

#define ALPO_Dummy                  (TAG_USER + 0x4B0000)

/* Every object type */
#define ALPO_Alloc                  (ALPO_Dummy + 1)     /* -                         */
#define ALPO_AllocWithString        (ALPO_Dummy + 2)     /* STRPTR (string, number)   */
#define ALPO_Release                (ALPO_Dummy + 2000)  /* -                         */
#define ALPO_Copy                   (ALPO_Dummy + 2001)  /* -: a deep copy            */
#define ALPO_Retain                 (ALPO_Dummy + 2002)  /* -                         */
#define ALPO_Identify               (ALPO_Dummy + 20000) /* ULONG *: ALPOT_           */
#define ALPO_Serialize              (ALPO_Dummy + 20001) /* struct ALPOPutProcInfo *  */
#define ALPO_Deserialize            (ALPO_Dummy + 20002) /* struct ALPOGetProcInfo *  */
#define ALPO_GetRetainCount         (ALPO_Dummy + 20003) /* LONG *                    */
#define ALPO_SetWithString          (ALPO_Dummy + 20004) /* STRPTR (string, number)   */
#define ALPO_GetAsString            (ALPO_Dummy + 20005) /* struct ALPOString *: the  */
                                                         /*   caller's buffer         */
/* PrefsString: 8-bit strings */
struct ALPOUniString
{
    ULONG *string;              /* UCS-4 */
    ULONG  length;
};

#define ALPOSTR_AllocSetString      (ALPO_Dummy + 30000) /* STRPTR                    */
#define ALPOSTR_AllocSetUniString   (ALPO_Dummy + 30001) /* struct ALPOUniString *    */
#define ALPOSTR_Clear               (ALPO_Dummy + 10000) /* -                         */
#define ALPOSTR_GetLength           (ALPO_Dummy + 40000) /* LONG *                    */
#define ALPOSTR_SetString           (ALPO_Dummy + 40001) /* STRPTR                    */
#define ALPOSTR_GetString           (ALPO_Dummy + 40002) /* STRPTR *                  */
#define ALPOSTR_SetUniString        (ALPO_Dummy + 40003) /* struct ALPOUniString *    */
#define ALPOSTR_GetUniString        (ALPO_Dummy + 40004) /* struct ALPOUniString *:   */
                                                         /*   the caller's buffer     */
/* PrefsNumber */
#define ALPONUMT_None               0
#define ALPONUMT_Long               1
#define ALPONUMT_Bool               2
#define ALPONUMT_Double             3

#define ALPONUM_AllocSetLong        (ALPO_Dummy + 30000) /* LONG                      */
#define ALPONUM_AllocSetBool        (ALPO_Dummy + 30001) /* BOOL                      */
#define ALPONUM_AllocSetDouble      (ALPO_Dummy + 30002) /* double *                  */
#define ALPONUM_GetType             (ALPO_Dummy + 40000) /* LONG *: ALPONUMT_         */
#define ALPONUM_ConvertToType       (ALPO_Dummy + 40030) /* LONG: ALPONUMT_           */
#define ALPONUM_SetLong             (ALPO_Dummy + 40001) /* LONG                      */
#define ALPONUM_GetLong             (ALPO_Dummy + 40002) /* LONG *                    */
#define ALPONUM_SetBool             (ALPO_Dummy + 40011) /* BOOL                      */
#define ALPONUM_GetBool             (ALPO_Dummy + 40012) /* BOOL *                    */
#define ALPONUM_SetDouble           (ALPO_Dummy + 40021) /* double *                  */
#define ALPONUM_GetDouble           (ALPO_Dummy + 40022) /* double *                  */

/* PrefsDate */
#define ALPODAT_AllocSetTimeVal     (ALPO_Dummy + 30000) /* struct timeval *          */
#define ALPODAT_AllocSetCTime       (ALPO_Dummy + 30001) /* ULONG: secs since 1970    */
#define ALPODAT_SetCurrentTime      (ALPO_Dummy + 10000) /* -                         */
#define ALPODAT_SetTimeVal          (ALPO_Dummy + 40000) /* struct timeval *          */
#define ALPODAT_GetTimeVal          (ALPO_Dummy + 40001) /* struct timeval *          */
#define ALPODAT_SetSecsSince1978    (ALPO_Dummy + 40002) /* ULONG                     */
#define ALPODAT_GetSecsSince1978    (ALPO_Dummy + 40003) /* ULONG *                   */
#define ALPODAT_SetClockData        (ALPO_Dummy + 40004) /* struct ClockData *        */
#define ALPODAT_GetClockData        (ALPO_Dummy + 40005) /* struct ClockData *        */
#define ALPODAT_SetDateStamp        (ALPO_Dummy + 40006) /* struct DateStamp *        */
#define ALPODAT_GetDateStamp        (ALPO_Dummy + 40007) /* struct DateStamp *        */

/* PrefsBinary */
struct ALPOBinData
{
    UBYTE *data;
    LONG   size;
};

#define ALPOBIN_AllocWithSize       (ALPO_Dummy + 30000) /* LONG: zeroed              */
#define ALPOBIN_AllocWithData       (ALPO_Dummy + 30001) /* struct ALPOBinData *      */
#define ALPOBIN_ClearData           (ALPO_Dummy + 10000) /* -                         */
#define ALPOBIN_GetData             (ALPO_Dummy + 40001) /* UBYTE **                  */
#define ALPOBIN_GetDataStruct       (ALPO_Dummy + 40002) /* struct ALPOBinData *      */
#define ALPOBIN_GetSize             (ALPO_Dummy + 40003) /* LONG *                    */
#define ALPOBIN_SetData             (ALPO_Dummy + 40004) /* struct ALPOBinData *      */

/* PrefsDictionary and PrefsArray */
struct ALPOObjIndex
{
    PrefsObject *obj;
    LONG         index;
};

struct ALPOObjKey
{
    PrefsObject *obj;
    STRPTR       key;
};

struct ALPOObjKeyIndex
{
    PrefsObject *obj;
    STRPTR       key;
    LONG         index;
};

#define ALPODICT_Clear                  (ALPO_Dummy + 10000) /* -                        */
#define ALPODICT_SortAscending          (ALPO_Dummy + 10001) /* -: by key                */
#define ALPODICT_SortDescending         (ALPO_Dummy + 10002) /* -                        */
#define ALPODICT_GetCount               (ALPO_Dummy + 40000) /* LONG *                   */
#define ALPODICT_SetObjForKey           (ALPO_Dummy + 40010) /* struct ALPOObjKey *      */
#define ALPODICT_GetObjForKey           (ALPO_Dummy + 40011) /* struct ALPOObjKey *      */
#define ALPODICT_RemoveObjForKey        (ALPO_Dummy + 40012) /* STRPTR                   */
#define ALPODICT_ReplaceObjForKey       (ALPO_Dummy + 40013) /* struct ALPOObjKey *      */
#define ALPODICT_GetObjAndIndexForKey   (ALPO_Dummy + 40014) /* struct ALPOObjKeyIndex * */
#define ALPODICT_GetObjAtIndex          (ALPO_Dummy + 40020) /* struct ALPOObjIndex *    */
#define ALPODICT_RemoveObjAtIndex       (ALPO_Dummy + 40021) /* LONG                     */
#define ALPODICT_ReplaceObjAtIndex      (ALPO_Dummy + 40022) /* struct ALPOObjIndex *    */
#define ALPODICT_GetObjAndKeyAtIndex    (ALPO_Dummy + 40023) /* struct ALPOObjKeyIndex * */
#define ALPODICT_InsertObjAtIndex       (ALPO_Dummy + 40024) /* struct ALPOObjKeyIndex * */
#define ALPODICT_RemoveObj              (ALPO_Dummy + 40030) /* PrefsObject *            */
#define ALPODICT_GetKeyAndIndexForObj   (ALPO_Dummy + 40031) /* struct ALPOObjKeyIndex * */

#define ALPOARR_Clear                   (ALPO_Dummy + 10000) /* -                        */
#define ALPOARR_GetCount                (ALPO_Dummy + 40001) /* LONG *                   */
#define ALPOARR_AddObj                  (ALPO_Dummy + 40010) /* PrefsObject *            */
#define ALPOARR_InsertObjAtIndex        (ALPO_Dummy + 40011) /* struct ALPOObjIndex *    */
#define ALPOARR_GetObjAtIndex           (ALPO_Dummy + 40012) /* struct ALPOObjIndex *    */
#define ALPOARR_RemoveObjAtIndex        (ALPO_Dummy + 40013) /* LONG                     */
#define ALPOARR_ReplaceObjAtIndex       (ALPO_Dummy + 40014) /* struct ALPOObjIndex *    */
#define ALPOARR_RemoveObj               (ALPO_Dummy + 40015) /* PrefsObject *            */
#define ALPOARR_GetIndexForObj          (ALPO_Dummy + 40016) /* struct ALPOObjIndex *    */

/* ReadPrefs(). The file is ENV:name, or ENVARC:name if that fails. */
#define READPREFS_Dummy                 (TAG_USER + 0x4B8000)
#define READPREFS_AppID                 (READPREFS_Dummy + 1)  /* ULONG: that app's file   */
#define READPREFS_FileName              (READPREFS_Dummy + 2)  /* STRPTR: name below ENV:, */
                                                               /*   or a full path         */
#define READPREFS_GetProcInfo           (READPREFS_Dummy + 3)  /* struct ALPOGetProcInfo * */
#define READPREFS_ReadENV               (READPREFS_Dummy + 10) /* BOOL, default TRUE       */
#define READPREFS_ReadENVARC            (READPREFS_Dummy + 11) /* BOOL, default TRUE       */

/* WritePrefs() */
#define WRITEPREFS_Dummy                (TAG_USER + 0x4B8800)
#define WRITEPREFS_AppID                (WRITEPREFS_Dummy + 1)  /* ULONG                   */
#define WRITEPREFS_FileName             (WRITEPREFS_Dummy + 2)  /* STRPTR                  */
#define WRITEPREFS_PutProcInfo          (WRITEPREFS_Dummy + 3)  /* struct ALPOPutProcInfo *  */
#define WRITEPREFS_WriteENV             (WRITEPREFS_Dummy + 10) /* BOOL, default TRUE      */
#define WRITEPREFS_WriteENVARC          (WRITEPREFS_Dummy + 11) /* BOOL, default FALSE     */

#endif /* LIBRARIES_APPLICATION_H */
