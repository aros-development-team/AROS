
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include <stdio.h>
#include <stdlib.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"
#include "muimiamipanel_message.h"

/***********************************************************************/

struct MiamiPanelIfGroupClass_DATA
{
    Object                      *app;

    Object                      *space;
    APTR                        cmenu;

    int                         children;
    int                         refresh;

    ULONG                       show;

    struct MUI_InputHandlerNode ih;

    ULONG                       flags;
};

enum
{
    FLG_Setup   = 1<<0,
    FLG_Handler = 1<<1,
    FLG_BWin    = 1<<2,
    FLG_Bar     = 1<<3,
    FLG_SkipBar = 1<<4,
};

/***********************************************************************/

#define NMX(x) ((1<<8)-1-(1<<x))

static ULONG cMenuIDs[] =
{
    MSG_Menu_Project,
        MSG_IFGroup_CItem_Online,
        MSG_IFGroup_CItem_Offline,
        MSG_IFGroup_CItem_Scale
};

static struct NewMenu cMenu[] =
{
    MTITLE(MSG_Menu_Project),
        MITEM(MSG_IFGroup_CItem_Online),
        MITEM(MSG_IFGroup_CItem_Offline),
        MITEM(MSG_IFGroup_CItem_Scale),
            MXSUB(TAG_SCALE_1,NMX(0)),
            MXSUB(TAG_SCALE_2,NMX(1)),
            MXSUB(TAG_SCALE_3,NMX(2)),
            MXSUB(TAG_SCALE_4,NMX(3)),
            MXSUB(TAG_SCALE_5,NMX(4)),
            MXSUB(TAG_SCALE_6,NMX(5)),
            MXSUB(TAG_SCALE_7,NMX(6)),
            MXSUB(TAG_SCALE_8,NMX(7)),
    MEND
};

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_IfGroup__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct TagItem   *attrs = message->ops_AttrList;
    Object           *space;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Frame,      MUIV_Frame_Virtual,
            //MUIA_Background, MUII_GroupBack,
            MUIA_HelpNode,   "IFGroup",
            Child, space = RectangleObject, /*MUIA_Background, MUII_GroupBack, */End,
            TAG_MORE,attrs))
    {
        struct MiamiPanelIfGroupClass_DATA *data = INST_DATA(CLASS,self);

        data->space = space;
        data->show  = GetTagData(MPA_Show,0,attrs);

        if (data->cmenu = MUI_MakeObject(MUIO_MenustripNM,(ULONG)cMenu,0))
        {
            if (!(data->show & MIAMIPANELV_Init_Flags_ShowDataTransferRate))
            {
                Object *cmscale;

                DoMethod(data->cmenu,MUIM_Family_Remove,(ULONG)(cmscale = (Object *)DoMethod(data->cmenu,MUIM_FindUData,MSG_IFGroup_CItem_Scale)));
                MUI_DisposeObject(cmscale);
            }
        }

        data->flags |= FLG_Bar;
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelIfGroupClass_DATA    *data = INST_DATA(CLASS,self);
    struct TagItem *tag;
    struct TagItem *tstate;

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Prefs:
                if (PREFS(tidata)->flags & MPV_Flags_BWin)
                    data->flags |= FLG_BWin;
                else data->flags &= ~FLG_BWin;
                break;

            case MPA_SkipBar:
                if (tidata) data->flags |= FLG_SkipBar;
                else
                {
                    ULONG vh;

                    data->flags &= ~FLG_SkipBar;

                    DoSuperMethod(CLASS,self,OM_GET,MUIA_Virtgroup_Height,(ULONG)&vh);

                    if (vh>=_height(self))
                    {
                        data->flags |= FLG_Bar;
                        set(_app(self),MPA_Bar,TRUE);
                    }
                    else
                    {
                        data->flags &= ~FLG_Bar;
                        set(_app(self),MPA_Bar,FALSE);
                    }
                }
                break;
        }
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__OM_DISPOSE(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (data->cmenu) MUI_DisposeObject(data->cmenu);

    if (data->flags & FLG_Handler)
        DoMethod(data->app,MUIM_Application_RemInputHandler,(ULONG)&data->ih);

    return DoSuperMethodA(CLASS,self,message);
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__MUIM_Setup(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (!DoSuperMethodA(CLASS,self,message)) return FALSE;

    if (!(data->flags & FLG_Handler))
    {
        data->app = _app(self);
        memset(&data->ih,0,sizeof(data->ih));
        data->ih.ihn_Object  = self;
        data->ih.ihn_Method  = MPM_IfGroup_HandleEvent;
        data->ih.ihn_Signals = 1<<MiamiPanelBaseIntern->mpb_port->mp_SigBit;
        DoMethod(data->app,MUIM_Application_AddInputHandler,(ULONG)&data->ih);

        data->flags |= FLG_Handler;
    }

    data->flags |= FLG_Setup;

    return TRUE;
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__MUIM_Cleanup(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfGroupClass_DATA *data = INST_DATA(CLASS,self);

    data->flags &= ~FLG_Setup;

    return DoSuperMethodA(CLASS,self,message);
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__MUIM_Show(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (!DoSuperMethodA(CLASS,self,message)) return FALSE;

    if (data->flags & FLG_BWin)
    {
        ULONG bar;
        ULONG vh;

        DoSuperMethod(CLASS,self,OM_GET,MUIA_Virtgroup_Height,(ULONG)&vh);

        if (!BOOLSAME(data->flags & FLG_Bar,bar = vh>=_height(self)))
        {
            if (bar) data->flags |= FLG_Bar;
            else data->flags &= ~FLG_Bar;

            if (!(data->flags & FLG_SkipBar))
                DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_app(self),3,MUIM_Set,MPA_Bar,(ULONG)bar);
        }
    }

    return TRUE;
}

/***********************************************************************/

static Object *
findInterface(Object *self,long unit)
{
    struct List *l;
    Object      *cstate;
    Object      *child;

    get(self,MUIA_Group_ChildList,&l);
    cstate = (Object *)l->lh_Head;

    while (child = NextObject(&cstate))
    {
        long u;

        if (get(child,MPA_If_Unit,&u) && (u==unit))
            return child;
    }

    return NULL;
}

/***********************************************************************/

static Object *
findNextChild(Object *self,Object *from,struct MiamiPanelIfGroupClass_DATA *data)
{
    struct List *l;
    Object      *cstate;
    Object      *child, *space;
    ULONG       next;

    get(self,MUIA_Group_ChildList,&l);
    cstate = (Object *)l->lh_Head;
    space = data->space;
    next = 0;

    while (child = NextObject(&cstate))
    {
        if (child==from) next = 1;
        else if (next && child!=space) return child;
    }

    return NULL;
}

/***********************************************************************/

static void
addInterface(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_AddInterface *message)
{
    Object           *child, *bar;
    struct MPS_Prefs *prefs;

    if (data->children)
        bar = RectangleObject,
            MUIA_Weight,         0,
            MUIA_Rectangle_HBar, TRUE,
            MUIA_FixHeightTxt,   "X",
        End;
    else bar = NULL;

    get(_app(self),MPA_Prefs,&prefs);

    if (child = NewObject(MiamiPanelBaseIntern->mpb_ifGroupClass->mcc_Class,
        	NULL,
            MUIA_ObjectID,    MAKE_ID('i','f',(message->unit & 0xF0),(message->unit & 0xF)),
            MUIA_ContextMenu, data->cmenu,
            MPA_Show,         data->show,
            MPA_If_Unit,      message->unit,
            MPA_If_Name,      message->name,
            MPA_If_State,     message->state,
            MPA_If_Ontime,    message->ontime,
            MPA_If_Speed,     message->speed,
            MPA_Prefs,        prefs,
			TAG_DONE))
    {
        DoSuperMethod(CLASS,self,MUIM_Group_InitChange);

        DoSuperMethod(CLASS,self,OM_REMMEMBER,(ULONG)data->space);

        if (bar) DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)bar);
        DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)child);

        DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)data->space);

        DoSuperMethod(CLASS,self,MUIM_Group_ExitChange);
        data->children++;
    }
    else if (bar) MUI_DisposeObject(bar);
}

/***********************************************************************/

static void
delInterface(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_DelInterface *message)
{
    Object *child;

    if (child = findInterface(self,message->unit))
    {
        register Object *next;

        DoSuperMethod(CLASS,self,MUIM_Group_InitChange);

        if (next = findNextChild(self, child, data))
        {
            DoSuperMethod(CLASS,self,OM_REMMEMBER,(ULONG)next);
            MUI_DisposeObject(next);
        }

        DoSuperMethod(CLASS,self,OM_REMMEMBER,(ULONG)child);
        MUI_DisposeObject(child);

        DoSuperMethod(CLASS,self,MUIM_Group_ExitChange);

        data->children--;
    }
}

/***********************************************************************/

static void
setInterfaceState(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_SetInterfaceState *message)
{
    Object *child;

    if (child = findInterface(self,message->unit))
        SetAttrs(child,MPA_If_State,  message->state,
                       MPA_If_Ontime, message->ontime,
                       TAG_DONE);
}

/***********************************************************************/

static void
setInterfaceSpeed(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_SetInterfaceSpeed *message)
{
    Object *child;

    if (child = findInterface(self,message->unit))
        set(child,MPA_If_Speed,message->speed);
}

/***********************************************************************/

static void
interfaceReport(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_InterfaceReport *message)
{
    Object *child;

    if (child = findInterface(self,message->unit))
        SetAttrs(child,MPA_If_Rate,    message->rate,
                       MPA_If_Now,     message->now,
                       MPA_If_Traffic, (ULONG)&message->total,
                       TAG_DONE);
}

/***********************************************************************/

static void
refreshName(struct IClass *CLASS,Object *self,struct MiamiPanelIfGroupClass_DATA *data,struct MPS_Msg_RefreshName *message)
{
    Object *child;

    if (child = findInterface(self,message->unit))
        set(child,MPA_If_Name,message->name);
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__MPM_IfGroup_HandleEvent(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfGroupClass_DATA    *data = INST_DATA(CLASS,self);
    struct MPS_Msg *mpsg;
    ULONG          res;

    res = 0;

    while (mpsg = (struct MPS_Msg *)GetMsg(MiamiPanelBaseIntern->mpb_port))
    {
        res++;

        switch (mpsg->type)
        {
            case MPV_Msg_Type_Cleanup:
                DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_app(self),2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
                break;

            case MPV_Msg_Type_AddInterface:
                addInterface(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_DelInterface:
                delInterface(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_SetInterfaceState:
                setInterfaceState(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_SetInterfaceSpeed:
                setInterfaceSpeed(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_InterfaceReport:
                interfaceReport(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_RefreshName:
                refreshName(CLASS,self,data,(APTR)mpsg);
                break;

            case MPV_Msg_Type_ToFront:
            {
                ULONG iconified;

                get(_app(self),MUIA_Application_Iconified,&iconified);
                if (iconified) set(_app(self),MUIA_Application_Iconified,FALSE);
                else
                    if (data->flags & FLG_Setup) set(_win(self),MUIA_Window_Open,TRUE);
                    else DisplayBeep(0);
                break;
            }

            case MPV_Msg_Type_InhibitRefresh:
            {
                struct MPS_Msg_InhibitRefresh *m = (struct MPS_Msg_InhibitRefresh *)mpsg;

                if (m->val)
                {
                    if (!data->refresh++)
                        DoMethod(self,MUIM_Group_InitChange);
                }
                else
                    if (data->refresh && !--data->refresh)
                        DoMethod(self,MUIM_Group_ExitChange);
                break;
            }

            default:
                break;
        }

        if (mpsg->flags & MPV_Msg_Flags_Reply) ReplyMsg((struct Message *)mpsg);
        else freeMsg(mpsg, MiamiPanelBaseIntern);
    }

    return res;
}

/***********************************************************************/

static ULONG
MUIPC_IfGroup__MPM_IfGroup_GrabIFList(struct IClass *CLASS,Object *self,Msg message)
{
    struct List      *l;
    Object           *cstate;
    Object           *child;
    struct MPS_Prefs *prefs;

    get(_app(self),MPA_Prefs,&prefs);
    freeIFList(prefs, MiamiPanelBaseIntern);

    get(self,MUIA_Group_ChildList,&l);
    cstate = (Object *)l->lh_Head;

    while (child = NextObject(&cstate))
    {
        UBYTE  *name;
        ULONG  scale;

        if (get(child,MPA_If_Name,&name))
        {
            get(child,MPA_If_Scale,&scale);
            createIFNode(prefs,name,scale, MiamiPanelBaseIntern);
        }
    }

    return NULL;
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_IfGroup_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                  return MUIPC_IfGroup__OM_NEW(CLASS,self,(APTR)message);
        case OM_SET:                  return MUIPC_IfGroup__OM_SET(CLASS,self,(APTR)message);
        case OM_DISPOSE:              return MUIPC_IfGroup__OM_DISPOSE(CLASS,self,(APTR)message);
        case MUIM_Setup:              return MUIPC_IfGroup__MUIM_Setup(CLASS,self,(APTR)message);
        case MUIM_Cleanup:            return MUIPC_IfGroup__MUIM_Cleanup(CLASS,self,(APTR)message);
        case MUIM_Show:               return MUIPC_IfGroup__MUIM_Show(CLASS,self,(APTR)message);
        case MPM_IfGroup_HandleEvent: return MUIPC_IfGroup__MPM_IfGroup_HandleEvent(CLASS,self,(APTR)message);
        case MPM_IfGroup_GrabIFList:  return MUIPC_IfGroup__MPM_IfGroup_GrabIFList(CLASS,self,(APTR)message);
        default:                      return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

static ULONG defScales[] =
{
    6000,
    12000,
    32000,
    64000,
    128000,
    256000,
    512000,
    1024000,
    0
};

struct TagItem scales[] =
{
    TAG_SCALE_1,     6000,
    TAG_SCALE_2,    12000,
    TAG_SCALE_3,    32000,
    TAG_SCALE_4,    64000,
    TAG_SCALE_5,   128000,
    TAG_SCALE_6,   256000,
    TAG_SCALE_7,   512000,
    TAG_SCALE_8,  1024000,
    TAG_DONE
};

static UBYTE scaleStrings[TAG_SCALE_LAST-TAG_SCALE-1][64];

ULONG
MUIPC_IfGroup_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_ifGroupClass = MUI_CreateCustomClass(NULL, MUIC_Virtgroup,NULL, sizeof(struct MiamiPanelIfGroupClass_DATA), MUIPC_IfGroup_Dispatcher))
    {
        struct NewMenu *nm;
        struct TagItem *t;
        UBYTE          *scaleFmt;
        BPTR           file;
        ULONG          *ids;
        int            i;

    for (i = 0, ids = defScales; *ids; ids++, i++) scales[i].ti_Data = *ids;

        if (file = Open("MIAMI:Libs/MUI.MiamiPanel.scales",MODE_OLDFILE))
        {
            UBYTE buf[32];

            i = 0;

            while (FGets(file,buf,sizeof(buf)))
            {
                UBYTE *s;

                for (s = buf; *s && *s!='\n' && *s!='\r'; s++);
                *s = 0;

                if (!*buf || *buf==';') continue;

                scales[i++].ti_Data = atoi(buf);
                if (i==TAG_SCALE_LAST-TAG_SCALE-1) break;
            }

            Close(file);
        }

        scaleFmt = __(MSG_IFGroup_CItem_ScaleFmt);

        for (i = 0, t = scales; t->ti_Tag!=TAG_DONE; i++, t++)
            snprintf(scaleStrings[i],sizeof(scaleStrings[i]),scaleFmt,t->ti_Data/1000);

        for (ids = cMenuIDs, nm = cMenu ; nm->nm_Type!=NM_END; nm++)
            if (nm->nm_Label!=NM_BARLABEL)
                if (((ULONG)nm->nm_UserData>TAG_SCALE) && ((ULONG)nm->nm_UserData<TAG_SCALE_LAST))
                    nm->nm_Label = scaleStrings[(ULONG)nm->nm_UserData-TAG_SCALE-1];
                else nm->nm_Label = __(*ids++);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
