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

BOOL create_gui(struct Req *req)
{
    Object *group, *ok_btn, *cancel_btn;
    struct TagItem group_tags[200];

    LONG i, n;

    D
    (
        bug(" i Name                      A F K M N S\n");
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
                req->cargs[i].s_flag ? 'X' : '-'
            );
        }
    )

    for (i = 0, n = 0; i < req->arg_cnt && n < 150; i++)
    {
        Object *new_obj = NULL;
        Object *new_arg_obj = NULL; // the object with the argument's value

        if (req->cargs[i].s_flag)
        {
            new_obj = HGroup,
                Child, Label(req->cargs[i].argname),
                Child, (IPTR)(new_arg_obj = MUI_MakeObject(MUIO_Checkmark, NULL)),
            End;
        }
        else if (req->cargs[i].n_flag)
        {
            new_obj = HGroup,
                Child, Label(req->cargs[i].argname),
                Child, (IPTR)(new_arg_obj = StringObject,
                    StringFrame,
                    MUIA_CycleChain, 1,
                    MUIA_String_Accept , "0123456879",
                End),
            End;
        }
        else if (req->cargs[i].m_flag)
        {
            new_obj = HGroup,
                Child, Label(req->cargs[i].argname),
                Child, (IPTR)PopaslObject,
                    MUIA_Popasl_Type, ASL_FileRequest,
                    ASLFR_DoMultiSelect, TRUE,
                    MUIA_Popstring_String, (IPTR)(new_arg_obj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),
                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopUp),
                End,
            End;
        }
        else
        {
            new_obj = HGroup,
                Child, Label(req->cargs[i].argname),
                Child, (IPTR)PopaslObject,
                    MUIA_Popasl_Type, ASL_FileRequest,
                    MUIA_Popstring_String, (IPTR)(new_arg_obj = StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                    End),
                    MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopUp),
                End,
            End;
        }
        if (new_obj)
        {
            group_tags[n].ti_Tag = Child;
            group_tags[n].ti_Data = (IPTR)new_obj;
            req->cargs[n].object = new_arg_obj;
            n++;
        }
    }
    group_tags[n].ti_Tag = MUIA_Frame;
    group_tags[n].ti_Data = MUIV_Frame_Group;
    n++;
    group_tags[n].ti_Tag = TAG_DONE;
    group = MUI_NewObjectA(MUIC_Group, group_tags);

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
                Child, (IPTR)group,
                Child, (IPTR) (RectangleObject, 
                    MUIA_Rectangle_HBar, TRUE,
                    MUIA_FixHeight,      2,
                End),
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, ok_btn = SimpleButton("OK"),
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
            (IPTR)application, 2,
            MUIM_Application_ReturnID, ID_OK
        );

        SET(window, MUIA_Window_Open, TRUE);
    
        return TRUE;
    }
    return FALSE;
}


BOOL handle_gui(void)
{
    ULONG sigs = 0;
    BOOL retval = FALSE;
    BOOL running = TRUE;

    while (running)
    {
        switch (DoMethod(application, MUIM_Application_Input, &sigs))
        {
            case ID_OK:
                running = FALSE;
                retval = TRUE;
                break;
            case MUIV_Application_ReturnID_Quit:
                running = FALSE;
                break;
        }
        if (running && sigs) Wait(sigs);
    }
    SET(window, MUIA_Window_Open, FALSE);
    return retval;
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
