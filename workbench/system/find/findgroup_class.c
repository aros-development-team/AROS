/*
    Copyright © 2016-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/utility.h>
#include <proto/workbench.h>
#include <proto/alib.h>

#include <libraries/mui.h>
#include <mui/NList_mcc.h>
#include <mui/NListview_mcc.h>

//#define DEBUG 1
#include <aros/debug.h>
#include <zune/customclasses.h>

#include <stdio.h>

#include "findgroup_class.h"
#include "locale.h"

#define PATHNAMESIZE (1024)


struct FindGroup_DATA
{
    Object *str_path, *str_pattern, *str_contents;
    Object *btn_start, *btn_stop, *btn_open, *btn_view, *btn_parent;
    Object *lst_result;
    Object *txt_status;
    Object *scanproc;
    struct Hook openwbobj_hook;
    struct Hook view_hook;
    struct Hook parent_hook;
    struct Hook activeentry_hook;
};


struct Listentry
{
    STRPTR fullname;
    ULONG namebuflen;
    struct FileInfoBlock fib;
};

static struct Hook list_display_hook, list_constr_hook, list_destr_hook, list_compare_hook;


// =======================================================================================

static void display_doserror(Object *app, ULONG error)
{
    TEXT buffer[255];
    Fault(error, NULL, buffer, sizeof buffer);
    MUI_Request(app, NULL, 0, _(MSG_APP_TITLE), _(MSG_OK), _(MSG_ERR_DOS), buffer);
}

// =======================================================================================

static BOOL checkfile(Object *app, struct AnchorPath *anchorpath, STRPTR pattern, STRPTR content)
{
    D(bug("[Find::checkfile] name %s pattern %s content %s\n", anchorpath->ap_Info.fib_FileName, pattern, content));

    // warning: this function is called from a sub process

    LONG retval = FALSE;

    if (anchorpath->ap_Info.fib_DirEntryType > 0) // ignore directories
    {
        return FALSE;
    }

    if ((pattern[0] == '\0') || MatchPatternNoCase(pattern, anchorpath->ap_Info.fib_FileName))
    {
        if (content && (content[0] != '\0'))
        {
            D(bug("[Find::checkfile] content search\n"));

            BPTR fh;
            LONG searchlen = strlen(content);
            LONG textlen = anchorpath->ap_Info.fib_Size;
            TEXT *text, *oldtext;
            LONG i;

            if (textlen != 0)
            {
            fh = Open(anchorpath->ap_Buf, MODE_OLDFILE);
            if (fh)
            {
                text = oldtext = AllocVec(textlen, MEMF_ANY);
                if (text)
                {
                    if (Read(fh, text, textlen) == textlen)
                    {
                        textlen -= searchlen;
                        while (textlen >= 0)
                        {
                            for(i = 0; i < searchlen; i++)
                            {
                                if (ToUpper(text[i]) != ToUpper(content[i]))
                                {
                                    break;
                                }
                            }

                            if (i == searchlen)
                            {
                                retval = TRUE;
                                break;
                            }
                            text++;
                            textlen--;
                        }
                    }
                    else
                    {
                        // Read() failed
                        // app must be NULL to avoid deadlocks
                        display_doserror(NULL, IoErr());
                    }
                    FreeVec(oldtext);
                }
                else
                {
                    MUI_Request(NULL, NULL, 0, _(MSG_APP_TITLE), _(MSG_OK), _(MSG_ERR_NO_MEM));
                }
                Close(fh);
            }
            else
            {
                // Open() failed
                // app must be NULL to avoid deadlocks
                display_doserror(NULL, IoErr());
            }
            }
        }
        else
        {
            retval = TRUE;
        }
    }
    return retval;
}

// =======================================================================================

AROS_UFH3(APTR, list_constr_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Listentry *, li, A1))
{
    AROS_USERFUNC_INIT

    struct Listentry *newentry = AllocPooled(pool, sizeof(struct Listentry));
    ULONG buflen;
    if (newentry)
    {
        buflen = strlen(li->fullname) + 1;
        newentry->fullname = AllocPooled(pool, buflen);
        if (newentry->fullname)
        {
            newentry->namebuflen = buflen;
            CopyMem(li->fullname, newentry->fullname, buflen);
        }
        newentry->fib = li->fib;
    }
    return newentry;

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3(void, list_destr_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Listentry *, li, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, li->fullname, li->namebuflen);
    FreePooled(pool, li, sizeof(struct Listentry));

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3S(LONG, list_display_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct Listentry *, li, A1))
{
    AROS_USERFUNC_INIT

    if (li)
    {
        static TEXT protbuf[8], sizebuf[20];
        static TEXT datebuf[20], timebuf[20];
        struct DateTime dt;

        // Protection bits
        protbuf[0] = li->fib.fib_Protection & FIBF_SCRIPT  ? 's' : '-';
        protbuf[1] = li->fib.fib_Protection & FIBF_PURE    ? 'p' : '-';
        protbuf[2] = li->fib.fib_Protection & FIBF_ARCHIVE ? 'a' : '-';

        // Not set means action is allowed!
        protbuf[3] = li->fib.fib_Protection & FIBF_READ    ? '-' : 'r';
        protbuf[4] = li->fib.fib_Protection & FIBF_WRITE   ? '-' : 'w';
        protbuf[5] = li->fib.fib_Protection & FIBF_EXECUTE ? '-' : 'e';
        protbuf[6] = li->fib.fib_Protection & FIBF_DELETE  ? '-' : 'd';
        protbuf[7] = '\0';

        // Size
        snprintf(sizebuf, sizeof sizebuf, "%lu", (long unsigned int)li->fib.fib_Size);

        // Date
        datebuf[0] = '\0';
        timebuf[0] = '\0';
        dt.dat_Stamp = li->fib.fib_Date;
        dt.dat_Format = FORMAT_DEF;
        dt.dat_Flags = 0;
        dt.dat_StrDay = NULL;
        dt.dat_StrDate = datebuf;
        dt.dat_StrTime = timebuf;
        DateToStr(&dt);

        *array++ = li->fullname;
        *array++ = sizebuf;
        *array++ = protbuf;
        *array++ = datebuf;
        *array++ = timebuf;
        *array++ = li->fib.fib_Comment;
    }
    else
    {
        *array++ = (STRPTR)_(MSG_LST_FULLPATH);
        *array++ = (STRPTR)_(MSG_LST_SIZE);
        *array++ = (STRPTR)_(MSG_LST_ATTRIBUTES);
        *array++ = (STRPTR)_(MSG_LST_DATE);
        *array++ = (STRPTR)_(MSG_LST_TIME);
        *array++ = (STRPTR)_(MSG_LST_COMMENT);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3(LONG, list_compare_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct Listentry *, li1, A2),
    AROS_UFHA(struct Listentry *, li2, A1))
{
    AROS_USERFUNC_INIT

    return stricmp(li2->fullname, li1->fullname);

    AROS_USERFUNC_EXIT
}


// =======================================================================================

AROS_UFH3S(void, openwbobj_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[Find::openwbobject_func] called\n"));

    // struct FindGroup_DATA *data = h->h_Data;
    struct Listentry *entry;
    BPTR filelock = (BPTR)-1;
    BPTR parentdirlock = (BPTR)-1;
    BPTR olddirlock = (BPTR)-1;

    DoMethod(obj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &entry);
    if (entry)
    {
        D(bug("[Find::openwbobj_func] name %s\n", entry->fullname));
        
        // trying to change directory to file's parent directory
        filelock = Lock(entry->fullname, SHARED_LOCK);
        if (filelock)
        {
            parentdirlock = ParentDir(filelock);
            olddirlock = CurrentDir(parentdirlock);
        }

        D(bug("[Find::openwbobj_func] file %p parent %p olddir %p\n", filelock, parentdirlock, olddirlock));

        // execute program even if directory change failed
        if (OpenWorkbenchObject(entry->fullname, TAG_DONE) == FALSE)
        {
            MUI_Request(_app(obj), _win(obj), 0, _(MSG_APP_TITLE), _(MSG_OK), _(MSG_ERR_NO_FILE), entry->fullname);
        }

        if (olddirlock != (BPTR)-1)
        {
            CurrentDir(olddirlock);
        }
        if (parentdirlock != (BPTR)-1)
        {
            UnLock(parentdirlock);
        }
        if (filelock != (BPTR)-1)
        {
            UnLock(filelock);
        }
    }

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3S(void, view_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[Find::view_func] called\n"));

    // struct FindGroup_DATA *data = h->h_Data;
    struct Listentry *entry;
    TEXT command[PATHNAMESIZE];
    BPTR con;

    DoMethod(obj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &entry);
    if (entry)
    {
        D(bug("[Find::view_func] viewing %s\n", entry->fullname));
        con = Open("CON:////Find Output/CLOSE/AUTO/WAIT", MODE_OLDFILE);
        snprintf(command, sizeof command, "SYS:Utilities/Multiview \"%s\"", entry->fullname);
        if (SystemTags(command, 
            SYS_Asynch, TRUE,
            SYS_Input,  con,
            SYS_Output, NULL,
            SYS_Error, NULL,
            TAG_DONE) == -1)
        {
            display_doserror(_app(obj), IoErr());
            Close(con); // an error occured, we must close con: ourselves
        }
    }

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3S(void, parent_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[Find::parent_func] called\n"));

    // struct FindGroup_DATA *data = h->h_Data;
    struct Listentry *entry;
    BPTR filelock;
    BPTR parentdirlock;
    TEXT buffer[PATHNAMESIZE];

    DoMethod(obj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &entry);
    if (entry)
    {
        D(bug("[Find::parent_func] name %s\n", entry->fullname));

        filelock = Lock(entry->fullname, SHARED_LOCK);
        if (filelock)
        {
            parentdirlock = ParentDir(filelock);
            if (NameFromLock(parentdirlock, buffer, PATHNAMESIZE))
            {
                D(bug("[Find::parent_func] parent %s\n", buffer));

                if (OpenWorkbenchObject(buffer, TAG_DONE) == FALSE)
                {
                    MUI_Request(_app(obj), _win(obj), 0, _(MSG_APP_TITLE), _(MSG_OK),
                        _(MSG_ERR_NO_DIR), buffer);
                }
            }
            UnLock(parentdirlock);
            UnLock(filelock);
        }
    }

    AROS_USERFUNC_EXIT
}

// =======================================================================================

AROS_UFH3S(void, activeentry_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[activeentry_func] called\n"));

    struct FindGroup_DATA *data = h->h_Data;
    LONG entry;

    entry = XGET(obj, MUIA_List_Active);
    if (entry == MUIV_NList_Active_Off)
    {
        SET(data->btn_open, MUIA_Disabled, TRUE);
        SET(data->btn_view, MUIA_Disabled, TRUE);
        SET(data->btn_parent, MUIA_Disabled, TRUE);
    }
    else
    {
        SET(data->btn_open, MUIA_Disabled, FALSE);
        SET(data->btn_view, MUIA_Disabled, FALSE);
        SET(data->btn_parent, MUIA_Disabled, FALSE);
    }

    AROS_USERFUNC_EXIT
}

// =======================================================================================

Object *FindGroup__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *str_path, *str_pattern, *str_contents;
    Object *btn_start, *btn_stop, *btn_open, *btn_view, *btn_parent;
    Object *lst_result;
    Object *txt_status;
    STRPTR path = NULL;
    STRPTR pattern = NULL;
    STRPTR contents = NULL;

    struct TagItem *tstate = message->ops_AttrList;
    struct TagItem *tag = NULL;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_FindGroup_Path:
                path = (STRPTR)tag->ti_Data;
                break;

            case MUIA_FindGroup_Pattern:
                pattern = (STRPTR)tag->ti_Data;
                break;

            case MUIA_FindGroup_Contents:
                contents = (STRPTR)tag->ti_Data;
                break;
        }
    }

    if (path == NULL) path = "SYS:";
    if (pattern == NULL) pattern = "";
    if (contents == NULL) contents = "";

    list_constr_hook.h_Entry = (HOOKFUNC)list_constr_func;
    list_destr_hook.h_Entry = (HOOKFUNC)list_destr_func;
    list_display_hook.h_Entry = (HOOKFUNC)list_display_func;
    list_compare_hook.h_Entry = (HOOKFUNC)list_compare_func;


    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        Child, HGroup,
            Child, VGroup,
                Child, VGroup,
                    GroupFrame,
                    Child, ColGroup(2),
                        GroupFrame,
                        Child, Label(_(MSG_LBL_PATH)),
                        Child, PopaslObject,
                            MUIA_Popasl_Type , ASL_FileRequest,
                            ASLFR_TitleText, _(MSG_LBL_PATH),
                            MUIA_Popstring_String, str_path = StringObject,
                                StringFrame,
                                MUIA_String_Contents, path,
                                MUIA_CycleChain, 1,
                            End,
                            MUIA_Popstring_Button, PopButton(MUII_PopFile),
                        End,
                        Child, Label(_(MSG_LBL_PATTERN)),
                        Child, str_pattern = StringObject,
                            StringFrame,
                            MUIA_String_Contents, pattern,
                            MUIA_CycleChain, 1,
                        End,
                        Child, Label(_(MSG_LBL_CONTENTS)),
                        Child, str_contents = StringObject,
                            StringFrame,
                            MUIA_String_Contents, contents,
                            MUIA_CycleChain, 1,
                        End,                    
                    End,
                    Child, HGroup,
                        GroupFrame,
                        Child, HVSpace,
                        Child, btn_start = SimpleButton(_(MSG_BTN_START)),
                        Child, HVSpace,
                        Child, btn_stop = SimpleButton(_(MSG_BTN_STOP)),
                        Child, HVSpace,
                    End,
                End,
                Child, lst_result = NListviewObject,
                    MUIA_NListview_NList, NListObject,
                        MUIA_NList_Format, "BAR,P=\33r BAR,BAR,BAR,BAR,",
                        MUIA_NList_Title, TRUE,
                        MUIA_Frame, MUIV_Frame_InputList,
                        MUIA_NList_DisplayHook, &list_display_hook,
                        MUIA_NList_ConstructHook, &list_constr_hook,
                        MUIA_NList_DestructHook, &list_destr_hook,
                        MUIA_NList_CompareHook, &list_compare_hook,
                    End,
                End,
                Child, HGroup,
                    GroupFrame,
                    Child, HVSpace,
                    Child, btn_open = SimpleButton(_(MSG_BTN_OPEN)),
                    Child, HVSpace,
                    Child, btn_view = SimpleButton(_(MSG_BTN_VIEW)),
                    Child, HVSpace,
                    Child, btn_parent = SimpleButton(_(MSG_BTN_DRAWER)),
                    Child, HVSpace,
                End,
                Child, txt_status = TextObject,
                    TextFrame,
                End,
            End,
        End,
        TAG_MORE, (IPTR)message->ops_AttrList,
        TAG_DONE
    );

    if (self)
    {
        struct FindGroup_DATA *data = INST_DATA(CLASS, self);

        // embed the process object
        data->scanproc = MUI_NewObject(MUIC_Process,
            MUIA_Process_SourceClass , CLASS,
            MUIA_Process_SourceObject, self,
            MUIA_Process_Priority    , -1,
            MUIA_Process_AutoLaunch  , FALSE,
            TAG_DONE);

        data->str_path          = str_path;
        data->str_pattern       = str_pattern;
        data->str_contents      = str_contents;
        data->btn_start         = btn_start;
        data->btn_stop          = btn_stop;
        data->btn_open          = btn_open;
        data->btn_view          = btn_view;
        data->btn_parent        = btn_parent;
        data->lst_result        = lst_result;
        data->txt_status        = txt_status;

        data->openwbobj_hook.h_Entry = (HOOKFUNC)openwbobj_func;
        data->openwbobj_hook.h_Data = data;

        data->view_hook.h_Entry = (HOOKFUNC)view_func;
        data->view_hook.h_Data = data;

        data->parent_hook.h_Entry = (HOOKFUNC)parent_func;
        data->parent_hook.h_Data = data;

        data->activeentry_hook.h_Entry = (HOOKFUNC)activeentry_func;
        data->activeentry_hook.h_Data = data;

        SET(data->btn_open, MUIA_Disabled, TRUE);
        SET(data->btn_view, MUIA_Disabled, TRUE);
        SET(data->btn_parent, MUIA_Disabled, TRUE);

        DoMethod
        (
            data->btn_start, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 1, MUIM_FindGroup_Start
        );

        DoMethod
        (
            data->btn_stop, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 1, MUIM_FindGroup_Stop
        );

        DoMethod
        (
            data->btn_open, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->openwbobj_hook
        );

        DoMethod
        (
            data->btn_view, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->view_hook
        );

        DoMethod
        (
            data->btn_parent, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->parent_hook
        );

        DoMethod
        (
            data->lst_result, MUIM_Notify, MUIA_NList_DoubleClick, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->openwbobj_hook
        );

        DoMethod
        (
            data->lst_result, MUIM_Notify, MUIA_NList_Active, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->activeentry_hook
        );
    }
    return self;
}

// =======================================================================================

IPTR FindGroup__OM_DISPOSE(Class *CLASS, Object *self, Msg msg)
{
    struct FindGroup_DATA *data = INST_DATA(CLASS, self);
    MUI_DisposeObject(data->scanproc);
    return DoSuperMethodA(CLASS, self, msg);
}

// =======================================================================================

IPTR FindGroup__MUIM_Cleanup(Class *CLASS, Object *self, Msg msg)
{
    struct FindGroup_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->scanproc, MUIM_Process_Kill, 0);

    return DoSuperMethodA(CLASS, self, msg);
}

// =======================================================================================

IPTR FindGroup__MUIM_Process_Process(Class *CLASS, Object *self, struct MUIP_Process_Process *msg)
{
    // this is our sub process

    D(bug("[Find::search process] called\n"));

    struct FindGroup_DATA *data = INST_DATA(CLASS, self);

    struct AnchorPath *anchorpath;
    LONG error;
    ULONG destlen;
    STRPTR destpattern;
    ULONG methodid1 = 0, methodid2 = 0;

    STRPTR path = (STRPTR)XGET(data->str_path, MUIA_String_Contents);
    STRPTR srcpattern = (STRPTR)XGET(data->str_pattern, MUIA_String_Contents);
    STRPTR content = (STRPTR)XGET(data->str_contents, MUIA_String_Contents);

    destlen = strlen(srcpattern) * 2 + 2;
    destpattern = AllocVec(destlen, MEMF_ANY);
    if (destpattern)
    {
        if (ParsePatternNoCase(srcpattern, destpattern, destlen) < 0) // error
        {
            // app must be NULL to avoid deadlocks
            MUI_Request(NULL, NULL, 0, _(MSG_APP_TITLE), _(MSG_OK), _(MSG_ERR_PATTERN));
            FreeVec(destpattern);
            return 0;
        }
    }

    // we must use PushMethod because we are in a sub process
    methodid1 = DoMethod(_app(self), MUIM_Application_PushMethod,  data->lst_result, 1, MUIM_NList_Clear);

    if (anchorpath = AllocMem(sizeof(struct AnchorPath) + PATHNAMESIZE, MEMF_CLEAR))
    {
        anchorpath->ap_Strlen = PATHNAMESIZE;
        anchorpath->ap_Flags = APF_DODIR;

        if ((error = MatchFirst(path, anchorpath)) == 0)
        {
            do
            {
                if (*msg->kill)
                {
                    // process stopped by user
                    break;
                }

                if (anchorpath->ap_Flags & APF_DIDDIR)
                {
                    anchorpath->ap_Flags &= ~(APF_DODIR | APF_DIDDIR);
                }
                else
                {
                    struct Listentry *entry;
                    D(bug("found %s\n", anchorpath->ap_Buf));
                    if (checkfile(_app(self), anchorpath, destpattern, content))
                    {
                        entry = AllocVec(sizeof (struct Listentry), MEMF_CLEAR);
                        if (entry)
                        {
                            entry->fullname = StrDup(anchorpath->ap_Buf);
                            CopyMem(&anchorpath->ap_Info, &entry->fib,  sizeof (entry->fib));

                            // we must use PushMethod because we are in a sub process
                            methodid2 = DoMethod(_app(self), MUIM_Application_PushMethod,
                                self, 2, MUIM_FindGroup_AddEntry, entry);
                            if (!methodid2)
                            {
                                FreeVec(entry->fullname);
                                FreeVec(entry);
                            }
                        }
                    }
                    anchorpath->ap_Flags = APF_DODIR;
                }
            } while ((error = MatchNext(anchorpath)) == 0);
        }

        MatchEnd(anchorpath);

        if (!*msg->kill && error != ERROR_NO_MORE_ENTRIES)
        {
            // app must be NULL to avoid deadlocks
            display_doserror(NULL, error);
        }

        FreeMem(anchorpath, sizeof(struct AnchorPath) + PATHNAMESIZE);
    }
    
    FreeVec(destpattern);

    // prevent method execution after task has been killed
    DoMethod(_app(self), MUIM_Application_UnpushMethod, self, methodid1, 0);
    DoMethod(_app(self), MUIM_Application_UnpushMethod, self, methodid2, 0);

    return 0;
}

// =======================================================================================

IPTR FindGroup__MUIM_FindGroup_Start(Class *CLASS, Object *self, Msg msg)
{
    struct FindGroup_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->scanproc, MUIM_Process_Launch);

    return 0;
}

// =======================================================================================

IPTR FindGroup__MUIM_FindGroup_Stop(Class *CLASS, Object *self, Msg msg)
{
    struct FindGroup_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->scanproc, MUIM_Process_Kill, 4);

    return 0;
}

// =======================================================================================

IPTR FindGroup__MUIM_FindGroup_AddEntry(Class *CLASS, Object *self, struct MUIP_FindGroup_AddEntry *msg)
{
    D(bug("[Find::MUIM_FindGroup_AddEntry] started\n"));
    struct FindGroup_DATA *data = INST_DATA(CLASS, self);

    DoMethod(data->lst_result, MUIM_List_InsertSingle, msg->entry, MUIV_List_Insert_Sorted);
    
    // we were called with PushMethod. After inserting the entry we can release the data
    FreeVec(msg->entry->fullname);
    FreeVec(msg->entry);

    return TRUE;
}

// =======================================================================================

ZUNE_CUSTOMCLASS_7
(
    FindGroup, NULL, MUIC_Group, NULL,
    OM_NEW,                     struct opSet *,
    OM_DISPOSE,                 Msg,
    MUIM_Cleanup,               Msg,
    MUIM_Process_Process,       struct MUIP_Process_Process *,
    MUIM_FindGroup_Start,       Msg,
    MUIM_FindGroup_Stop,        Msg,
    MUIM_FindGroup_AddEntry,    struct MUIP_FindGroup_AddEntry *
);
