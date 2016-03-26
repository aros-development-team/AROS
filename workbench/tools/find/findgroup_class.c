/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
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
//#include <mui/NList_mcc.h>
//#include <mui/NListview_mcc.h>

#define DEBUG 1
#include <aros/debug.h>
#include <zune/customclasses.h>

#include <stdio.h>

#include "findgroup_class.h"
#include "locale.h"

#define PATHNAMESIZE (1024)


struct FindGroup_DATA
{
    Object *str_path, *str_pattern, *str_contents;
    Object *btn_start, *btn_stop;
    Object *lst_result;
    Object *txt_status;
    struct Hook search_hook;
    struct Hook openwbobj_hook;
};


struct Listentry
{
    STRPTR fullname;
    ULONG namebuflen;
    struct FileInfoBlock fib;
};

static struct Hook list_display_hook, list_constr_hook, list_destr_hook;


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


AROS_UFH3S(LONG, list_display_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct Listentry *, li, A1))
{
    AROS_USERFUNC_INIT

    if (li)
    {
        static TEXT protbuf[8], sizebuf[20];
        static TEXT datebuf[10], timebuf[10];
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
        *array++ = "Full Path";
        *array++ = "Size";
        *array++ = "Attributes";
        *array++ = "Date";
        *array++ = "Time";
        *array++ = "Comment";
    }

    return 0;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, search_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[search_func] called\n"));

    struct FindGroup_DATA *data = h->h_Data;

    struct AnchorPath *anchorpath;
    LONG error;
    BPTR dirlock;
    BPTR oldlock = (BPTR)-1;

    STRPTR path = (STRPTR)XGET(data->str_path, MUIA_String_Contents);
    STRPTR pattern = (STRPTR)XGET(data->str_pattern, MUIA_String_Contents);
    STRPTR content = (STRPTR)XGET(data->str_contents, MUIA_String_Contents);

    dirlock = Lock(path, SHARED_LOCK);
    if (dirlock == 0)
    {
        puts("Can't lock directory"); // TODO: requester
        return;
    }

    oldlock = CurrentDir(dirlock);

    SET(data->btn_start, MUIA_Disabled, TRUE);
    SET(data->btn_stop, MUIA_Disabled, FALSE);
    SET(data->txt_status, MUIA_Text_Contents, "Searching...");
    DoMethod(data->lst_result, MUIM_List_Clear);

    if (anchorpath = AllocMem(sizeof(struct AnchorPath) + PATHNAMESIZE, MEMF_CLEAR))
    {
        anchorpath->ap_Strlen = PATHNAMESIZE;
        //anchorpath->ap_BreakBits = SIGBREAKF_CTRL_C;
        anchorpath->ap_Flags = APF_DODIR;
        if ((error = MatchFirst(pattern, anchorpath)) == 0)
        {
            do
            {
                if (anchorpath->ap_Flags & APF_DIDDIR)
                {
                    anchorpath->ap_Flags &= ~(APF_DODIR | APF_DIDDIR);
                }
                else
                {
                    struct Listentry entry;
                    if (anchorpath->ap_Info.fib_DirEntryType < 0) // only files
                    {
                        entry.fullname = anchorpath->ap_Buf;
                        entry.fib = anchorpath->ap_Info;
                        DoMethod(data->lst_result, MUIM_List_InsertSingle, &entry, MUIV_List_Insert_Bottom);
                    }
                    anchorpath->ap_Flags = APF_DODIR;
                }
            } while ((error = MatchNext(anchorpath)) == 0);
        }

        MatchEnd(anchorpath);

        if (error != ERROR_NO_MORE_ENTRIES)
            PrintFault(error, NULL);

        FreeMem(anchorpath, sizeof(struct AnchorPath) + PATHNAMESIZE);
    }
    
    SET(data->btn_start, MUIA_Disabled, FALSE);
    SET(data->btn_stop, MUIA_Disabled, TRUE);
    SET(data->txt_status, MUIA_Text_Contents, "");

    if (oldlock != (BPTR)-1)
    {
        CurrentDir(oldlock);
    }

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(void, openwbobj_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[openwbobject_func] called\n"));

    // struct FindGroup_DATA *data = h->h_Data;
    struct Listentry *entry;
    
    DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);
    if (entry)
    {
        OpenWorkbenchObject(entry->fullname, TAG_DONE);
    }

    AROS_USERFUNC_EXIT
}


Object *FindGroup__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *str_path, *str_pattern, *str_contents;
    Object *btn_start, *btn_stop;
    Object *lst_result;
    Object *txt_status;

    list_constr_hook.h_Entry = (HOOKFUNC)list_constr_func;
    list_destr_hook.h_Entry = (HOOKFUNC)list_destr_func;
    list_display_hook.h_Entry = (HOOKFUNC)list_display_func;


    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        Child, HGroup,
            Child, VGroup,
                Child, ColGroup(2),
                    GroupFrame,
                    Child, Label("Path"),
                    Child, PopaslObject,
                        MUIA_Popasl_Type , ASL_FileRequest,
                        ASLFR_TitleText, "Path",
                        MUIA_Popstring_String, str_path = StringObject,
                            StringFrame,
                            MUIA_String_Contents, "SYS:",
                        End,
                    MUIA_Popstring_Button, PopButton(MUII_PopFile),
                    End,
                    Child, Label("Pattern"),
                    Child, str_pattern = StringObject,
                        StringFrame,
                    End,
                    Child, Label("Contents"),
                    Child, str_contents = StringObject,
                        StringFrame,
                    End,                    
                End,
                Child, lst_result = ListviewObject,
                    MUIA_Listview_List, ListObject,
                        MUIA_List_Format, "BAR,P=\33r BAR,BAR,BAR,BAR,",
                        MUIA_List_Title, TRUE,
                        MUIA_Frame, MUIV_Frame_InputList,
                        MUIA_List_DisplayHook, &list_display_hook,
                        MUIA_List_ConstructHook, &list_constr_hook,
                        MUIA_List_DestructHook, &list_destr_hook,
                    End,
                End,
                Child, txt_status = TextObject,
                    TextFrame,
                End,
                Child, HGroup,
                    GroupFrame,
                    Child, HVSpace,
                    Child, btn_start = SimpleButton("Start"),
                    Child, HVSpace,
                    Child, btn_stop = SimpleButton("Stop"),
                    Child, HVSpace,
                End,
            End,
        End,
        TAG_MORE, (IPTR)message->ops_AttrList,
        TAG_DONE
    );

    if (self)
    {
        struct FindGroup_DATA *data = INST_DATA(CLASS, self);

        data->str_path          = str_path;
        data->str_pattern       = str_pattern;
        data->str_contents      = str_contents;
        data->btn_start         = btn_start;
        data->btn_stop          = btn_stop;
        data->lst_result        = lst_result;
        data->txt_status        = txt_status;

        data->search_hook.h_Entry = (HOOKFUNC)search_func;
        data->search_hook.h_Data = data;

        data->openwbobj_hook.h_Entry = (HOOKFUNC)openwbobj_func;
        data->openwbobj_hook.h_Data = data;

        SET(data->btn_stop, MUIA_Disabled, TRUE);
        SET(data->str_contents, MUIA_Disabled, TRUE);

        DoMethod
        (
            data->btn_start, MUIM_Notify, MUIA_Pressed, FALSE,
            self, 2, MUIM_CallHook, &data->search_hook
        );

        DoMethod
        (
            data->lst_result, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
            self, 2, MUIM_CallHook, &data->openwbobj_hook
        );
    }
    return self;
}


ZUNE_CUSTOMCLASS_1
(
    FindGroup, NULL, MUIC_Group, NULL,
    OM_NEW,          struct opSet *
);
