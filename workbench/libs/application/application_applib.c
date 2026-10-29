/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Library-wide state: the last used applications and documents,
          the blanker and game mode, and application icon locks.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include LC_LIBDEFS_FILE

#define LU_APPSMAX_KEY  "LastUsedAppsMaxCount"
#define LU_DOCSMAX_KEY  "LastUsedDocsMaxCount"
#define LU_DEFAULTMAX   10

/* Callers hold ab_Lock; the file was normally read by app_PreloadGlobal()
   already, so this only makes the empty dictionary. */
void app_LoadGlobal(struct ApplicationBase *AppBase)
{
    if (AppBase->ab_GlobalLoaded || !app_IsProcess())
        return;
    if (!AppBase->ab_Global && !(AppBase->ab_Global = po_NewDict()))
        return;
    prefs_Read(AppBase, AppBase->ab_Global, APPLIB_GLOBALPREFS, TRUE, TRUE);
    AppBase->ab_GlobalLoaded = TRUE;
}

void app_PreloadGlobal(struct ApplicationBase *AppBase)
{
    PrefsObject *tmp;

    if (AppBase->ab_GlobalLoaded || !app_IsProcess() || !(tmp = po_NewDict()))
        return;
    prefs_Read(AppBase, tmp, APPLIB_GLOBALPREFS, TRUE, TRUE);

    ObtainSemaphore(&AppBase->ab_Lock);
    if (!AppBase->ab_GlobalLoaded)
    {
        if (AppBase->ab_Global)
            po_MoveContents(AppBase->ab_Global, tmp);
        else
        {
            AppBase->ab_Global = tmp;
            tmp = NULL;
        }
        AppBase->ab_GlobalLoaded = TRUE;
    }
    ReleaseSemaphore(&AppBase->ab_Lock);
    po_Release(tmp);
}

static LONG app_Minute(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return ds.ds_Days * 1440 + ds.ds_Minute;
}

/* ENV: always, ENVARC: when asked. One save at a time, so the last writer
   writes the newest state. */
void app_SaveGlobal(struct ApplicationBase *AppBase, BOOL envarc)
{
    PrefsObject *copy;

    if (!app_IsProcess())
        return;
    ObtainSemaphore(&AppBase->ab_SaveLock);
    ObtainSemaphore(&AppBase->ab_Lock);
    copy = AppBase->ab_GlobalLoaded ? po_Copy(AppBase->ab_Global) : NULL;
    if (copy)
    {
        AppBase->ab_GlobalStale = !envarc;
        if (envarc)
            AppBase->ab_ArcMinute = app_Minute();
    }
    ReleaseSemaphore(&AppBase->ab_Lock);
    if (copy)
    {
        prefs_Write(AppBase, copy, APPLIB_GLOBALPREFS, TRUE, envarc);
        po_Release(copy);
    }
    ReleaseSemaphore(&AppBase->ab_SaveLock);
}

/* The last used lists changed: ENV: now, ENVARC: when the last application
   has gone or at most once a minute. */
void app_SaveLists(struct ApplicationBase *AppBase, BOOL last)
{
    BOOL envarc;

    if (!app_IsProcess())
        return;
    ObtainSemaphore(&AppBase->ab_Lock);
    envarc = last || AppBase->ab_ArcMinute < 0 || app_Minute() - AppBase->ab_ArcMinute >= 1;
    ReleaseSemaphore(&AppBase->ab_Lock);
    app_SaveGlobal(AppBase, envarc);
}

static LONG lu_Max(struct ApplicationBase *AppBase, CONST_STRPTR key)
{
    LONG max = po_DictLong(AppBase->ab_Global, key, LU_DEFAULTMAX);

    if (max < 0)
        max = 0;
    if (max > APPLIB_LASTUSED_MAX)
        max = APPLIB_LASTUSED_MAX;
    return max;
}

/* The array for key in the global dictionary, made if missing. */
static PrefsObject *lu_Array(struct ApplicationBase *AppBase, CONST_STRPTR key)
{
    PrefsObject *arr;

    if (!AppBase->ab_Global)
        return NULL;
    if (po_Type(arr = po_DictGet(AppBase->ab_Global, key)) == ALPOT_Array)
        return arr;
    if ((arr = po_NewArray()) && !po_DictSet(AppBase->ab_Global, key, arr))
    {
        po_Release(arr);
        arr = NULL;
    }
    return arr;
}

static void lu_Trim(PrefsObject *arr, LONG max)
{
    while (po_Count(arr) > max)
        po_ArrayRemove(arr, po_Count(arr) - 1);
}

/* Put entry first, after removing entries whose matchkey has the same value. */
static void lu_Push(PrefsObject *arr, PrefsObject *entry, CONST_STRPTR matchkey, CONST_STRPTR matchval,
                    LONG max)
{
    LONG i;

    for (i = po_Count(arr) - 1; i >= 0; i--)
    {
        CONST_STRPTR v = po_DictString(po_At(arr, i), matchkey, NULL);

        if (v && Stricmp(v, matchval) == 0)
            po_ArrayRemove(arr, i);
    }
    if (!po_ArrayInsert(arr, entry, 0))
        po_Release(entry);
    lu_Trim(arr, max);
}

static BOOL lu_SetString(PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR value)
{
    PrefsObject *s;

    if (!value)
        return TRUE;
    if (!(s = po_NewString(value)))
        return FALSE;
    if (!po_DictSet(dict, key, s))
    {
        po_Release(s);
        return FALSE;
    }
    return TRUE;
}

static PrefsObject *lu_Entry(CONST_STRPTR k1, CONST_STRPTR v1, CONST_STRPTR k2, CONST_STRPTR v2,
                             CONST_STRPTR k3, CONST_STRPTR v3)
{
    PrefsObject *d, *date;

    if (!(d = po_NewDict()))
        return NULL;
    if ((date = po_NewNow()) && !po_DictSet(d, LUA_OPENDATE_KEY, date))
        po_Release(date);
    if (!lu_SetString(d, k1, v1) || !lu_SetString(d, k2, v2) || !lu_SetString(d, k3, v3))
    {
        po_Release(d);
        return NULL;
    }
    return d;
}

/* TRUE when the list changed and the caller has to save it. */
BOOL app_NoteAppStarted(struct ApplicationBase *AppBase, struct AppEntry *ae)
{
    PrefsObject *arr, *entry;

    app_LoadGlobal(AppBase);
    if (!(arr = lu_Array(AppBase, LUA_BASE_KEY)))
        return FALSE;
    if (!(entry = lu_Entry(LUA_APPIDENTIFIER_KEY, ae->ae_BaseIdent, LUA_APPNAME_KEY, ae->ae_Name,
                           LUA_APPPATH_KEY, ae->ae_FileName)))
        return FALSE;
    lu_Push(arr, entry, LUA_APPIDENTIFIER_KEY, ae->ae_BaseIdent, lu_Max(AppBase, LU_APPSMAX_KEY));
    app_BroadcastTo(AppBase, APPF_APPNOTIFY, 0, APPLIBMT_LastAppDocChange);
    return TRUE;
}

BOOL app_NoteDocOpened(struct ApplicationBase *AppBase, struct AppEntry *ae, CONST_STRPTR path)
{
    PrefsObject *arr, *entry;

    if (!path || !*path)
        return FALSE;
    app_LoadGlobal(AppBase);
    if (!(arr = lu_Array(AppBase, LUD_BASE_KEY)))
        return FALSE;
    if (!(entry = lu_Entry(LUD_DOCPATH_KEY, path, LUD_APPIDENTIFIER_KEY, ae->ae_BaseIdent,
                           LUD_APPPATH_KEY, ae->ae_FileName)))
        return FALSE;
    lu_Push(arr, entry, LUD_DOCPATH_KEY, path, lu_Max(AppBase, LU_DOCSMAX_KEY));
    app_BroadcastTo(AppBase, APPF_APPNOTIFY, 0, APPLIBMT_LastAppDocChange);
    return TRUE;
}

/* A copy of the global document entries of one application. */
PrefsObject *app_DocsOfApp(struct ApplicationBase *AppBase, struct AppEntry *ae)
{
    PrefsObject *all, *mine;
    LONG i;

    app_LoadGlobal(AppBase);
    if (!(mine = po_NewArray()))
        return NULL;
    all = po_DictGet(AppBase->ab_Global, LUD_BASE_KEY);
    for (i = 0; i < po_Count(all); i++)
    {
        PrefsObject *e = po_At(all, i), *c;
        CONST_STRPTR id = po_DictString(e, LUD_APPIDENTIFIER_KEY, NULL);

        if (id && Stricmp(id, ae->ae_BaseIdent) == 0 && (c = po_Copy(e)))
        {
            if (!po_ArrayInsert(mine, c, -1))
                po_Release(c);
        }
    }
    return mine;
}

BOOL app_ClearDocsOfApp(struct ApplicationBase *AppBase, struct AppEntry *ae)
{
    PrefsObject *all;
    BOOL changed = FALSE;
    LONG i;

    app_LoadGlobal(AppBase);
    all = po_DictGet(AppBase->ab_Global, LUD_BASE_KEY);
    for (i = po_Count(all) - 1; i >= 0; i--)
    {
        CONST_STRPTR id = po_DictString(po_At(all, i), LUD_APPIDENTIFIER_KEY, NULL);

        if (id && Stricmp(id, ae->ae_BaseIdent) == 0)
        {
            po_ArrayRemove(all, i);
            changed = TRUE;
        }
    }
    if (changed)
        app_BroadcastTo(AppBase, APPF_APPNOTIFY, 0, APPLIBMT_LastAppDocChange);
    return changed;
}

BOOL app_GameModeActive(struct ApplicationBase *AppBase)
{
    struct AppEntry *ae;

    ForeachNode(&AppBase->ab_Apps, ae)
    {
        if (ae->ae_Flags & APPF_GAMEMODE)
            return TRUE;
    }
    return FALSE;
}

static BOOL app_BlankerAllowed(struct ApplicationBase *AppBase)
{
    struct AppEntry *ae;

    ForeachNode(&AppBase->ab_Apps, ae)
    {
        if ((ae->ae_Flags & APPF_GAMEMODE) || !(ae->ae_Flags & APPF_ALLOWBLANKER))
            return FALSE;
    }
    return TRUE;
}

/* Tell the blankers when the answer to "may the screen blank?" changes. */
void app_UpdateBlanker(struct ApplicationBase *AppBase)
{
    BOOL allowed = app_BlankerAllowed(AppBase);

    if (allowed != AppBase->ab_BlankerAllowed)
    {
        AppBase->ab_BlankerAllowed = allowed;
        app_BroadcastTo(AppBase, APPF_BLANKNOTIFY, 0,
                        allowed ? APPLIBMT_BlankerAllow : APPLIBMT_BlankerDisallow);
    }
}

/*****************************************************************************

    NAME */
#include <proto/application.h>

        AROS_LH1(BOOL, GetAppLibAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 14, Application)

/*  FUNCTION
        Read library-wide attributes, APPLIBATTR_ tags marked (G).
        The arrays are copies: ALPO_Release them when done. Their entries
        are dictionaries with the LUA_ or LUD_ keys, most recent first.

    RESULT
        FALSE if a tag could not be read.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate = tags, *tag;
    BOOL ok = TRUE;

    app_PreloadGlobal(AppBase);
    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    app_LoadGlobal(AppBase);

    while ((tag = NextTagItem(&tstate)))
    {
        if (!tag->ti_Data)
            continue;
        switch (tag->ti_Tag)
        {
        case APPLIBATTR_LastUsedAppsArray:
        case APPLIBATTR_LastUsedDocsArray:
            {
                PrefsObject *arr = lu_Array(AppBase, tag->ti_Tag == APPLIBATTR_LastUsedAppsArray
                                                     ? LUA_BASE_KEY : LUD_BASE_KEY);

                if (!(*(PrefsObject **)tag->ti_Data = arr ? po_Copy(arr) : po_NewArray()))
                    ok = FALSE;
            }
            break;
        case APPLIBATTR_LastUsedAppsMaxCount:
            *(ULONG *)tag->ti_Data = lu_Max(AppBase, LU_APPSMAX_KEY);
            break;
        case APPLIBATTR_LastUsedDocsMaxCount:
            *(ULONG *)tag->ti_Data = lu_Max(AppBase, LU_DOCSMAX_KEY);
            break;
        case APPLIBATTR_BlankerIsAllowed:
            *(ULONG *)tag->ti_Data = app_BlankerAllowed(AppBase);
            break;
        case APPLIBATTR_GameModeIsActive:
            *(ULONG *)tag->ti_Data = app_GameModeActive(AppBase);
            break;
        default:
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
        AROS_LH2(BOOL, SetAppLibAttrsA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, dummy, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 15, Application)

/*  FUNCTION
        Change library-wide attributes, APPLIBATTR_ tags marked (S). The
        settings are saved at once to ENV:Sys/application.xml and
        ENVARC:Sys/application.xml.

    INPUTS
        dummy - 0.

    RESULT
        FALSE if a tag could not be set.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tstate = tags, *tag;
    BOOL ok = TRUE, changed = FALSE;

    (void)dummy;
    app_PreloadGlobal(AppBase);
    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    app_LoadGlobal(AppBase);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case APPLIBATTR_LastUsedAppsMaxCount:
        case APPLIBATTR_LastUsedDocsMaxCount:
            {
                BOOL apps = (tag->ti_Tag == APPLIBATTR_LastUsedAppsMaxCount);
                PrefsObject *n;

                if (tag->ti_Data > APPLIB_LASTUSED_MAX || !AppBase->ab_Global
                    || !(n = po_NewLong(tag->ti_Data)))
                {
                    ok = FALSE;
                    break;
                }
                if (!po_DictSet(AppBase->ab_Global, apps ? LU_APPSMAX_KEY : LU_DOCSMAX_KEY, n))
                {
                    po_Release(n);
                    ok = FALSE;
                    break;
                }
                lu_Trim(lu_Array(AppBase, apps ? LUA_BASE_KEY : LUD_BASE_KEY), tag->ti_Data);
                changed = TRUE;
            }
            break;
        case APPLIBATTR_ClearLastUsedApps:
        case APPLIBATTR_ClearLastUsedDocs:
            if (tag->ti_Data)
            {
                po_Clear(lu_Array(AppBase, tag->ti_Tag == APPLIBATTR_ClearLastUsedApps
                                           ? LUA_BASE_KEY : LUD_BASE_KEY));
                changed = TRUE;
            }
            break;
        default:
            ok = FALSE;
            break;
        }
    }

    if (changed)
        app_BroadcastTo(AppBase, APPF_APPNOTIFY, 0, APPLIBMT_LastAppDocChange);
    ReleaseSemaphore(&AppBase->ab_Lock);

    if (changed)
        app_SaveGlobal(AppBase, TRUE);

    return ok;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, LockApplicationIcon,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 16, Application)

/*  FUNCTION
        Lock an application's icon so that APPATTR_IconType can be read and
        the icon or docky used. While it is locked the application cannot
        change its icon type, and UnregisterApplication() waits for up to
        five seconds; after that the application is gone but its icon
        information stays valid until the last unlock. Locks nest.

        A dock must unlock the icon as soon as it is sent
        APPLIBMT_AppWantsChangeIconType or APPLIBMT_AppUnregister.

    RESULT
        FALSE if there is no such application or it is unregistering.

    SEE ALSO
        UnlockApplicationIcon()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AppEntry *ae;
    BOOL ok = FALSE;

    ObtainSemaphore(&AppBase->ab_Lock);
    if ((ae = app_Find(AppBase, appID)) && !(ae->ae_Flags & APPF_LEAVING))
    {
        ae->ae_IconLocks++;
        ok = TRUE;
    }
    ReleaseSemaphore(&AppBase->ab_Lock);

    return ok;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, UnlockApplicationIcon,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 17, Application)

/*  FUNCTION
        Undo one LockApplicationIcon().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct AppEntry *ae, *gone = NULL;

    ObtainSemaphore(&AppBase->ab_Lock);
    if ((ae = app_Find(AppBase, appID)))
    {
        if (ae->ae_IconLocks)
            ae->ae_IconLocks--;
    }
    else
    {
        ForeachNode(&AppBase->ab_Gone, ae)
        {
            if (ae->ae_ID == appID)
            {
                if (ae->ae_IconLocks && !--ae->ae_IconLocks)
                {
                    REMOVE(ae);
                    gone = ae;
                }
                break;
            }
        }
    }
    ReleaseSemaphore(&AppBase->ab_Lock);
    if (gone)
        app_FreeEntry(gone);

    AROS_LIBFUNC_EXIT
}
