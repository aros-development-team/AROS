/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Prefs/Notifications - where pop-ups appear, how long they stay,
          sounds, the log, and which applications may show them.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <libraries/application.h>
#include <zune/prefseditor.h>
#include <zune/systemprefswindow.h>
#include <zune/customclasses.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/application.h>
#include <proto/alib.h>

#include <string.h>

#include "../../system/Notifications/notifyprefs.h"

#define VERSTAG "\0$VER: Notifications 1.0 (18.10.2026)"
static const char version[] __attribute__((used)) = VERSTAG;

struct Library *ApplicationBase;

static CONST_STRPTR positions[] = NPREFS_POSITIONS;
static CONST_STRPTR position_labels[] =
{
    "Bottom right", "Bottom left", "Top right", "Top left", NULL
};

struct AppItem
{
    BOOL ai_Shown;
    TEXT ai_Name[APPNOTIFY_TITLE_MAX + 1];
};

struct NotifyEditor_DATA
{
    Object *position, *time, *maxvisible, *width;
    Object *holdaway, *dnd, *beep, *sounds, *defsound, *log;
    Object *apps;
    ULONG   appid;
};

#define SETUP_INST_DATA struct NotifyEditor_DATA *data = INST_DATA(CLASS, self)

static struct MUI_CustomClass *NotifyEditor_CLASS;
#define NotifyEditorObject BOOPSIOBJMACRO_START(NotifyEditor_CLASS->mcc_Class)

/***************************************************************************/

static AROS_UFH3(APTR, AppConstruct,
                 AROS_UFHA(struct Hook *, h, A0),
                 AROS_UFHA(APTR, pool, A2),
                 AROS_UFHA(struct AppItem *, item, A1))
{
    AROS_USERFUNC_INIT

    struct AppItem *n = AllocPooled(pool, sizeof(*n));

    (void)h;
    if (n)
        *n = *item;
    return n;

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, AppDestruct,
                 AROS_UFHA(struct Hook *, h, A0),
                 AROS_UFHA(APTR, pool, A2),
                 AROS_UFHA(struct AppItem *, item, A1))
{
    AROS_USERFUNC_INIT

    (void)h;
    FreePooled(pool, item, sizeof(*item));

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, AppDisplay,
                 AROS_UFHA(struct Hook *, h, A0),
                 AROS_UFHA(STRPTR *, array, A2),
                 AROS_UFHA(struct AppItem *, item, A1))
{
    AROS_USERFUNC_INIT

    (void)h;
    if (item)
    {
        array[0] = item->ai_Shown ? "Shown" : "\33bHidden";
        array[1] = item->ai_Name;
    }
    else
    {
        array[0] = "\33bPop-ups";
        array[1] = "\33bApplication";
    }

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(LONG, AppCompare,
                 AROS_UFHA(struct Hook *, h, A0),
                 AROS_UFHA(struct AppItem *, a, A2),
                 AROS_UFHA(struct AppItem *, b, A1))
{
    AROS_USERFUNC_INIT

    (void)h;
    return Stricmp(b->ai_Name, a->ai_Name);

    AROS_USERFUNC_EXIT
}

static struct Hook construct_hook = { { NULL, NULL }, (HOOKFUNC)AROS_ASMSYMNAME(AppConstruct), NULL, NULL };
static struct Hook destruct_hook = { { NULL, NULL }, (HOOKFUNC)AROS_ASMSYMNAME(AppDestruct), NULL, NULL };
static struct Hook display_hook = { { NULL, NULL }, (HOOKFUNC)AROS_ASMSYMNAME(AppDisplay), NULL, NULL };
static struct Hook compare_hook = { { NULL, NULL }, (HOOKFUNC)AROS_ASMSYMNAME(AppCompare), NULL, NULL };

/* Add an application to the list once; an existing entry keeps its state. */
static void AddApp(Object *list, CONST_STRPTR name, BOOL shown)
{
    struct AppItem item, *e;
    LONG i;

    /* the server itself never shows anything of its own */
    if (!name || !*name || Stricmp(name, NPREFS_APPNAME) == 0)
        return;
    for (i = 0; ; i++)
    {
        DoMethod(list, MUIM_List_GetEntry, i, (IPTR)&e);
        if (!e)
            break;
        if (Stricmp(e->ai_Name, name) == 0)
            return;
    }
    item.ai_Shown = shown;
    strncpy(item.ai_Name, name, sizeof(item.ai_Name) - 1);
    item.ai_Name[sizeof(item.ai_Name) - 1] = '\0';
    DoMethod(list, MUIM_List_InsertSingle, (IPTR)&item, MUIV_List_Insert_Sorted);
}

/* Every application the system knows of: in the settings, running, or
   used recently. */
static void FillApps(struct NotifyEditor_DATA *data, PrefsObject *dict)
{
    PrefsObject *apps = DictGetObjectForKey(dict, NPK_APPS), *recent = NULL;
    struct ALPOObjKeyIndex oki;
    struct MinList *running;
    LONG count = 0, i;

    set(data->apps, MUIA_List_Quiet, TRUE);
    DoMethod(data->apps, MUIM_List_Clear);

    PrefsDictionary(apps, NULL, ALPODICT_GetCount, (IPTR)&count, TAG_DONE);
    for (i = 0; i < count; i++)
    {
        oki.index = i;
        if (PrefsDictionary(apps, NULL, ALPODICT_GetObjAndKeyAtIndex, (IPTR)&oki, TAG_DONE) && oki.key)
            AddApp(data->apps, oki.key, DictGetBoolForKey(apps, oki.key, TRUE));
    }

    if ((running = GetApplicationList()))
    {
        struct ApplicationNode *an;

        ForeachNode(running, an)
            AddApp(data->apps, an->name, TRUE);
        FreeApplicationList(running);
    }

    if (GetAppLibAttrs(APPLIBATTR_LastUsedAppsArray, (IPTR)&recent, TAG_DONE) && recent)
    {
        struct ALPOObjIndex oi;

        count = 0;
        PrefsArray(recent, NULL, ALPOARR_GetCount, (IPTR)&count, TAG_DONE);
        for (oi.index = 0; oi.index < count; oi.index++)
        {
            if (PrefsArray(recent, NULL, ALPOARR_GetObjAtIndex, (IPTR)&oi, TAG_DONE))
                AddApp(data->apps, DictGetStringForKey(oi.obj, LUA_APPNAME_KEY, NULL), TRUE);
        }
        PrefsArray(recent, NULL, ALPO_Release, 0, TAG_DONE);
    }

    set(data->apps, MUIA_List_Quiet, FALSE);
}

static void SetNumber(PrefsObject *dict, CONST_STRPTR key, LONG v, BOOL isbool)
{
    PrefsObject *n = PrefsNumber(NULL, NULL, isbool ? ALPONUM_AllocSetBool : ALPONUM_AllocSetLong, v,
                                 TAG_DONE);

    if (n && !DictSetObjectForKey(dict, n, key))
        PrefsNumber(n, NULL, ALPO_Release, 0, TAG_DONE);
}

static void SetString(PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR s)
{
    PrefsObject *o = PrefsString(NULL, NULL, ALPOSTR_AllocSetString, s ? (IPTR)s : (IPTR)"", TAG_DONE);

    if (o && !DictSetObjectForKey(dict, o, key))
        PrefsString(o, NULL, ALPO_Release, 0, TAG_DONE);
}

static void Prefs2Gadgets(struct NotifyEditor_DATA *data, PrefsObject *dict)
{
    LONG v;

    set(data->position, MUIA_Cycle_Active,
        DictGetOptionForKey(dict, NPK_POSITION, positions, NDEF_POSITION));
    v = DictGetIntegerForKey(dict, NPK_DISPLAYTIME, NDEF_DISPLAYTIME);
    set(data->time, MUIA_Numeric_Value, (v < 0 || v > NLIM_DISPLAYTIME) ? NDEF_DISPLAYTIME : v);
    v = DictGetIntegerForKey(dict, NPK_MAXVISIBLE, NDEF_MAXVISIBLE);
    set(data->maxvisible, MUIA_Numeric_Value, (v < 1 || v > NLIM_MAXVISIBLE) ? NDEF_MAXVISIBLE : v);
    v = DictGetIntegerForKey(dict, NPK_WIDTH, NDEF_WIDTH);
    set(data->width, MUIA_Numeric_Value, (v < NLIM_WIDTHMIN || v > NLIM_WIDTHMAX) ? NDEF_WIDTH : v);
    set(data->holdaway, MUIA_Selected, DictGetBoolForKey(dict, NPK_HOLDAWAY, NDEF_HOLDAWAY));
    set(data->dnd, MUIA_Selected, DictGetBoolForKey(dict, NPK_DND, NDEF_DND));
    set(data->beep, MUIA_Selected, DictGetBoolForKey(dict, NPK_BEEP, NDEF_BEEP));
    set(data->sounds, MUIA_Selected, DictGetBoolForKey(dict, NPK_SOUNDS, NDEF_SOUNDS));
    set(data->defsound, MUIA_String_Contents, DictGetStringForKey(dict, NPK_DEFSOUND, ""));
    set(data->log, MUIA_Selected, DictGetBoolForKey(dict, NPK_LOG, NDEF_LOG));
    FillApps(data, dict);
}

static void Gadgets2Prefs(struct NotifyEditor_DATA *data, PrefsObject *dict)
{
    PrefsObject *apps;
    struct AppItem *e;
    LONG i;

    SetString(dict, NPK_POSITION, positions[XGET(data->position, MUIA_Cycle_Active)]);
    SetNumber(dict, NPK_DISPLAYTIME, XGET(data->time, MUIA_Numeric_Value), FALSE);
    SetNumber(dict, NPK_MAXVISIBLE, XGET(data->maxvisible, MUIA_Numeric_Value), FALSE);
    SetNumber(dict, NPK_WIDTH, XGET(data->width, MUIA_Numeric_Value), FALSE);
    SetNumber(dict, NPK_HOLDAWAY, XGET(data->holdaway, MUIA_Selected), TRUE);
    SetNumber(dict, NPK_DND, XGET(data->dnd, MUIA_Selected), TRUE);
    SetNumber(dict, NPK_BEEP, XGET(data->beep, MUIA_Selected), TRUE);
    SetNumber(dict, NPK_SOUNDS, XGET(data->sounds, MUIA_Selected), TRUE);
    SetString(dict, NPK_DEFSOUND, (STRPTR)XGET(data->defsound, MUIA_String_Contents));
    SetNumber(dict, NPK_LOG, XGET(data->log, MUIA_Selected), TRUE);

    /* Only the hidden ones need remembering; anything else is shown. */
    if ((apps = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE)))
    {
        for (i = 0; ; i++)
        {
            DoMethod(data->apps, MUIM_List_GetEntry, i, (IPTR)&e);
            if (!e)
                break;
            if (!e->ai_Shown)
                SetNumber(apps, e->ai_Name, FALSE, TRUE);
        }
        if (!DictSetObjectForKey(dict, apps, NPK_APPS))
            PrefsDictionary(apps, NULL, ALPO_Release, 0, TAG_DONE);
    }
}

/***************************************************************************/

struct FHIO
{
    BPTR fh;
    LONG back;
};

static LONG fh_Get(struct ALPOGetProcInfo *pi)
{
    struct FHIO *io = pi->handlePtr;
    LONG c;

    if (io->back >= 0)
    {
        c = io->back;
        io->back = -1;
        return c;
    }
    c = FGetC(io->fh);
    return c < 0 ? -1 : c;
}

static LONG fh_UnGet(LONG c, struct ALPOGetProcInfo *pi)
{
    ((struct FHIO *)pi->handlePtr)->back = c;
    return c;
}

static void fh_Put(LONG c, struct ALPOPutProcInfo *pi)
{
    FPutC(((struct FHIO *)pi->handlePtr)->fh, c);
}

/***************************************************************************/

#define MUIM_NotifyEditor_SetShown  (TAG_USER | 0x4A0001)
#define MUIM_NotifyEditor_Try       (TAG_USER | 0x4A0002)
#define MUIM_NotifyEditor_ViewLog   (TAG_USER | 0x4A0003)

struct MUIP_NotifyEditor_SetShown { STACKED ULONG MethodID; STACKED LONG shown; };

static Object *Check(void)
{
    return MUI_MakeObject(MUIO_Checkmark, NULL);
}

static IPTR NotifyEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *position, *time, *maxvisible, *width, *holdaway, *dnd, *beep, *sounds, *defsound, *log;
    Object *apps, *appview, *show, *hide, *try, *viewlog;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_PrefsEditor_Name,     (IPTR)"Notifications",
        MUIA_PrefsEditor_Path,     (IPTR)NPREFS_FILE,
        MUIA_PrefsEditor_IconTool, (IPTR)NPREFS_EDITOR,

        Child, HGroup,
            Child, VGroup,
                Child, ColGroup(2),
                    GroupFrameT("Pop-ups"),
                    Child, Label1("Position:"),
                    Child, (IPTR)(position = MUI_MakeObject(MUIO_Cycle, NULL, (IPTR)position_labels)),
                    Child, Label1("Seconds shown:"),
                    Child, (IPTR)(time = SliderObject,
                        MUIA_Numeric_Min, 0,
                        MUIA_Numeric_Max, NLIM_DISPLAYTIME,
                        MUIA_ShortHelp, (IPTR)"0 shows longer texts for longer, 4 to 10 seconds",
                    End),
                    Child, Label1("At most on screen:"),
                    Child, (IPTR)(maxvisible = SliderObject,
                        MUIA_Numeric_Min, 1,
                        MUIA_Numeric_Max, NLIM_MAXVISIBLE,
                        MUIA_ShortHelp, (IPTR)"More wait until there is room",
                    End),
                    Child, Label1("Width:"),
                    Child, (IPTR)(width = SliderObject,
                        MUIA_Numeric_Min, NLIM_WIDTHMIN,
                        MUIA_Numeric_Max, NLIM_WIDTHMAX,
                    End),
                End,
                Child, ColGroup(2),
                    GroupFrameT("Behaviour"),
                    Child, (IPTR)(holdaway = Check()),
                    Child, LLabel1("Keep pop-ups while I am away"),
                    Child, (IPTR)(dnd = Check()),
                    Child, LLabel1("Do not disturb (urgent ones still show)"),
                    Child, (IPTR)(beep = Check()),
                    Child, LLabel1("Flash the screen for urgent ones"),
                    Child, (IPTR)(log = Check()),
                    Child, LLabel1("Keep a log in " NPREFS_LOGFILE),
                    Child, (IPTR)(sounds = Check()),
                    Child, LLabel1("Play sounds"),
                End,
                Child, HGroup,
                    GroupFrameT("Sound for every pop-up"),
                    Child, (IPTR)PopaslObject,
                        MUIA_Popstring_String, (IPTR)(defsound = StringObject,
                            StringFrame,
                            MUIA_CycleChain, TRUE,
                        End),
                        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                        ASLFR_TitleText, (IPTR)"Choose a sound",
                    End,
                End,
            End,
            Child, VGroup,
                GroupFrameT("Applications"),
                Child, (IPTR)(appview = ListviewObject,
                    MUIA_Listview_List, (IPTR)(apps = ListObject,
                        InputListFrame,
                        MUIA_List_Format, (IPTR)"MIW=20 BAR,",
                        MUIA_List_Title, TRUE,
                        MUIA_List_ConstructHook, (IPTR)&construct_hook,
                        MUIA_List_DestructHook, (IPTR)&destruct_hook,
                        MUIA_List_DisplayHook, (IPTR)&display_hook,
                        MUIA_List_CompareHook, (IPTR)&compare_hook,
                    End),
                    MUIA_ShortHelp, (IPTR)"Double-click to switch an application's pop-ups on or off",
                End),
                Child, HGroup,
                    Child, (IPTR)(show = SimpleButton("_Show")),
                    Child, (IPTR)(hide = SimpleButton("_Hide")),
                End,
                Child, HGroup,
                    Child, (IPTR)(try = SimpleButton("Show _an example")),
                    Child, (IPTR)(viewlog = SimpleButton("View _log")),
                End,
            End,
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;
        Object *changers[] = { holdaway, dnd, beep, sounds, log };
        ULONG i;

        data->position = position;
        data->time = time;
        data->maxvisible = maxvisible;
        data->width = width;
        data->holdaway = holdaway;
        data->dnd = dnd;
        data->beep = beep;
        data->sounds = sounds;
        data->defsound = defsound;
        data->log = log;
        data->apps = apps;
        data->appid = RegisterApplication("NotificationsPrefs", REGAPP_URLIdentifier, (IPTR)NPREFS_URL,
                                          REGAPP_Description, (IPTR)"Notifications settings",
                                          REGAPP_NoIcon, TRUE, TAG_DONE);

        for (i = 0; i < sizeof(changers) / sizeof(changers[0]); i++)
            DoMethod(changers[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
                     (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(position, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
                 (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(time, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                 (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(maxvisible, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                 (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(width, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
                 (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(defsound, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
                 (IPTR)self, 3, MUIM_Set, MUIA_PrefsEditor_Changed, TRUE);
        DoMethod(show, MUIM_Notify, MUIA_Pressed, FALSE,
                 (IPTR)self, 2, MUIM_NotifyEditor_SetShown, TRUE);
        DoMethod(hide, MUIM_Notify, MUIA_Pressed, FALSE,
                 (IPTR)self, 2, MUIM_NotifyEditor_SetShown, FALSE);
        DoMethod(appview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
                 (IPTR)self, 2, MUIM_NotifyEditor_SetShown, -1);
        DoMethod(try, MUIM_Notify, MUIA_Pressed, FALSE,
                 (IPTR)self, 1, MUIM_NotifyEditor_Try);
        DoMethod(viewlog, MUIM_Notify, MUIA_Pressed, FALSE,
                 (IPTR)self, 1, MUIM_NotifyEditor_ViewLog);

        DoMethod(self, MUIM_PrefsEditor_SetDefaults);
    }
    return (IPTR)self;
}

static IPTR NotifyEditor__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if (data->appid)
        UnregisterApplication(data->appid, NULL);
    return DoSuperMethodA(CLASS, self, message);
}

/* shown: TRUE, FALSE, or -1 to switch */
static IPTR NotifyEditor__MUIM_NotifyEditor_SetShown(Class *CLASS, Object *self,
                                                     struct MUIP_NotifyEditor_SetShown *message)
{
    SETUP_INST_DATA;
    struct AppItem *e = NULL;

    DoMethod(data->apps, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&e);
    if (!e)
        return FALSE;
    e->ai_Shown = message->shown < 0 ? !e->ai_Shown : message->shown;
    DoMethod(data->apps, MUIM_List_Redraw, MUIV_List_Redraw_Active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return TRUE;
}

/* Put the settings on trial and show a pop-up with them. */
static IPTR NotifyEditor__MUIM_NotifyEditor_Try(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    DoMethod(self, MUIM_PrefsEditor_Test);
    if (data->appid)
        Notify(data->appid,
               APPNOTIFY_Title, (IPTR)"Notifications",
               APPNOTIFY_Text,  (IPTR)"This is how notifications look with these settings.",
               TAG_DONE);
    return TRUE;
}

static IPTR NotifyEditor__MUIM_NotifyEditor_ViewLog(Class *CLASS, Object *self, Msg message)
{
    SystemTags("SYS:Utilities/MultiView " NPREFS_LOGFILE,
               SYS_Asynch, TRUE, SYS_Input, 0, SYS_Output, 0, TAG_DONE);
    return TRUE;
}

static IPTR NotifyEditor__MUIM_PrefsEditor_ImportFH(Class *CLASS, Object *self,
                                                   struct MUIP_PrefsEditor_ImportFH *message)
{
    SETUP_INST_DATA;
    struct FHIO io = { message->fh, -1 };
    struct ALPOGetProcInfo pi;
    TEXT otag[TAGBUFFER_SIZE];
    PrefsObject *dict;
    ULONG err;

    if (!(dict = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE)))
        return FALSE;
    memset(&pi, 0, sizeof(pi));
    pi.getChProc = fh_Get;
    pi.unGetChProc = fh_UnGet;
    pi.handlePtr = &io;
    pi.key_otag = otag;
    pi.encoding = ALPOENC_UTF8;
    err = ReadPrefs(dict, READPREFS_GetProcInfo, (IPTR)&pi, TAG_DONE);
    if (!err)
        Prefs2Gadgets(data, dict);
    PrefsDictionary(dict, NULL, ALPO_Release, 0, TAG_DONE);
    return err == 0;
}

static IPTR NotifyEditor__MUIM_PrefsEditor_ExportFH(Class *CLASS, Object *self,
                                                   struct MUIP_PrefsEditor_ExportFH *message)
{
    SETUP_INST_DATA;
    struct FHIO io = { message->fh, -1 };
    struct ALPOPutProcInfo pi;
    PrefsObject *dict;
    ULONG err;

    if (!(dict = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE)))
        return FALSE;
    Gadgets2Prefs(data, dict);
    memset(&pi, 0, sizeof(pi));
    pi.putChProc = fh_Put;
    pi.handlePtr = &io;
    pi.encoding = ALPOENC_UTF8;
    err = WritePrefs(dict, WRITEPREFS_PutProcInfo, (IPTR)&pi, TAG_DONE);
    PrefsDictionary(dict, NULL, ALPO_Release, 0, TAG_DONE);
    return err == 0;
}

static IPTR NotifyEditor__MUIM_PrefsEditor_SetDefaults(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    PrefsObject *dict;

    if (!(dict = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_DONE)))
        return FALSE;
    Prefs2Gadgets(data, dict);
    PrefsDictionary(dict, NULL, ALPO_Release, 0, TAG_DONE);
    return TRUE;
}

BOOPSI_DISPATCHER(IPTR, NotifyEditor_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW:
        return NotifyEditor__OM_NEW(CLASS, self, (struct opSet *)message);
    case OM_DISPOSE:
        return NotifyEditor__OM_DISPOSE(CLASS, self, message);
    case MUIM_NotifyEditor_SetShown:
        return NotifyEditor__MUIM_NotifyEditor_SetShown(CLASS, self,
                                                        (struct MUIP_NotifyEditor_SetShown *)message);
    case MUIM_NotifyEditor_Try:
        return NotifyEditor__MUIM_NotifyEditor_Try(CLASS, self, message);
    case MUIM_NotifyEditor_ViewLog:
        return NotifyEditor__MUIM_NotifyEditor_ViewLog(CLASS, self, message);
    case MUIM_PrefsEditor_ImportFH:
        return NotifyEditor__MUIM_PrefsEditor_ImportFH(CLASS, self,
                                                       (struct MUIP_PrefsEditor_ImportFH *)message);
    case MUIM_PrefsEditor_ExportFH:
        return NotifyEditor__MUIM_PrefsEditor_ExportFH(CLASS, self,
                                                       (struct MUIP_PrefsEditor_ExportFH *)message);
    case MUIM_PrefsEditor_SetDefaults:
        return NotifyEditor__MUIM_PrefsEditor_SetDefaults(CLASS, self, message);
    }
    return DoSuperMethodA(CLASS, self, message);
}
BOOPSI_DISPATCHER_END

/***************************************************************************/

int main(void)
{
    Object *application, *window;
    int ret = RETURN_FAIL;

    if (!(ApplicationBase = OpenLibrary("application.library", 1)))
        return RETURN_FAIL;

    if ((NotifyEditor_CLASS = MUI_CreateCustomClass(NULL, MUIC_PrefsEditor, NULL,
                                                    sizeof(struct NotifyEditor_DATA),
                                                    NotifyEditor_Dispatcher)))
    {
        application = (Object *)ApplicationObject,
            MUIA_Application_Title, (IPTR)"Notifications",
            MUIA_Application_Version, (IPTR)(VERSTAG + 1),
            MUIA_Application_Description, (IPTR)"Notifications settings",
            MUIA_Application_SingleTask, TRUE,
            MUIA_Application_Base, (IPTR)"NOTIFYPREF",
            SubWindow, (IPTR)(window = (Object *)SystemPrefsWindowObject,
                MUIA_Window_ID, MAKE_ID('N', 'T', 'F', 'Y'),
                WindowContents, (IPTR)NotifyEditorObject,
                End,
            End),
        End;

        if (application)
        {
            SET(window, MUIA_Window_Open, TRUE);
            DoMethod(application, MUIM_Application_Execute);
            MUI_DisposeObject(application);
            ret = RETURN_OK;
        }
        MUI_DeleteCustomClass(NotifyEditor_CLASS);
    }

    CloseLibrary(ApplicationBase);
    return ret;
}
