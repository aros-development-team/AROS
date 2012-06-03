/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>
#include <libraries/asl.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/alib.h>

#include <stdio.h>

#define DEBUG 1
#include <aros/debug.h>

#include "r.h"

#define ID_OK (99)

Object *application, *window;
struct Hook execute_btn_hook;


AROS_UFH3(void, execute_btn_func,
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


BOOL create_gui(struct Req *req)
{
    Object *ok_btn, *cancel_btn, *str_group, *chk_group;
    struct TagItem str_group_tags[50];
    struct TagItem chk_group_tags[50];

    LONG i, str_cnt, chk_cnt;

    execute_btn_hook.h_Entry = (HOOKFUNC)execute_btn_func;

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
            new_obj = Label(req->cargs[i].argname);
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
            new_obj = Label(req->cargs[i].argname);
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
            new_obj = Label(req->cargs[i].argname);
            if (new_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                str_cnt++;
            }
            new_arg_obj = PopaslObject,
                MUIA_Popasl_Type, ASL_FileRequest,
                ASLFR_DoMultiSelect, TRUE,
                MUIA_Popstring_String, (IPTR)(new_arg_obj = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopUp),
            End;
            if (new_arg_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_arg_obj;
                req->cargs[i].object = new_arg_obj;
                str_cnt++;
            }
        }
        else
        {
            new_obj = Label(req->cargs[i].argname);
            if (new_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_obj;
                str_cnt++;
            }
            new_arg_obj = PopaslObject,
                MUIA_Popasl_Type, ASL_FileRequest,
                MUIA_Popstring_String, (IPTR)(new_arg_obj = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                End),
                MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopUp),
            End;
            if (new_arg_obj)
            {
                str_group_tags[str_cnt].ti_Tag = Child;
                str_group_tags[str_cnt].ti_Data = (IPTR)new_arg_obj;
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

    application = (Object *)ApplicationObject,
        MUIA_Application_Title, "R", //__(MSG_TITLE),
        MUIA_Application_Version, (IPTR) VERSION,
        MUIA_Application_Description, "R", //__(MSG_TITLE),
        MUIA_Application_Base, (IPTR) "REQUEST",
        MUIA_Application_UseCommodities, FALSE,
        SubWindow, (IPTR)(window = (Object *)WindowObject,
            MUIA_Window_Title, "R - The GUI Generator", //__(MSG_TITLE),
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
                    Child, ok_btn = SimpleButton("Execute"),
                    Child, HVSpace,
                    Child, cancel_btn = SimpleButton("Cancel"),
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
}
