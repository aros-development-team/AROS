/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Registering applications and messaging between them.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include "application_po.h"
#include LC_LIBDEFS_FILE

/* Delay(2) steps while a dock still holds the icon lock: five seconds */
#define APP_UNLOCK_TRIES 125

static void app_CopyStr(STRPTR dst, CONST_STRPTR src, ULONG size)
{
    if (!src)
        src = "";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

/* The program's path, from whichever of the three tags the caller gave. */
static STRPTR app_ProgramPath(struct TagItem *tags)
{
    struct WBStartup *wbs;
    STRPTR path;
    BPTR lock;
    CONST_STRPTR name = NULL;

    if ((name = (CONST_STRPTR)GetTagData(REGAPP_FileName, 0, tags)))
        return app_StrDup(name);

    if (!(path = AllocVec(1024, MEMF_ANY | MEMF_CLEAR)))
        return NULL;

    if ((lock = (BPTR)GetTagData(REGAPP_FileLock, 0, tags)))
    {
        if (NameFromLock(lock, path, 1024))
            return path;
    }
    else if ((wbs = (struct WBStartup *)GetTagData(REGAPP_WBStartup, 0, tags))
             && wbs->sm_NumArgs > 0)
    {
        if (NameFromLock(wbs->sm_ArgList[0].wa_Lock, path, 1024)
            && AddPart(path, wbs->sm_ArgList[0].wa_Name, 1024))
            return path;
    }
    else
    {
        struct Process *me = (struct Process *)FindTask(NULL);
        TEXT prog[256];

        if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS && me->pr_HomeDir
            && GetProgramName(prog, sizeof(prog))
            && NameFromLock(me->pr_HomeDir, path, 1024)
            && AddPart(path, FilePart(prog), 1024))
            return path;
    }

    FreeVec(path);
    return NULL;
}


static BOOL app_SameApp(struct AppEntry *ae, CONST_STRPTR name, CONST_STRPTR url)
{
    return (Stricmp(ae->ae_Name, name) == 0) && (Stricmp(ae->ae_URL, url) == 0);
}

static BOOL app_IdentTaken(struct ApplicationBase *AppBase, CONST_STRPTR ident)
{
    struct AppEntry *ae;

    ForeachNode(&AppBase->ab_Apps, ae)
    {
        if (Stricmp(ae->ae_Ident, ident) == 0)
            return TRUE;
    }
    return FALSE;
}

static void app_BuildIdent(STRPTR dst, struct AppEntry *ae, ULONG n)
{
    TEXT inst[16], *p = inst + sizeof(inst) - 1;
    ULONG v = n;

    *p = '\0';
    if (n)
    {
        do
            *--p = '0' + (v % 10);
        while (v /= 10);
        *--p = '_';
    }
    strcpy(dst, ae->ae_Name);
    strcat(dst, p);
    if (ae->ae_URL[0])
    {
        strcat(dst, ".");
        strncat(dst, ae->ae_URL, APP_IDENTMAX - strlen(dst) - 1);
    }
}

/* "name.url", then "name_1.url", "name_2.url", ... for later instances. */
static BOOL app_MakeIdent(struct ApplicationBase *AppBase, struct AppEntry *ae)
{
    ULONG n;

    app_BuildIdent(ae->ae_BaseIdent, ae, 0);
    for (n = 0; ; n++)
    {
        app_BuildIdent(ae->ae_Ident, ae, n);
        if (!app_IdentTaken(AppBase, ae->ae_Ident))
        {
            ae->ae_Counter = n;
            return TRUE;
        }
        if (ae->ae_Flags & APPF_NOCOUNTER)
            return FALSE;
    }
}

/* [ENVDir/]basename.xml below ENV:, or the custom file name. */
static STRPTR app_PrefsName(struct AppEntry *ae, struct TagItem *tags)
{
    CONST_STRPTR custom = (CONST_STRPTR)GetTagData(REGAPP_CustomPrefsFileName, 0, tags);
    CONST_STRPTR dir = (CONST_STRPTR)GetTagData(REGAPP_ENVDir, 0, tags);
    CONST_STRPTR base = (CONST_STRPTR)GetTagData(REGAPP_CustomPrefsBaseName, 0, tags);
    STRPTR name;

    if (custom)
        return app_StrDup(custom);
    if (!base)
        base = ae->ae_BaseIdent;
    if (!(name = AllocVec(APP_PATHMAX, MEMF_ANY | MEMF_CLEAR)))
        return NULL;
    if (dir && *dir)
    {
        strncpy(name, dir, APP_PATHMAX - 1);
        if (name[strlen(name) - 1] != '/' && name[strlen(name) - 1] != ':')
            strncat(name, "/", APP_PATHMAX - strlen(name) - 1);
    }
    strncat(name, base, APP_PATHMAX - strlen(name) - 1);
    strncat(name, ".xml", APP_PATHMAX - strlen(name) - 1);
    return name;
}

static ULONG app_FlagFor(Tag tag)
{
    switch (tag)
    {
    case REGAPP_UniqueApplication:      return APPF_UNIQUE;
    case REGAPP_HasIconifyFeature:
    case APPATTR_HasIconifyFeature:     return APPF_ICONIFY;
    case REGAPP_HasPrefsWindow:
    case APPATTR_HasPrefsWindow:        return APPF_PREFSWINDOW;
    case REGAPP_CanCreateNewDocs:
    case APPATTR_CanCreateNewDocs:      return APPF_NEWDOCS;
    case REGAPP_CanPrintDocs:
    case APPATTR_CanPrintDocs:          return APPF_PRINTDOCS;
    case REGAPP_AppNotifications:
    case APPATTR_AppNotifications:      return APPF_APPNOTIFY;
    case REGAPP_BlankerNotifications:
    case APPATTR_BlankerNotifications:  return APPF_BLANKNOTIFY;
    case REGAPP_AllowsBlanker:
    case APPATTR_AllowsBlanker:         return APPF_ALLOWBLANKER;
    case REGAPP_SavePrefs:
    case APPATTR_SavePrefs:             return APPF_SAVEPREFS;
    case REGAPP_Hidden:
    case APPATTR_Hidden:                return APPF_HIDDEN;
    case REGAPP_NeedsGameMode:
    case APPATTR_NeedsGameMode:         return APPF_GAMEMODE;
    case REGAPP_NoIcon:
    case APPATTR_NoIcon:                return APPF_NOICON;
    }
    return 0;
}

void app_FreeEntry(struct AppEntry *ae)
{
    po_Release(ae->ae_Prefs);
    FreeVec(ae->ae_PrefsName);
    FreeVec(ae->ae_Description);
    FreeVec(ae->ae_FileName);
    FreeVec(ae);
}

/*****************************************************************************

    NAME */
#include <proto/application.h>

        AROS_LH2(ULONG, RegisterApplicationA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 5, Application)

/*  FUNCTION
        Register the calling program as an application. It gets an ID, a
        unique identifier ("name.urlidentifier", with "_1", "_2" ... added
        for further instances) and a message port, see APPATTR_Port.

    INPUTS
        name - application name, without spaces.
        tags - REGAPP_ tags. With REGAPP_LoadPrefs the application's prefs
               file, ENV:[ENVDir/]name.urlidentifier.xml (or ENVARC: if
               that is missing), is read into APPATTR_MainPrefsDict.

    RESULT
        The application ID, or 0. With REGAPP_UniqueApplication, 0 is also
        returned when the application is already running; that instance is
        sent an APPLIBMT_Unique message.

    NOTES
        Call UnregisterApplication() from the same task before exiting.

    SEE ALSO
        UnregisterApplicationA(), GetApplicationAttrsA()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate = tags, *tag;
    struct ApplicationIconInfo *aii;
    struct AppEntry *ae, *other;
    CONST_STRPTR url;
    BOOL loadprefs, noted;
    ULONG id = 0;

    if (!name || !*name || strchr(name, ' ') || strlen(name) >= APP_NAMEMAX)
        return 0;

    if (!(ae = AllocVec(sizeof(struct AppEntry), MEMF_ANY | MEMF_CLEAR)))
        return 0;

    app_CopyStr(ae->ae_Name, name, sizeof(ae->ae_Name));
    url = (CONST_STRPTR)GetTagData(REGAPP_URLIdentifier, (IPTR)"", tags);
    if (!url)
        url = "";
    if (Strnicmp(url, "www.", 4) == 0)
        url += 4;
    app_CopyStr(ae->ae_URL, url, sizeof(ae->ae_URL));

    ae->ae_Flags = APPF_ALLOWBLANKER;
    while ((tag = NextTagItem(&tstate)))
    {
        ULONG flag = app_FlagFor(tag->ti_Tag);

        if (flag)
        {
            if (tag->ti_Data)
                ae->ae_Flags |= flag;
            else
                ae->ae_Flags &= ~flag;
        }
    }
    if (!GetTagData(REGAPP_IdenticalNameCounter, TRUE, tags))
        ae->ae_Flags |= APPF_NOCOUNTER;

    ae->ae_Icon.iconType = APPICONT_ProgramIcon;
    if ((aii = (struct ApplicationIconInfo *)GetTagData(REGAPP_AppIconInfo, 0, tags)))
        ae->ae_Icon = *aii;
    if (ae->ae_Flags & APPF_NOICON)
        ae->ae_Icon.iconType = APPICONT_None;

    loadprefs = GetTagData(REGAPP_LoadPrefs, FALSE, tags)
                || GetTagData(REGAPP_CustomPrefsFileName, 0, tags)
                || GetTagData(REGAPP_CustomPrefsBaseName, 0, tags);

    ae->ae_Description = app_StrDup((CONST_STRPTR)GetTagData(REGAPP_Description, 0, tags));
    ae->ae_FileName = app_ProgramPath(tags);
    ae->ae_Owner = FindTask(NULL);

    if (!(ae->ae_Port = CreateMsgPort()))
        goto fail;

    /* The prefs file name depends on the base identifier only, so the file
       is read before the lock is taken. */
    app_BuildIdent(ae->ae_BaseIdent, ae, 0);
    if (!(ae->ae_PrefsName = app_PrefsName(ae, tags)))
        goto fail;
    if (loadprefs && (ae->ae_Prefs = po_NewDict()))
        prefs_Read(AppBase, ae->ae_Prefs, ae->ae_PrefsName, TRUE, TRUE);
    if (!(ae->ae_Flags & APPF_NOICON))
        app_PreloadGlobal(AppBase);

    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);

    ForeachNode(&AppBase->ab_Apps, other)
    {
        if (app_SameApp(other, ae->ae_Name, ae->ae_URL)
            && ((other->ae_Flags | ae->ae_Flags) & APPF_UNIQUE))
        {
            app_Post(AppBase, other, 0, APPLIBMT_Unique, NULL);
            ReleaseSemaphore(&AppBase->ab_Lock);
            goto fail;
        }
    }

    if (!app_MakeIdent(AppBase, ae))
    {
        ReleaseSemaphore(&AppBase->ab_Lock);
        goto fail;
    }

    ae->ae_ID = id = AppBase->ab_NextID++;
    if (!AppBase->ab_NextID)
        AppBase->ab_NextID = 1;
    ADDTAIL((struct List *)&AppBase->ab_Apps, ae);

    app_BroadcastTo(AppBase, APPF_APPNOTIFY, id, APPLIBMT_AppRegister);
    if (ae->ae_Flags & APPF_GAMEMODE)
        app_Broadcast(AppBase, id, APPLIBMT_GameModeEntered);
    app_UpdateBlanker(AppBase);
    noted = !(ae->ae_Flags & APPF_NOICON) && app_NoteAppStarted(AppBase, ae);

    ReleaseSemaphore(&AppBase->ab_Lock);

    if (noted)
        app_SaveLists(AppBase);

    D(bug("[application] registered %s as %lu\n", ae->ae_Ident, (unsigned long)id));
    return id;

fail:
    if (ae->ae_Port)
        DeleteMsgPort(ae->ae_Port);
    app_FreeEntry(ae);
    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, UnregisterApplicationA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 6, Application)

/*  FUNCTION
        Remove an application registered with RegisterApplication().
        Messages still waiting on its port are replied, game mode and a
        blanker veto are undone, and with REGAPP_SavePrefs the prefs are
        written to ENV: and ENVARC:.

        Docks that locked the application's icon are sent
        APPLIBMT_AppUnregister; until they unlock it, or for at most five
        seconds, the application stays. After that it is gone, but the
        icon information a dock read stays valid until its last
        UnlockApplicationIcon().

    INPUTS
        appID - the application ID.
        tags  - UNREGAPP_WaitForUnlock: FALSE to fail at once while the
                icon is locked, instead of waiting. NULL is fine.

    RESULT
        FALSE if there was no such application, or its icon is locked and
        UNREGAPP_WaitForUnlock is FALSE.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL wait = GetTagData(UNREGAPP_WaitForUnlock, TRUE, tags);
    struct AppEntry *ae;
    struct Message *msg;
    PrefsObject *prefs = NULL;
    STRPTR prefsname = NULL;
    BOOL gone, arc;
    LONG tries;

    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    if (!(ae = app_Find(AppBase, appID)) || (ae->ae_Flags & APPF_LEAVING))
    {
        ReleaseSemaphore(&AppBase->ab_Lock);
        return FALSE;
    }

    if (ae->ae_IconLocks && !wait)
    {
        app_BroadcastTo(AppBase, APPF_APPNOTIFY, appID, APPLIBMT_AppWantsChangeIconType);
        ReleaseSemaphore(&AppBase->ab_Lock);
        return FALSE;
    }

    ae->ae_Flags |= APPF_LEAVING;
    app_BroadcastTo(AppBase, APPF_APPNOTIFY, appID, APPLIBMT_AppUnregister);
    for (tries = 0; ae->ae_IconLocks && tries < APP_UNLOCK_TRIES; tries++)
    {
        ReleaseSemaphore(&AppBase->ab_Lock);
        Delay(2);
        ObtainSemaphore(&AppBase->ab_Lock);
        app_Reap(AppBase);
    }
    REMOVE(ae);
    /* A dock that still holds the icon lock may be reading the icon
       information: the entry stays until its last unlock. */
    gone = ae->ae_IconLocks != 0;
    if (gone)
        ADDTAIL((struct List *)&AppBase->ab_Gone, ae);

    if (ae->ae_Flags & APPF_GAMEMODE)
        app_Broadcast(AppBase, ae->ae_ID, APPLIBMT_GameModeLeft);
    app_UpdateBlanker(AppBase);

    while ((msg = GetMsg(ae->ae_Port)))
        ReplyMsg(msg);
    app_Reap(AppBase);

    /*
     * The port's signal belongs to the task that registered. From any other
     * task, freeing it would release that task's signal bit here instead,
     * so the port is left allocated.
     */
    if (FindTask(NULL) == ae->ae_Owner)
        DeleteMsgPort(ae->ae_Port);
    ae->ae_Port = NULL;

    /* Taken over: nothing else reaches the entry, a dock reads only the icon. */
    if ((ae->ae_Flags & APPF_SAVEPREFS) && ae->ae_Prefs && ae->ae_PrefsName)
    {
        prefs = ae->ae_Prefs;
        prefsname = ae->ae_PrefsName;
        ae->ae_Prefs = NULL;
        ae->ae_PrefsName = NULL;
    }
    arc = AppBase->ab_GlobalStale
          && app_ArcDue(AppBase, IsListEmpty((struct List *)&AppBase->ab_Apps));
    ReleaseSemaphore(&AppBase->ab_Lock);

    if (prefs)
    {
        prefs_Write(AppBase, prefs, prefsname, TRUE, TRUE);
        po_Release(prefs);
        FreeVec(prefsname);
    }
    if (!gone)
        app_FreeEntry(ae);
    if (arc)
        app_SaveGlobal(AppBase, FALSE, TRUE);
    return TRUE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, SetApplicationAttrsA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 7, Application)

/*  FUNCTION
        Change the attributes of an application, see the APPATTR_ tags
        marked (S) in <libraries/application.h>.

        APPATTR_IconType fails while a dock has the icon locked; the docks
        are asked to unlock it, so try again a little later.
        APPATTR_MainPrefsDict hands the dictionary to the application,
        which releases the one it had. APPATTR_FlushPrefs TRUE writes the
        dictionary to ENV: and ENVARC: now, whether or not the application
        registered with REGAPP_SavePrefs.

    RESULT
        FALSE if there was no such application or a tag could not be set.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate = tags, *tag;
    struct AppEntry *ae;
    PrefsObject *flush = NULL;
    STRPTR flushname = NULL;
    BOOL ok = TRUE, saveglobal = FALSE;

    app_PreloadGlobal(AppBase);
    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    if (!(ae = app_Find(AppBase, appID)))
    {
        ReleaseSemaphore(&AppBase->ab_Lock);
        return FALSE;
    }

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG flag;

        switch (tag->ti_Tag)
        {
        case APPATTR_NeedsGameMode:
            if (!tag->ti_Data != !(ae->ae_Flags & APPF_GAMEMODE))
            {
                ae->ae_Flags ^= APPF_GAMEMODE;
                app_Broadcast(AppBase, ae->ae_ID,
                              tag->ti_Data ? APPLIBMT_GameModeEntered : APPLIBMT_GameModeLeft);
                app_UpdateBlanker(AppBase);
            }
            break;

        case APPATTR_AllowsBlanker:
            if (tag->ti_Data)
                ae->ae_Flags |= APPF_ALLOWBLANKER;
            else
                ae->ae_Flags &= ~APPF_ALLOWBLANKER;
            app_UpdateBlanker(AppBase);
            break;

        case APPATTR_Hidden:
            if (!tag->ti_Data != !(ae->ae_Flags & APPF_HIDDEN))
            {
                ae->ae_Flags ^= APPF_HIDDEN;
                app_BroadcastTo(AppBase, APPF_APPNOTIFY, ae->ae_ID, APPLIBMT_AppDidChangeHidden);
            }
            break;

        case APPATTR_IconType:
            if (!tag->ti_Data)
                ok = FALSE;
            else if (ae->ae_IconLocks)
            {
                app_BroadcastTo(AppBase, APPF_APPNOTIFY, ae->ae_ID, APPLIBMT_AppWantsChangeIconType);
                ok = FALSE;
            }
            else
            {
                ae->ae_Icon = *(struct ApplicationIconInfo *)tag->ti_Data;
                app_BroadcastTo(AppBase, APPF_APPNOTIFY, ae->ae_ID, APPLIBMT_AppDidChangeIconType);
            }
            break;

        case APPATTR_AppOpenedDocument:
            if (app_NoteDocOpened(AppBase, ae, (CONST_STRPTR)tag->ti_Data))
                saveglobal = TRUE;
            break;

        case APPATTR_ClearLastUsedDocs:
            if (tag->ti_Data && app_ClearDocsOfApp(AppBase, ae))
                saveglobal = TRUE;
            break;

        case APPATTR_MainPrefsDict:
            if (po_Type((PrefsObject *)tag->ti_Data) != ALPOT_Dictionary)
                ok = FALSE;
            else if ((PrefsObject *)tag->ti_Data != ae->ae_Prefs)
            {
                po_Release(ae->ae_Prefs);
                ae->ae_Prefs = (PrefsObject *)tag->ti_Data;
            }
            break;

        case APPATTR_FlushPrefs:
            if (!tag->ti_Data)
                break;
            if (!ae->ae_Prefs || !(flushname = app_StrDup(ae->ae_PrefsName))
                || !(flush = po_Copy(ae->ae_Prefs)))
                ok = FALSE;
            break;

        default:
            flag = app_FlagFor(tag->ti_Tag);
            if (flag && flag != APPF_UNIQUE && flag != APPF_NOICON)
            {
                if (tag->ti_Data)
                    ae->ae_Flags |= flag;
                else
                    ae->ae_Flags &= ~flag;
            }
            else
                ok = FALSE;
            break;
        }
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    /* The file I/O, with the lock released: snapshots of the global lists
       and of the application's own dictionary. */
    if (saveglobal)
        app_SaveLists(AppBase);
    if (flush)
    {
        if (prefs_Write(AppBase, flush, flushname, TRUE, TRUE))
            ok = FALSE;
        po_Release(flush);
    }
    FreeVec(flushname);

    return ok;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, GetApplicationAttrsA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 8, Application)

/*  FUNCTION
        Read attributes of an application. ti_Data points to the storage:
        an APTR or STRPTR for pointer attributes, a ULONG for the others.
        Strings stay valid while the application is registered.

        APPATTR_MainPrefsDict is the application's own dictionary, made
        empty if it had none; APPATTR_LastUsedDocsArray is a copy, to be
        ALPO_Release'd. APPATTR_IconType may only be used while the icon
        is locked with LockApplicationIcon().

    RESULT
        FALSE if there was no such application or a tag is unknown.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate = tags, *tag;
    struct AppEntry *ae;
    BOOL ok = TRUE;

    if (FindTagItem(APPATTR_LastUsedDocsArray, tags))
        app_PreloadGlobal(AppBase);
    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    if (!(ae = app_Find(AppBase, appID)))
    {
        ReleaseSemaphore(&AppBase->ab_Lock);
        return FALSE;
    }

    while ((tag = NextTagItem(&tstate)))
    {
        APTR *pstore = (APTR *)tag->ti_Data;
        ULONG *lstore = (ULONG *)tag->ti_Data;
        ULONG flag;

        if (!tag->ti_Data)
            continue;

        switch (tag->ti_Tag)
        {
        case APPATTR_Port:          *pstore = ae->ae_Port; break;
        case APPATTR_Name:          *pstore = ae->ae_Name; break;
        case APPATTR_AppIdentifier: *pstore = ae->ae_Ident; break;
        case APPATTR_URLIdentifier: *pstore = ae->ae_URL; break;
        case APPATTR_Description:   *pstore = ae->ae_Description; break;
        case APPATTR_FileName:      *pstore = ae->ae_FileName; break;
        case APPATTR_IconType:      *pstore = &ae->ae_Icon; break;
        case APPATTR_AppIdentifierCounter: *lstore = ae->ae_Counter; break;
        case APPATTR_UniqueApplication: *lstore = (ae->ae_Flags & APPF_UNIQUE) != 0; break;
        case APPATTR_LastUsedDocsArray:
            if (!(*pstore = app_DocsOfApp(AppBase, ae)))
                ok = FALSE;
            break;
        case APPATTR_MainPrefsDict:
            if (!ae->ae_Prefs)
                ae->ae_Prefs = po_NewDict();
            if (!(*pstore = ae->ae_Prefs))
                ok = FALSE;
            break;
        default:
            if ((flag = app_FlagFor(tag->ti_Tag)))
                *lstore = (ae->ae_Flags & flag) != 0;
            else
                ok = FALSE;
            break;
        }
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    return ok;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, FindApplicationA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 9, Application)

/*  FUNCTION
        Find a registered application by FINDAPP_Name (the first instance),
        FINDAPP_AppIdentifier (one instance) or FINDAPP_FileName.

    RESULT
        The application ID, or 0.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR name = (CONST_STRPTR)GetTagData(FINDAPP_Name, 0, tags);
    CONST_STRPTR ident = (CONST_STRPTR)GetTagData(FINDAPP_AppIdentifier, 0, tags);
    CONST_STRPTR file = (CONST_STRPTR)GetTagData(FINDAPP_FileName, 0, tags);
    struct AppEntry *ae;
    ULONG id = 0;

    ObtainSemaphore(&AppBase->ab_Lock);
    ForeachNode(&AppBase->ab_Apps, ae)
    {
        if ((name && Stricmp(ae->ae_Name, name) == 0)
            || (ident && Stricmp(ae->ae_Ident, ident) == 0)
            || (file && ae->ae_FileName && Stricmp(ae->ae_FileName, file) == 0))
        {
            id = ae->ae_ID;
            break;
        }
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    return id;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(BOOL, SendApplicationMsg,

/*  SYNOPSIS */
        AROS_LHA(ULONG, senderAppID, D0),
        AROS_LHA(ULONG, receiverAppID, D1),
        AROS_LHA(struct ApplicationMsg *, msg, A0),
        AROS_LHA(ULONG, type, D2),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 10, Application)

/*  FUNCTION
        Send a message to an application, or with receiverAppID 0 to every
        application except the sender. The library sends a copy: msg, and
        the string of an APPLIBMT_CustomMsg, APPLIBMT_OpenDoc or
        APPLIBMT_PrintDoc, can be reused at once. msg may be NULL for the
        other types.

        The APPLIBMT_App... types always go to every application with
        AppNotifications set, the APPLIBMT_Blanker... types to every one
        with BlankerNotifications set, whatever receiverAppID is.

    RESULT
        TRUE if at least one application was sent the message.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR text = NULL;
    struct AppEntry *ae;
    BOOL sent = FALSE, always;
    ULONG flag = 0;

    if (msg)
    {
        if (type == APPLIBMT_CustomMsg || type == APPLIBMT_ConfirmClose)
            text = ((struct ApplicationCustomMsg *)msg)->customMsg;
        else if (type == APPLIBMT_OpenDoc || type == APPLIBMT_PrintDoc)
            text = ((struct ApplicationOpenPrintDocMsg *)msg)->fileName;
    }
    always = app_IsAlwaysBroadcast(type, &flag);

    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    ForeachNode(&AppBase->ab_Apps, ae)
    {
        if (always)
        {
            if (ae->ae_ID == senderAppID || !(ae->ae_Flags & flag))
                continue;
        }
        else if ((receiverAppID && ae->ae_ID != receiverAppID)
                 || (!receiverAppID && ae->ae_ID == senderAppID))
            continue;
        if (app_Post(AppBase, ae, senderAppID, type, text))
            sent = TRUE;
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    return sent;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH0(struct MinList *, GetApplicationList,

/*  SYNOPSIS */

/*  LOCATION */
        struct ApplicationBase *, AppBase, 12, Application)

/*  FUNCTION
        A snapshot of the registered applications, as a list of
        struct ApplicationNode. Free it with FreeApplicationList().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MinList *list;
    struct AppEntry *ae;

    if (!(list = AllocVec(sizeof(struct MinList), MEMF_ANY)))
        return NULL;
    NEWLIST((struct List *)list);

    ObtainSemaphore(&AppBase->ab_Lock);
    ForeachNode(&AppBase->ab_Apps, ae)
    {
        struct ApplicationNode *an;
        ULONG il = strlen(ae->ae_Ident) + 1, nl = strlen(ae->ae_Name) + 1;
        ULONG dl = ae->ae_Description ? strlen(ae->ae_Description) + 1 : 0;
        ULONG fl = ae->ae_FileName ? strlen(ae->ae_FileName) + 1 : 0;
        STRPTR p;

        if (ae->ae_Flags & APPF_LEAVING)
            continue;
        if (!(an = AllocVec(sizeof(*an) + il + nl + dl + fl, MEMF_ANY | MEMF_CLEAR)))
            break;
        p = (STRPTR)(an + 1);
        an->identifier = p;
        CopyMem(ae->ae_Ident, p, il);
        p += il;
        an->name = p;
        CopyMem(ae->ae_Name, p, nl);
        p += nl;
        if (dl)
        {
            an->description = p;
            CopyMem(ae->ae_Description, p, dl);
            p += dl;
        }
        if (fl)
        {
            an->fileName = p;
            CopyMem(ae->ae_FileName, p, fl);
        }
        an->appID = ae->ae_ID;
        an->hidden = (ae->ae_Flags & APPF_HIDDEN) != 0;
        ADDTAIL((struct List *)list, (struct Node *)an);
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    return list;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, FreeApplicationList,

/*  SYNOPSIS */
        AROS_LHA(struct MinList *, list, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 13, Application)

/*  FUNCTION
        Free a list from GetApplicationList().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Node *node;

    (void)AppBase;
    if (!list)
        return;
    while ((node = REMHEAD((struct List *)list)))
        FreeVec(node);
    FreeVec(list);

    AROS_LIBFUNC_EXIT
}
