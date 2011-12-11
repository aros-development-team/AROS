/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

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

#define CATCOMP_NUMBERS
#include "locale.h"
#include "macros.h"

#include <libraries/openurl.h>
#include <stdio.h>

#include <SDI/SDI_hook.h>

#include "debug.h"

/**************************************************************************/
/*
** Little lamp class for enabled/disabled
*/

static struct MUI_CustomClass *lampClass = NULL;
#define lampObject NewObject(lampClass->mcc_Class,NULL

struct lampData
{
    LONG               enabled;
    LONG               disabled;
    LONG               detail;
    WORD               delta;

    ULONG              flags;
};

enum
{
    FLG_LampSetup    = 1<<0,
    FLG_LampDisabled = 1<<1,
};

/***********************************************************************/

static IPTR mLampNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return (IPTR)DoSuperNew(cl,obj,
        MUIA_Font, MUIV_Font_List,
        TAG_MORE,  msg->ops_AttrList);
}

/**************************************************************************/

static IPTR mLampSets(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct lampData *data = INST_DATA(cl,obj);
    struct TagItem  *tag;

    if((tag = FindTagItem(MUIA_Lamp_Disabled,msg->ops_AttrList)))
    {
        if (tag->ti_Data)
            SET_FLAG(data->flags, FLG_LampDisabled);
        else
            CLEAR_FLAG(data->flags, FLG_LampDisabled);

        /* Of course, we don't redraw here */
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static IPTR mLampSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct lampData *data = INST_DATA(cl,obj);
    if (!DoSuperMethodA(cl,obj,(APTR)msg)) return FALSE;

    if (MUIMasterBase->lib_Version<20)
	{
    	data->enabled  = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"r00000000,ffffffff,00000000",0);
	    data->disabled = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"rffffffff,00000000,00000000",0);
	    data->detail   = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"r00000000,00000000,00000000",0);
    }
	else
    {
    	data->enabled  = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"r02ff00",0);
   		data->disabled = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"rff0000",0);
		data->detail   = MUI_ObtainPen(muiRenderInfo(obj),(APTR)"r000000",0);
	}

    SET_FLAG(data->flags, FLG_LampSetup);

    return TRUE;
}

/***********************************************************************/

static IPTR mLampCleanup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
    struct lampData *data = INST_DATA(cl,obj);

    if(isFlagSet(data->flags, FLG_LampSetup))
    {
        MUI_ReleasePen(muiRenderInfo(obj),data->enabled);
        MUI_ReleasePen(muiRenderInfo(obj),data->disabled);
        MUI_ReleasePen(muiRenderInfo(obj),data->detail);

        CLEAR_FLAG(data->flags, FLG_LampSetup);
    }

    return DoSuperMethodA(cl,obj,(APTR)msg);
}

/***********************************************************************/

static IPTR mLampAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct lampData          *data = INST_DATA(cl,obj);
    struct RastPort          rp;
    struct TextExtent        te;
    UWORD                    w, h, d;

    DoSuperMethodA(cl,obj,(APTR)msg);

    CopyMem(&_screen(obj)->RastPort,&rp,sizeof(rp));

    /* Don't ask or modify ! */

    SetFont(&rp,_font(obj));
    TextExtent(&rp,"  ",2,&te);

    w = te.te_Width;
    h = te.te_Height;

    if (w>=h) d = w;
    else d = h;

    data->delta = te.te_Extent.MinY;

    msg->MinMaxInfo->MinWidth  += d;
    msg->MinMaxInfo->MinHeight += h;
    msg->MinMaxInfo->DefWidth  += d;
    msg->MinMaxInfo->DefHeight += h;
    msg->MinMaxInfo->MaxWidth  += d;
    msg->MinMaxInfo->MaxHeight += h;

    return 0;
}

/***********************************************************************/

static IPTR mLampDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct lampData *data = INST_DATA(cl,obj);
    IPTR            res;

    res = DoSuperMethodA(cl,obj,(APTR)msg);

    if(hasFlag(msg->flags, (MADF_DRAWOBJECT|MADF_DRAWUPDATE)))
    {
        WORD l, t, r, b;

        /* Don't ask or modify ! */

        l = _mleft(obj);
        r = _mright(obj);
        t = _mtop(obj)+(_mheight(obj)+data->delta)/2-1;
        b = t-data->delta;

        if (r-l>2)
        {
            l += 1;
            r -= 1;
        }

        if (b-t>2)
        {
            t += 1;
            b -= 1;
        }

        SetAPen(_rp(obj), MUIPEN(isFlagSet(data->flags, FLG_LampDisabled) ? data->disabled : data->enabled));
        RectFill(_rp(obj),l,t,r,b);

        SetAPen(_rp(obj),MUIPEN(data->detail));
        Move(_rp(obj),l,t);
        Draw(_rp(obj),r,t);
        Draw(_rp(obj),r,b);
        Draw(_rp(obj),l,b);
        Draw(_rp(obj),l,t);
    }

    return res;
}

/***********************************************************************/

SDISPATCHER(lampDispatcher)
{
    switch(msg->MethodID)
    {
        case OM_NEW:         return mLampNew(cl,obj,(APTR)msg);
        case OM_SET:         return mLampSets(cl,obj,(APTR)msg);

        case MUIM_Setup:     return mLampSetup(cl,obj,(APTR)msg);
        case MUIM_Cleanup:   return mLampCleanup(cl,obj,(APTR)msg);
        case MUIM_AskMinMax: return mLampAskMinMax(cl,obj,(APTR)msg);
        case MUIM_Draw:      return mLampDraw(cl,obj,(APTR)msg);

        default:             return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/

static BOOL initLampClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((lampClass = MUI_CreateCustomClass(NULL, MUIC_Rectangle, NULL, sizeof(struct lampData), ENTRY(lampDispatcher))) != NULL)
       success = TRUE;

    RETURN(success);
    return success;
}

/**************************************************************************/

static void disposeLampClass(void)
{
    if(lampClass != NULL)
        MUI_DeleteCustomClass(lampClass);
}

/**************************************************************************/
/*
** List of clients with lamps
*/

static struct MUI_CustomClass *listClass = NULL;
#define listObject NewObject(listClass->mcc_Class,NULL

struct listData
{
    Object *olamp;
    APTR   lamp;
    TEXT   col0buf[NAME_LEN+16];

    ULONG  nameOfs;
    ULONG  pathOfs;
    ULONG  nodeSize;

    TEXT  format[64];

    ULONG  flags;

    struct Hook conHook;
    struct Hook desHook;
    struct Hook dispHook;
};

enum
{
    FLG_ListSetup = 1<<0,
};

/* Used for Import/Export */
struct listIO
{
    ULONG len;
    TEXT format[64];
};

/**************************************************************************/

HOOKPROTO(conFunc, struct URL_Node *, APTR pool, struct URL_Node *node)
{
    struct listData *data = hook->h_Data;
    struct URL_Node *new;

    if(isFlagSet(node->Flags, UNF_NTALLOC))
    {
      new = node;
      CLEAR_FLAG(node->Flags, UNF_NTALLOC);
    }
    else if((new = AllocPooled(pool,data->nodeSize)) != NULL)
      CopyMem(node, new, data->nodeSize);

    return new;
}
MakeStaticHook(conHook, conFunc);

/**************************************************************************/

HOOKPROTO(desFunc, void, APTR pool, struct URL_Node *node)
{
    struct listData *data = hook->h_Data;

    FreePooled(pool,node,data->nodeSize);
}
MakeStaticHook(desHook, desFunc);

/**************************************************************************/

HOOKPROTO(dispFunc, void, STRPTR *array, struct URL_Node *node)
{
    struct listData *data = hook->h_Data;

    if (node)
    {
        if (data->lamp)
        {
            set(data->olamp, MUIA_Lamp_Disabled, isFlagSet(node->Flags, UNF_DISABLED));
            #if defined(__LP64__)
            sprintf(data->col0buf,"\33O[%016lx]", (IPTR)data->lamp);
            #else
            sprintf(data->col0buf,"\33O[%08lx]", (IPTR)data->lamp);
            #endif
            *array++ = data->col0buf;
        }
        else
          *array++ = (STRPTR)(isFlagSet(node->Flags, UNF_DISABLED) ? " " : ">");

        *array++ = (STRPTR)node+data->nameOfs;
        *array   = (STRPTR)node+data->pathOfs;
    }
    else
    {
        *array++ = (STRPTR)" ";
        *array++ = getString(MSG_Edit_ListName);
        *array   = getString(MSG_Edit_ListPath);
    }
}
MakeStaticHook(dispHook, dispFunc);

/**************************************************************************/

static IPTR mListNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
  if((obj = (Object *)DoSuperNew(cl,obj,
            InputListFrame,
            MUIA_List_Title,         TRUE,
            MUIA_List_Format,        "C=0,C=1,C=2",
            MUIA_List_DragSortable,  TRUE,
            MUIA_List_Pool,          g_pool,
            MUIA_List_DragSortable,  TRUE,
            MUIA_List_ShowDropMarks, TRUE,
            TAG_MORE, msg->ops_AttrList)) != NULL)
  {
        struct listData *data = INST_DATA(cl,obj);

        // the hooks make use of the instance data and hence must not
        // put the data pointer into the global hook definitions
        InitHook(&data->conHook, conHook, data);
        InitHook(&data->desHook, desHook, data);
        InitHook(&data->dispHook, dispHook, data);

        // now tell the list object to use the local hooks
        set(obj, MUIA_List_ConstructHook, &data->conHook);
        set(obj, MUIA_List_DestructHook, &data->desHook);
        set(obj, MUIA_List_DisplayHook, &data->dispHook);

        data->nameOfs  = GetTagData(MUIA_AppList_NodeNameOffset,0,msg->ops_AttrList);
        data->pathOfs  = GetTagData(MUIA_AppList_NodePathOffset,0,msg->ops_AttrList);
        data->nodeSize = GetTagData(MUIA_AppList_NodeSize,0,msg->ops_AttrList);

        strlcpy(data->format,"C=0,C=1,C=2", sizeof(data->format));

        if (lampClass) data->olamp = lampObject, End;

    }

    return (IPTR)obj;
}

/**************************************************************************/

static IPTR mListDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct listData *data = INST_DATA(cl,obj);

    if (data->olamp) MUI_DisposeObject(data->olamp);

    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************/

static IPTR mListSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
  IPTR result = FALSE;

  ENTER();

  if(DoSuperMethodA(cl, obj, (APTR)msg))
  {
    struct listData *data = INST_DATA(cl, obj);

    if(isFlagClear(data->flags, FLG_ListSetup))
    {
      /* After thinking about that hard, I decided to use the lamp in >=8 color screen */
      if(data->olamp != NULL && GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH) > 3)
        data->lamp = (APTR)DoSuperMethod(cl, obj, MUIM_List_CreateImage, (IPTR)data->olamp, 0);

      SET_FLAG(data->flags, FLG_ListSetup);

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

/***********************************************************************/

static IPTR mListCleanup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
  IPTR result;
  struct listData *data = INST_DATA(cl, obj);

  ENTER();

  if(isFlagSet(data->flags, FLG_ListSetup))
  {
    if(data->lamp != NULL)
    {
      DoSuperMethod(cl, obj, MUIM_List_DeleteImage, (IPTR)data->lamp);
      data->lamp = NULL;
    }

    CLEAR_FLAG(data->flags, FLG_ListSetup);
  }

  result = DoSuperMethodA(cl, obj, (APTR)msg);

  RETURN(result);
  return(result);
}

/***********************************************************************/
/*
** Import format
*/

static IPTR mListImport(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    ULONG id;

    if(MUIMasterBase->lib_Version < 20)
        return 0;

    if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
    {
        struct listIO *io;

        if((io = (struct listIO *)DoMethod(msg->dataspace,MUIM_Dataspace_Find, id)) != NULL)
        {
            struct listData *data = INST_DATA(cl,obj);

            strlcpy(data->format, io->format, sizeof(data->format));
            set(obj,MUIA_List_Format,data->format);
        }
    }

    return 0;
}

/***********************************************************************/
/*
** Export format
*/

static IPTR mListExport(UNUSED struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    ULONG id;

    if(MUIMasterBase->lib_Version < 20)
        return 0;

    if((id = (muiNotifyData(obj)->mnd_ObjectID)) != 0)
    {
        struct listIO io;
        STRPTR        f;

        if((f = (STRPTR)xget(obj, MUIA_List_Format)) != NULL)
        {
          io.len = strlen(f)+1;
          strlcpy(io.format, f, sizeof(io.format));

          DoMethod(msg->dataspace,MUIM_Dataspace_Add,(IPTR)&io,sizeof(ULONG)+io.len,id);
        }
    }

    return 0;
}

/**************************************************************************/
/*
** Check if format changed
*/

static IPTR mListCheckSave(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct listData *data = INST_DATA(cl,obj);
  STRPTR f;

  if((f = (STRPTR)xget(obj, MUIA_List_Format)) != NULL)
    return (IPTR)strcmp(f,(STRPTR)&data->format);
  else
    return 0;
}

/**************************************************************************/

SDISPATCHER(listDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                  return mListNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:              return mListDispose(cl,obj,(APTR)msg);

        case MUIM_Setup:              return mListSetup(cl,obj,(APTR)msg);
        case MUIM_Cleanup:            return mListCleanup(cl,obj,(APTR)msg);
        case MUIM_Import:             return mListImport(cl,obj,(APTR)msg);
        case MUIM_Export:             return mListExport(cl,obj,(APTR)msg);

        case MUIM_App_CheckSave:      return mListCheckSave(cl,obj,(APTR)msg);

        default:                      return DoSuperMethodA(cl,obj,msg);
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
    Object         *appList;
    Object         *add;
    Object         *edit;
    Object         *clone;
    Object         *delete;
    Object         *disable;
    Object         *up;
    Object         *down;

    ULONG          nameOfs;
    ULONG          pathOfs;
    ULONG          nodeSize;
    struct IClass  *editClass;
    ULONG          editAttr;
    ULONG          listAttr;

    STRPTR         newNodeName;
};

/**************************************************************************/

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object        *appl, *addb, *editb, *cloneb, *deleteb, *disableb, *upb, *downb;
    struct IClass *editWinClass;
    STRPTR         nodeName;
    CONST_STRPTR helpNode;
    ULONG         nameOfs, pathOfs, nodeSize, editWinAttr, listAttr, help, id;

    /* What we are  */
    switch (GetTagData(MUIA_AppList_Type,0,msg->ops_AttrList))
    {
        case MUIV_AppList_Type_Browser:
            nameOfs            = sizeof(struct MinNode) + sizeof(ULONG);
            pathOfs            = sizeof(struct MinNode) + sizeof(ULONG) + NAME_LEN;
            nodeSize           = sizeof(struct URL_BrowserNode);
            editWinClass       = g_browserEditWinClass->mcc_Class;
            editWinAttr        = MUIA_BrowserEditWin_Browser;
            listAttr           = MUIA_BrowserEditWin_ListObj;
            nodeName           = getString(MSG_Browser_NewBrowser);
            helpNode           = "BROWSER";
            help               = MSG_Browser_List_Help;
            id                 = MAKE_ID('B','L','S','T');
            break;

        case MUIV_AppList_Type_Mailer:
            nameOfs            = sizeof(struct MinNode) + sizeof(ULONG);
            pathOfs            = sizeof(struct MinNode) + sizeof(ULONG) + NAME_LEN;
            nodeSize           = sizeof(struct URL_MailerNode);
            editWinClass       = g_mailerEditWinClass->mcc_Class;
            editWinAttr        = MUIA_MailerEditWin_Mailer;
            listAttr           = MUIA_MailerEditWin_ListObj;
            nodeName           = getString(MSG_Mailer_NewMailer);
            helpNode           = "MAILERS";
            help               = MSG_Mailer_List_Help;
            id                 = MAKE_ID('M','L','S','T');
            break;

        case MUIV_AppList_Type_FTP:
            nameOfs            = sizeof(struct MinNode) + sizeof(ULONG);
            pathOfs            = sizeof(struct MinNode) + sizeof(ULONG) + NAME_LEN;
            nodeSize           = sizeof(struct URL_FTPNode);
            editWinClass       = g_FTPEditWinClass->mcc_Class;
            editWinAttr        = MUIA_FTPEditWin_FTP;
            listAttr           = MUIA_FTPEditWin_ListObj;
            nodeName           = getString(MSG_FTP_NewFTP);
            helpNode           = "FTPS";
            help               = MSG_FTP_List_Help;
            id                 = MAKE_ID('F','L','S','T');
            break;

        default:
            return 0;
    }

    if((obj = (Object *)DoSuperNew(cl,obj,
                MUIA_HelpNode,           helpNode,
                MUIA_ShortHelp,          getString(help),
                MUIA_Group_Horiz,        TRUE,
                MUIA_Group_HorizSpacing, 2,

                Child, ListviewObject,
                    MUIA_CycleChain,               TRUE,
                    MUIA_Listview_DefClickColumn,  1,
                    MUIA_Listview_DragType,        MUIV_Listview_DragType_Immediate,
                    MUIA_Listview_List, appl = listObject,
                        MUIA_ObjectID,               id,
                        MUIA_AppList_NodeNameOffset, nameOfs,
                        MUIA_AppList_NodePathOffset, pathOfs,
                        MUIA_AppList_NodeSize,       nodeSize,
                    End,
                End,

                Child, VGroup,
                    //VirtualFrame,
                    //MUIA_Background, MUII_GroupBack,
                    MUIA_Group_VertSpacing, 2,
                    MUIA_Weight,     		0,

                    Child, addb    = obutton(MSG_AppList_Add,MSG_AppList_Add_Help),
                    Child, editb   = obutton(MSG_AppList_Edit,MSG_AppList_Edit_Help),
                    Child, cloneb  = obutton(MSG_AppList_Clone,MSG_AppList_Clone_Help),
                    Child, deleteb = obutton(MSG_AppList_Delete,MSG_AppList_Delete_Help),
                    Child, HGroup,
                        MUIA_Group_HorizSpacing, 1,
                        Child, upb      = oibutton(IBT_Up,MSG_AppList_MoveUp_Help),
                        Child, downb    = oibutton(IBT_Down,MSG_AppList_MoveDown_Help),
                        Child, disableb = otbutton(MSG_AppList_Disable,MSG_AppList_Disable_Help),
                    End,
                    Child, VSpace(0),
                End,

            TAG_MORE, msg->ops_AttrList)) != NULL)
    {
        struct data *data = INST_DATA(cl,obj);

        /* init instance data */
        data->appList  = appl;
        data->add      = addb;
        data->edit     = editb;
        data->clone    = cloneb;
        data->delete   = deleteb;
        data->disable  = disableb;
        data->up       = upb;
        data->down     = downb;

        data->nameOfs  = nameOfs;
        data->pathOfs  = pathOfs;
        data->nodeSize = nodeSize;

        data->editClass   = editWinClass;
        data->editAttr    = editWinAttr;
        data->listAttr    = listAttr;
        data->newNodeName = nodeName;

        /* listview */
        DoMethod(appl,MUIM_Notify,MUIA_List_Active,MUIV_EveryTime,(IPTR)obj,1,MUIM_AppList_ActiveChanged);
        DoMethod(appl,MUIM_Notify,MUIA_Listview_DoubleClick,MUIV_EveryTime,(IPTR)obj,2,MUIM_AppList_Edit,TRUE);

        /* buttons */
        DoMethod(obj,MUIM_MultiSet,MUIA_Disabled,TRUE,
            (IPTR)editb,
            (IPTR)cloneb,
            (IPTR)deleteb,
            (IPTR)disableb,
            (IPTR)upb,
            (IPTR)downb,
            NULL);

        /* list buttons */
        DoMethod(addb,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_AppList_Add);
        DoMethod(editb,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_AppList_Edit,FALSE);
        DoMethod(cloneb,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_AppList_Clone);
        DoMethod(deleteb,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,1,MUIM_AppList_Delete);
        DoMethod(disableb,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,(IPTR)obj,2,MUIM_AppList_Disable,MUIV_TriggerValue);
        DoMethod(upb,MUIM_Notify,MUIA_Timer,MUIV_EveryTime,(IPTR)obj,2,MUIM_AppList_Move,TRUE);
        DoMethod(downb,MUIM_Notify,MUIA_Timer,MUIV_EveryTime,(IPTR)obj,2,MUIM_AppList_Move,FALSE);
    }

    return (IPTR)obj;
}

/**************************************************************************/
/*
** I hate this: it will be removed asap!
*/

static IPTR mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct data *data = INST_DATA(cl,obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_AppList_ListObj: *msg->opg_Storage = (IPTR)data->appList; return TRUE;
        default: return DoSuperMethodA(cl,obj,(Msg)msg);
    }
}

/**************************************************************************/

static IPTR mAdd(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data     *data = INST_DATA(cl,obj);
    struct URL_Node *node;

    if (!(node = AllocPooled(g_pool,data->nodeSize))) return FALSE;

    memset(node,0,data->nodeSize);
    strcpy((STRPTR)node+data->nameOfs,data->newNodeName);

    node->Flags = UNF_NEW|UNF_NTALLOC;

    DoMethod(data->appList,MUIM_List_InsertSingle,(IPTR)node,MUIV_List_Insert_Bottom);

    set(data->appList,MUIA_List_Active,xget(data->appList,MUIA_List_InsertPosition));

    DoMethod(obj,MUIM_AppList_Edit,FALSE);

    return TRUE;
}

/**************************************************************************/

static IPTR mEdit(struct IClass *cl, Object *obj, struct MUIP_AppList_Edit *msg)
{
    struct data     *data = INST_DATA(cl,obj);
    struct URL_Node *node;

    DoMethod(data->appList,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,(IPTR)&node);
    if (node)
    {
        if (msg->check && (xget(data->appList,MUIA_Listview_ClickColumn)==0))
        {
            set(data->appList,MUIA_Listview_ClickColumn,1);

            if(isFlagSet(node->Flags, UNF_DISABLED))
              CLEAR_FLAG(node->Flags, UNF_DISABLED);
            else
              SET_FLAG(node->Flags, UNF_DISABLED);

            DoMethod(data->appList, MUIM_List_Redraw, xget(data->appList, MUIA_List_Active));
            set(data->disable, MUIA_Selected, isFlagSet(node->Flags, UNF_DISABLED));
        }
        else
            DoMethod(_app(obj),MUIM_App_OpenWin,(IPTR)data->editClass,data->editAttr,(IPTR)node,data->listAttr,(IPTR)data->appList,TAG_END);
    }

    return TRUE;
}

/**************************************************************************/

static IPTR mClone(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    ULONG success = FALSE;
    struct data     *data = INST_DATA(cl,obj);
    struct URL_Node *node;
    ULONG           active;

    active = xget(data->appList, MUIA_List_Active);
    DoMethod(data->appList, MUIM_List_GetEntry, active, (IPTR)&node);
    if(node != NULL)
    {
        struct URL_Node *new;

        if((new = AllocPooled(g_pool, data->nodeSize)) != NULL)
        {
            CopyMem(node, new, data->nodeSize);
            new->Flags = UNF_NEW|UNF_NTALLOC;

            DoMethod(data->appList, MUIM_List_InsertSingle, (IPTR)new, MUIV_List_Insert_Bottom);
            set(data->appList, MUIA_List_Active, MUIV_List_Active_Bottom);

            DoMethod(obj, MUIM_AppList_Edit, FALSE);

            success = TRUE;
        }
    }

    return success;
}

/**************************************************************************/

static IPTR mDelete(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    UBYTE       *node;
    ULONG       active;

    active = xget(data->appList,MUIA_List_Active);
    DoMethod(data->appList,MUIM_List_GetEntry,active,(IPTR)&node);
    if (node)
    {
        DoMethod(_app(obj),MUIM_App_CloseWin,data->editAttr,(IPTR)node);
        DoMethod(data->appList,MUIM_List_Remove,active);
    }

    return TRUE;
}

/**************************************************************************/

static IPTR mActiveChanged(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    LONG    a;

    a = (LONG)xget(data->appList,MUIA_List_Active);
    if (a>=0)
    {
        struct URL_Node *node;
        LONG       n;

        DoMethod(obj,MUIM_MultiSet,MUIA_Disabled,FALSE,
            (IPTR)data->edit,
            (IPTR)data->clone,
            (IPTR)data->delete,
            (IPTR)data->disable,
            NULL);

        DoMethod(data->appList,MUIM_List_GetEntry,a,(IPTR)&node);
        set(data->disable, MUIA_Selected, isFlagSet(node->Flags, UNF_DISABLED));

        if (a==0) SetAttrs(data->up,MUIA_Selected,FALSE,MUIA_Disabled,TRUE,TAG_DONE);
        else set(data->up,MUIA_Disabled,FALSE);

        n = xget(data->appList,MUIA_List_Entries);
        if (n-1<=a) SetAttrs(data->down,MUIA_Selected,FALSE,MUIA_Disabled,TRUE,TAG_DONE);
        else set(data->down,MUIA_Disabled,FALSE);
    }
    else
    {
        set(data->disable,MUIA_Selected,FALSE);

        DoMethod(obj,MUIM_MultiSet,MUIA_Disabled,TRUE,
            (IPTR)data->edit,
            (IPTR)data->clone,
            (IPTR)data->delete,
            (IPTR)data->disable,
            (IPTR)data->up,
            (IPTR)data->down,
            NULL);
    }

    return TRUE;
}

/**************************************************************************/

static IPTR mDisable(struct IClass *cl, Object *obj, struct MUIP_AppList_Disable *msg)
{
    struct data     *data = INST_DATA(cl,obj);
    struct URL_Node *node;

    DoMethod(data->appList,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,(IPTR)&node);
    if (node)
    {
        if (!BOOLSAME(msg->disable, isFlagSet(node->Flags, UNF_DISABLED)))
        {
            if(msg->disable)
                SET_FLAG(node->Flags, UNF_DISABLED);
            else
                CLEAR_FLAG(node->Flags, UNF_DISABLED);

            DoMethod(data->appList,MUIM_List_Redraw,xget(data->appList,MUIA_List_Active));
        }
    }

    return TRUE;
}

/**************************************************************************/

static IPTR mMove(struct IClass *cl, Object *obj, struct MUIP_AppList_Move *msg)
{
    struct data *data = INST_DATA(cl,obj);

    DoMethod(data->appList,MUIM_List_Exchange,MUIV_List_Exchange_Active,msg->up ? MUIV_List_Exchange_Previous : MUIV_List_Exchange_Next);
    set(data->appList,MUIA_List_Active,msg->up ? MUIV_List_Active_Up : MUIV_List_Active_Down);

    return 0;
}

/**************************************************************************/
/*
** Forward to the list
*/

static IPTR mCheckSave(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    return DoMethod(data->appList,MUIM_App_CheckSave);
}

/**************************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                     return mNew(cl,obj,(APTR)msg);
        case OM_GET:                     return mGet(cl,obj,(APTR)msg);

        case MUIM_AppList_Add:           return mAdd(cl,obj,(APTR)msg);
        case MUIM_AppList_Edit:          return mEdit(cl,obj,(APTR)msg);
        case MUIM_AppList_Clone:         return mClone(cl,obj,(APTR)msg);
        case MUIM_AppList_Delete:        return mDelete(cl,obj,(APTR)msg);
        case MUIM_AppList_ActiveChanged: return mActiveChanged(cl,obj,(APTR)msg);
        case MUIM_AppList_Disable:       return mDisable(cl,obj,(APTR)msg);
        case MUIM_AppList_Move:          return mMove(cl,obj,(APTR)msg);
        case MUIM_App_CheckSave:         return mCheckSave(cl,obj,(APTR)msg);

        default:                         return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

BOOL initAppListClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if(initListClass() == TRUE)
    {
        if((g_appListClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
        {
            initLampClass();
            success = TRUE;
        }
        else
            disposeListClass();
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposeAppListClass(void)
{
    ENTER();

    disposeLampClass();
    disposeListClass();
    if(g_appListClass != NULL)
        MUI_DeleteCustomClass(g_appListClass);

    LEAVE();
}

/**************************************************************************/

