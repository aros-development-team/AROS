#ifndef APPLICATION_INTERN_H
#define APPLICATION_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: application.library private state.
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <libraries/application.h>

#define APP_NAMEMAX     64
#define APP_IDENTMAX    200
#define APP_PATHMAX     512

#define APPF_UNIQUE         (1 << 0)
#define APPF_ICONIFY        (1 << 1)
#define APPF_PREFSWINDOW    (1 << 2)
#define APPF_NEWDOCS        (1 << 3)
#define APPF_PRINTDOCS      (1 << 4)
#define APPF_APPNOTIFY      (1 << 5)
#define APPF_BLANKNOTIFY    (1 << 6)
#define APPF_ALLOWBLANKER   (1 << 7)
#define APPF_SAVEPREFS      (1 << 8)
#define APPF_HIDDEN         (1 << 9)
#define APPF_GAMEMODE       (1 << 10)
#define APPF_NOICON         (1 << 11)
#define APPF_LEAVING        (1 << 12)   /* unregistering, waiting for icon locks */
#define APPF_NOCOUNTER      (1 << 13)   /* REGAPP_IdenticalNameCounter FALSE */

struct AppEntry
{
    struct MinNode   ae_Node;
    ULONG            ae_ID;
    ULONG            ae_Flags;
    ULONG            ae_Counter;    /* the _n of the identifier */
    struct MsgPort  *ae_Port;       /* created in, and owned by, ae_Owner */
    struct Task     *ae_Owner;
    STRPTR           ae_Description;
    STRPTR           ae_FileName;
    STRPTR           ae_PrefsName;  /* below ENV:, or a full path */
    PrefsObject     *ae_Prefs;      /* main prefs dictionary, or NULL */
    ULONG            ae_IconLocks;
    struct ApplicationIconInfo ae_Icon;
    TEXT             ae_Name[APP_NAMEMAX];
    TEXT             ae_URL[APP_NAMEMAX * 2];
    TEXT             ae_Ident[APP_IDENTMAX];
    TEXT             ae_BaseIdent[APP_IDENTMAX];  /* without the _n */
};

struct ApplicationBase
{
    struct Library          ab_Lib;
    struct SignalSemaphore  ab_Lock;
    struct MinList          ab_Apps;
    struct MinList          ab_Gone;    /* unregistered, icon still locked by a dock */
    ULONG                   ab_NextID;
    /*
     * Messages the library sends come back here when the receiver replies.
     * The port never signals; replied messages are freed on the next call.
     */
    struct MsgPort          ab_Reaper;
    ULONG                   ab_Outstanding;     /* sent, not yet freed */

    /* Global settings and the last used lists, ENV:Sys/application.xml */
    PrefsObject            *ab_Global;
    BOOL                    ab_GlobalLoaded;
    BOOL                    ab_GlobalStale;     /* ENVARC: behind ENV: */
    LONG                    ab_ArcMinute;       /* when ENVARC: was last written, -1 never */
    struct SignalSemaphore  ab_SaveLock;        /* one global save at a time */
    BOOL                    ab_BlankerAllowed;  /* as last broadcast */
};

extern struct DosLibrary *DOSBase;
extern struct UtilityBase *UtilityBase;

#define APPLIB_GLOBALPREFS  "Sys/application.xml"

/* Callers hold ab_Lock. */
struct AppEntry *app_Find(struct ApplicationBase *AppBase, ULONG appID);
void app_Reap(struct ApplicationBase *AppBase);
BOOL app_Post(struct ApplicationBase *AppBase, struct AppEntry *to, ULONG sender,
              ULONG type, CONST_STRPTR text);
void app_Broadcast(struct ApplicationBase *AppBase, ULONG sender, ULONG type);
void app_BroadcastTo(struct ApplicationBase *AppBase, ULONG flag, ULONG sender, ULONG type);
BOOL app_IsAlwaysBroadcast(ULONG type, ULONG *flag);
void app_UpdateBlanker(struct ApplicationBase *AppBase);
BOOL app_GameModeActive(struct ApplicationBase *AppBase);
void app_LoadGlobal(struct ApplicationBase *AppBase);
BOOL app_NoteAppStarted(struct ApplicationBase *AppBase, struct AppEntry *ae);
BOOL app_NoteDocOpened(struct ApplicationBase *AppBase, struct AppEntry *ae, CONST_STRPTR path);
PrefsObject *app_DocsOfApp(struct ApplicationBase *AppBase, struct AppEntry *ae);
BOOL app_ClearDocsOfApp(struct ApplicationBase *AppBase, struct AppEntry *ae);

void app_FreeEntry(struct AppEntry *ae);

/* Callers must not hold ab_Lock: these do the file I/O. */
void app_PreloadGlobal(struct ApplicationBase *AppBase);
void app_SaveGlobal(struct ApplicationBase *AppBase, BOOL envarc);
void app_SaveLists(struct ApplicationBase *AppBase, BOOL last);
STRPTR app_StrDup(CONST_STRPTR s);
BOOL app_IsProcess(void);

/* PrefsObjects, for use inside the library */
PrefsObject *po_NewDict(void);
PrefsObject *po_NewArray(void);
PrefsObject *po_NewString(CONST_STRPTR s);
PrefsObject *po_NewLong(LONG v);
PrefsObject *po_NewNow(void);
void po_Release(PrefsObject *obj);
PrefsObject *po_Copy(PrefsObject *obj);
ULONG po_Type(PrefsObject *obj);
LONG po_Count(PrefsObject *obj);
PrefsObject *po_At(PrefsObject *obj, LONG index);
BOOL po_ArrayInsert(PrefsObject *arr, PrefsObject *item, LONG index);
void po_ArrayRemove(PrefsObject *arr, LONG index);
void po_Clear(PrefsObject *obj);
BOOL po_DictSet(PrefsObject *dict, CONST_STRPTR key, PrefsObject *item);
PrefsObject *po_DictGet(PrefsObject *dict, CONST_STRPTR key);
CONST_STRPTR po_DictString(PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR def);
LONG po_DictLong(PrefsObject *dict, CONST_STRPTR key, LONG def);
BOOL po_MoveContents(PrefsObject *to, PrefsObject *from);

/* Serialization, in application_xml.c */
ULONG xml_Serialize(PrefsObject *obj, struct ALPOPutProcInfo *pi);
ULONG xml_Deserialize(PrefsObject *obj, struct ALPOGetProcInfo *pi);
ULONG xml_Begin(struct ALPOGetProcInfo *pi);
ULONG xml_ReadStream(PrefsObject *dict, struct ALPOGetProcInfo *pi);
ULONG xml_WriteStream(PrefsObject *dict, struct ALPOPutProcInfo *pi);
ULONG xml_ReadFile(PrefsObject *dict, CONST_STRPTR path);
ULONG xml_WriteFile(PrefsObject *dict, CONST_STRPTR path);
ULONG prefs_Read(struct ApplicationBase *AppBase, PrefsObject *dict, CONST_STRPTR name,
                 BOOL env, BOOL envarc);
ULONG prefs_Write(struct ApplicationBase *AppBase, PrefsObject *dict, CONST_STRPTR name,
                  BOOL env, BOOL envarc);

#endif /* APPLICATION_INTERN_H */
