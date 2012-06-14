/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/
#define DEBUG 1
#include <aros/debug.h>

#include <libraries/mui.h>
#include <libraries/asl.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/asl.h>
#include <proto/dos.h>

#include <stdio.h>

#include "r.h"
#include "locale.h"

Object *application, *window;
struct Hook execute_btn_hook, pop_single_btn_hook, pop_multi_btn_hook;
struct FileRequester *request;

AROS_UFH3S(void, execute_btn_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(struct Req **, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[R/execute_btn_func] msg %p *msg %p\n", msg, *msg));
    (*msg)->do_execute = TRUE;
    DoMethod(application, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(void, pop_single_btn_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(Object **, str_object, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[R/pop_single_btn_func] msg %p *msg %p\n", str_object, *str_object));

    if
    (
        AslRequestTags
        (
            request,
            ASLFR_DoMultiSelect, FALSE,
            ASLFR_InitialFile, "",
            ASLFR_InitialDrawer, "",
            TAG_DONE
        )
    )
    {
        ULONG buflen = strlen(request->fr_Drawer) + strlen(request->fr_File) + 10;
        TEXT *buffer = AllocPooled(poolmem, buflen);
        if (buffer)
        {
            strcpy(buffer, request->fr_Drawer);
            AddPart(buffer, request->fr_File, buflen);
            SET(*str_object, MUIA_String_Contents, buffer);
            FreePooled(poolmem, buffer, buflen);
        }
    }

    AROS_USERFUNC_EXIT
}


AROS_UFH3S(void, pop_multi_btn_func,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(Object **, str_object, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[R/pop_multi_btn_func] msg %p *msg %p\n", str_object, *str_object));

    if
    (
        AslRequestTags
        (
            request,
            ASLFR_DoMultiSelect, TRUE,
            ASLFR_InitialFile, "",
            ASLFR_InitialDrawer, "",
            TAG_DONE
        )
    )
    {
        LONG i;
        ULONG buflen = 10;
        for (i = 0; i < request->fr_NumArgs; i++)
        {
            buflen += strlen(request->fr_Drawer) + strlen(request->fr_ArgList[i].wa_Name) + 5;
        }

        TEXT *buffer = AllocPooled(poolmem, buflen);
        if (buffer)
        {
            TEXT *ptr = buffer;
            *ptr = '\0';
            for (i = 0; i < request->fr_NumArgs; i++)
            {
                *ptr++ = '\"';
                strcpy(ptr, request->fr_Drawer);
                AddPart(ptr, request->fr_ArgList[i].wa_Name, buffer + buflen - ptr - 2);
                ptr = buffer + strlen(buffer);
                strcpy(ptr, "\" ");
                ptr += 2;
            }
            SET(*str_object, MUIA_String_Contents, buffer);
            FreePooled(poolmem, buffer, buflen);
        }
    }

    AROS_USERFUNC_EXIT
}


BOOL create_gui(struct Req *req)
{
    Object *ok_btn, *cancel_btn, *str_group, *chk_group;
    struct TagItem str_group_tags[50];
    struct TagItem chk_group_tags[50];
    TEXT filename[256];
    LONG i, str_cnt, chk_cnt;

    request = AllocAslRequest(ASL_FileRequest, NULL);
    if (request == NULL)
        return FALSE;

    execute_btn_hook.h_Entry = (HOOKFUNC)execute_btn_func;
    pop_single_btn_hook.h_Entry = (HOOKFUNC)pop_single_btn_func;
    pop_multi_btn_hook.h_Entry = (HOOKFUNC)pop_multi_btn_func;

    D
    (
        bug(" i Name                      A F K M N S T\n");
        for (i = 0; i < req->arg_cnt; i++)
        {
            bug
            (
                "%2d %25s %c %c %c %c %c %c\n",
                i, req->cargs[i].argname,
                req->cargs[i].a_flag ? 'X' : '-',
                req->cargs[i].f_flag ? 'X' : '-',
                req->cargs[i].k_flag ? 'X' : '-',
                req->cargs[i].m_flag ? 'X' : '-',
                req->cargs[i].n_flag ? 'X' : '-',
                req->cargs[i].s_flag ? 'X' : '-',
                req->cargs[i].t_flag ? 'X' : '-'
            );
        }
    )

    for
    (
        i = 0, str_cnt = 0, chk_cnt = 0;
        i < req->arg_cnt && str_cnt < 40 && chk_cnt < 40;
        i++
    )
    {
        Object *new_obj = NULL;
        Object *new_arg_obj = NULL; // the object with the argument's value

        if (req->cargs[i].s_flag || req->cargs[i].t_flag) // Switch
        {
            new_obj = Label1(req->cargs[i].argname);
            if (new_obj)
            {
                chk_group_tags[chk_cnt].ti_Tag = Child;
                chk_group_tags[chk_cnt].ti_Data = (IPTR)new_obj;
                chk_cnt++;
            }
            new_arg_obj = MUI_MakeObject(MUIO_Checkmark, NULL);
            if (new_arg_obj)
            {
                chk_group_tags[chk_cnt].ti_Tag = Child;
                chk_group_tags[chk_cnt].ti_Data = (IPTR)new_arg_obj;
                req->cargs[i].object = new_arg_obj;
                chk_cnt++;
            }
        }
        else if (req->cargs[i].n_flag) // Numeric
        {
            new_obj = Label2(req->cargs[i].argname);
            if (new_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                str_cnt++;
            }
            new_arg_obj = StringObject,
                StringFrame,
                MUIA_CycleChain, 1,
                MUIA_String_Accept , "+-0123456879",
            End;
            if (new_arg_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_arg_obj;
                req->cargs[i].object = new_arg_obj;
                str_cnt++;
            }
        }
        else if (req->cargs[i].m_flag) // Multiple
        {
            Object *pop_btn;
            new_obj = Label2(req->cargs[i].argname);
            if (new_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                str_cnt++;
            }
            new_obj = HGroup,
                Child, new_arg_obj = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End,
                Child, pop_btn = PopButton(MUII_PopFile),
            End;
            if (new_obj)
            {
                D(bug("[R] pop %p string %p\n", pop_btn, new_arg_obj));
                DoMethod
                (
                    pop_btn, MUIM_Notify, MUIA_Pressed, FALSE,
                    (IPTR)pop_btn, 3, MUIM_CallHook, &pop_multi_btn_hook, new_arg_obj
                );
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                req->cargs[i].object = new_arg_obj;
                str_cnt++;
            }
        }
        else
        {
            Object *pop_btn;
            new_obj = Label2(req->cargs[i].argname);
            if (new_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                str_cnt++;
            }
            new_obj = HGroup,
                Child, new_arg_obj = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End,
                Child, pop_btn = PopButton(MUII_PopFile),
            End;
            if (new_obj)
            {
                D(bug("[R] pop %p string %p\n", pop_btn, new_arg_obj));
                DoMethod
                (
                    pop_btn, MUIM_Notify, MUIA_Pressed, FALSE,
                    (IPTR)pop_btn, 3, MUIM_CallHook, &pop_single_btn_hook, new_arg_obj
                );
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                req->cargs[i].object = new_arg_obj;
                str_cnt++;
            }
        }
    }

    // create string group
    str_group_tags[str_cnt].ti_Tag = MUIA_Group_Columns;
    str_group_tags[str_cnt].ti_Data = 2;
    str_cnt++;
    str_group_tags[str_cnt].ti_Tag = TAG_DONE;
    str_group = MUI_NewObjectA(MUIC_Group, str_group_tags);

    // fill empty check slots
    for (i = 0; i < chk_cnt % 8; i++)
    {
        chk_group_tags[chk_cnt].ti_Tag = Child;
        chk_group_tags[chk_cnt].ti_Data = (IPTR)HVSpace;
        chk_cnt++;
    }

    // create check group
    chk_group_tags[chk_cnt].ti_Tag = MUIA_Group_Columns;
    chk_group_tags[chk_cnt].ti_Data = 8;
    chk_cnt++;
    chk_group_tags[chk_cnt].ti_Tag = TAG_DONE;
    chk_group = MUI_NewObjectA(MUIC_Group, chk_group_tags);

    sprintf(filename, _(MSG_WIN_TITLE), req->filename);
    application = (Object *)ApplicationObject,
        MUIA_Application_Title, __(MSG_APP_TITLE),
        MUIA_Application_Version, (IPTR) VERSION,
        MUIA_Application_Description, __(MSG_APP_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "REQUEST",
        MUIA_Application_UseCommodities, FALSE,
        SubWindow, (IPTR)(window = (Object *)WindowObject,
            MUIA_Window_Title, (IPTR) filename,
            MUIA_Window_ID, MAKE_ID('R','E','Q','U'),
            WindowContents, VGroup,
                Child, VGroup,
                    GroupFrame,
                    Child, str_group,
                    Child, HVSpace,
                    Child, chk_group,
                End,
                Child, (IPTR) (RectangleObject, 
                    MUIA_Rectangle_HBar, TRUE,
                    MUIA_FixHeight,      2,
                End),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, ok_btn = SimpleButton(_(MSG_OK)),
                    Child, HVSpace,
                    Child, cancel_btn = SimpleButton(_(MSG_CANCEL)),
                End,
            End,
        End),
    End;

    if (application)
    {
        DoMethod
        (
            window,
            MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR)application, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );

        DoMethod
        (
            cancel_btn,
            MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)application, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );

        DoMethod
        (
            ok_btn,
            MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)application, 3,
            MUIM_CallHook, &execute_btn_hook, req
        );

        SET(window, MUIA_Window_Open, TRUE);
    
        return TRUE;
    }
    return FALSE;
}


BOOL handle_gui(struct Req *req)
{
    DoMethod(application, MUIM_Application_Execute);
    SET(window, MUIA_Window_Open, FALSE);
    return req->do_execute;
}


BOOL get_gui_bool(struct CArg *carg)
{
    return XGET(carg->object, MUIA_Selected);
}


CONST_STRPTR get_gui_string(struct CArg *carg)
{
    return (CONST_STRPTR)XGET(carg->object, MUIA_String_Contents);
}


void cleanup_gui(void)
{
    if (application) MUI_DisposeObject(application);
    if (request) FreeAslRequest(request);
}
