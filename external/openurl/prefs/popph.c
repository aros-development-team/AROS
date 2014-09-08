/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "openurl.h"

#include <exec/execbase.h>
#include <libraries/asl.h>

#include "SDI_hook.h"
#include "macros.h"

#include "debug.h"

/**************************************************************************/
/*
** Place holders list
*/

static struct MUI_CustomClass *listClass = NULL;
#define listObject NewObject(listClass->mcc_Class,NULL

struct listData
{
    STRPTR *phs;
    STRPTR *names;
    struct Hook dispHook;
};

/**************************************************************************/

HOOKPROTONHNO(conFun, ULONG, ULONG num)
{
    return num+1;
}
MakeStaticHook(conHook, conFun);

/**************************************************************************/

HOOKPROTO(dispFun, void, STRPTR *array, ULONG num)
{
    struct listData *data = hook->h_Data;

    if (num)
    {
        num--;

        *array++ = data->phs[num];
        *array   = data->names[num];
    }
}
MakeStaticHook(dispHook, dispFun);

/**************************************************************************/

static IPTR
mListNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *attrs = msg->ops_AttrList;
    STRPTR *phs, *names;

    phs   = (STRPTR *)GetTagData(MUIA_Popph_Syms,(IPTR)NULL,attrs);
    names = (STRPTR *)GetTagData(MUIA_Popph_Names,(IPTR)NULL,attrs);
    if (!phs || !names) return 0;

    if((obj = (Object *)DoSuperNew(cl,obj,
            InputListFrame,
            MUIA_List_Format,        ",",
            MUIA_List_Pool,          g_pool,
            MUIA_List_ConstructHook, &conHook,
            TAG_MORE, attrs)) != NULL)
    {
        struct listData *data = INST_DATA(cl,obj);
        int             i;

        data->phs   = phs;
        data->names = names;

        InitHook(&data->dispHook, dispHook, data);

        superset(cl,obj,MUIA_List_DisplayHook,&data->dispHook);

        conHook.h_Data  = data;

        for (i = 0; phs[i]; i++)
            DoSuperMethod(cl,obj,MUIM_List_InsertSingle,i,MUIV_List_Insert_Bottom);
    }

    return (IPTR)obj;
}

/**************************************************************************/

SDISPATCHER(listDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return mListNew(cl,obj,(APTR)msg);

        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

static BOOL initListClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((listClass = MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct listData), ENTRY(listDispatcher))) != NULL)
        success = TRUE;

    RETURN(success);
    return success;
}

/**************************************************************************/

static void disposeListClass(void)
{
    if(listClass != NULL)
        MUI_DeleteCustomClass(listClass);
}

/**************************************************************************/

struct data
{
    Object               *str;

    struct Hook          closeHook;
    struct FileRequester *req;

    STRPTR               *phs;
    STRPTR               *names;
};

/**************************************************************************/

HOOKPROTONH(windowFun, void, Object *pop, Object *win)
{
  set(win,MUIA_Window_DefaultObject,pop);
}
MakeStaticHook(windowHook, windowFun);

/***********************************************************************/

HOOKPROTO(closeFun, void, Object *list, Object *str)
{
    struct data *data = hook->h_Data;
    LONG       a;

    a = xget(list,MUIA_List_Active);
    if (a>=0)
    {
        STRPTR buf, x;
        ULONG pos, lx, l;

        pos = xget(str, MUIA_String_BufferPos);
        x = (STRPTR)xget(str, MUIA_String_Contents);

        lx = strlen(x);
        l  = strlen(data->phs[a]);

        if((buf = AllocPooled(g_pool,lx+l+1)) != NULL)
        {
            if (pos>0) CopyMem(x,buf,pos);
            CopyMem(data->phs[a],buf+pos,l);
            if (lx) CopyMem(x+pos,buf+pos+l,lx-pos+1);
            set(str,MUIA_String_Contents,buf);
            FreePooled(g_pool,buf,lx+l+1);
        }
    }
}
MakeStaticHook(closeHook, closeFun);

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object         *str, *lv;
    struct TagItem *attrs = msg->ops_AttrList;
    STRPTR         *phs, *names;

    phs   = (STRPTR*)GetTagData(MUIA_Popph_Syms,(IPTR)NULL,attrs);
    if (!phs) return 0;

    names = (STRPTR*)GetTagData(MUIA_Popph_Names,FALSE,attrs);
    if (!names) return 0;

    if((obj = (Object *)DoSuperNew(cl,obj,
            MUIA_Group_Horiz,        TRUE,
            MUIA_Group_HorizSpacing, 1,

            Child, PopobjectObject,
                MUIA_Popstring_String, str = ostring(GetTagData(MUIA_Popph_MaxLen,0,attrs),GetTagData(MUIA_Popph_Key,(IPTR)NULL,attrs),0),
                MUIA_Popstring_Button, opopbutton(MUII_PopUp,0),
                MUIA_Popobject_Object, lv = ListviewObject,
                    MUIA_Listview_List, listObject,
                        MUIA_Popph_Syms,  phs,
                        MUIA_Popph_Names, names,
                    End,
                End,
                MUIA_Popobject_WindowHook, &windowHook,
            End,

            TAG_MORE, attrs)) != NULL)
    {

        struct data *data = INST_DATA(cl,obj);

        data->str = str;

        data->phs   = phs;
        data->names = names;

        InitHook(&data->closeHook, closeHook, data);
        set(obj,MUIA_Popobject_ObjStrHook,&data->closeHook);

        if (GetTagData(MUIA_Popph_Asl,FALSE,attrs))
        {
            APTR req;

            if((req = MUI_AllocAslRequest(ASL_FileRequest, NULL)) != NULL)
            {
                Object *bt;

                if((bt = opopbutton(MUII_PopFile, 0)) != NULL)
                {
                    DoSuperMethod(cl,obj,OM_ADDMEMBER,(IPTR)bt);

                    data->req = req;

                    DoMethod(bt,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_Popph_RequestFile);
                }
                else MUI_FreeAslRequest(req);
            }
        }

        DoMethod(lv,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,(IPTR)obj,2,MUIM_Popstring_Close,TRUE);
    }

    return (IPTR)obj;
}

/***********************************************************************/

static IPTR mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    if (data->req) MUI_FreeAslRequest(data->req);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

HOOKPROTONO(reqIntuiFun, void, struct IntuiMessage *imsg)
{
  if(imsg->Class==IDCMP_REFRESHWINDOW)
    DoMethod(hook->h_Data,MUIM_Application_CheckRefresh);
}
MakeStaticHook(reqIntuiHook, reqIntuiFun);

static IPTR mRequestFile(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    struct Hook intuiHook;
    TEXT       path[256], *x, *file, *p;

    set(_app(obj),MUIA_Application_Sleep,TRUE);

    InitHook(&intuiHook, reqIntuiHook, _app(obj));

    x = (STRPTR)xget(data->str, MUIA_String_Contents);
    file = FilePart(x);
    if((p = PathPart(x)) != NULL)
    {
        strlcpy(path, x, p-x+1);
        p = path;
    }

    if (MUI_AslRequestTags(data->req,
                           ASLFR_InitialFile,       (IPTR)file,
                           p ? ASLFR_InitialDrawer : TAG_IGNORE, (IPTR)p,
                           ASLFR_IntuiMsgFunc,      (IPTR)&intuiHook,
                           ASLFR_Window,            (IPTR)_window(obj),
                           ASLFR_PrivateIDCMP,      TRUE,
                           ASLFR_Flags1,            FRF_INTUIFUNC,
                           TAG_DONE))
    {
        TEXT buf[256];

        strlcpy(buf, data->req->fr_Drawer, sizeof(buf));
        AddPart(buf, data->req->fr_File, sizeof(buf));

        if (*buf) set(data->str,MUIA_String_Contents,buf);
    }
    else if (IoErr()) DisplayBeep(0);

    set(_app(obj),MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                 return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:             return mDispose(cl,obj,(APTR)msg);

        case MUIM_Popph_RequestFile: return mRequestFile(cl,obj,(APTR)msg);

        default:                     return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/

BOOL initPopphClass(void)
{
    BOOL success = TRUE;

    ENTER();

    if(initListClass() == TRUE)
    {
        if((g_popphClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
            success = TRUE;
        else
            disposeListClass();
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposePopphClass(void)
{
    ENTER();

    disposeListClass();
    if(g_popphClass != NULL)
        MUI_DeleteCustomClass(g_popphClass);

    LEAVE();
}

/**************************************************************************/

