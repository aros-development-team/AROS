

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"
#include "muimiamipanel_commands.h"

#include <mui/Busy_mcc.h>

/***********************************************************************/

struct MiamiPanelIfClass_DATA
{
    Object          *this;

    Object          *g;
    Object          *name;
    Object          *slabel;
    Object          *state;
    Object          *busy;
    Object          *lbutton;

    Object          *ontime;
    Object          *traffic;
    Object          *rate;
    Object          *speed;

    APTR            cmenu;
    APTR            cmtitle;
    APTR            cmonline;
    APTR            cmoffline;

    UBYTE           namev[16];
    long            unitv;
    ULONG           ontimev;
    ULONG           statev;

    ULONG           scale;

    ULONG           flags;
};

enum
{
    FLG_UseBusyBar   = 1<<0,
    FLG_BusyBarInUse = 1<<1,
};

/***********************************************************************/

#define SHOW_ALL (MIAMIPANELV_Init_Flags_ShowStatusButton|MIAMIPANELV_Init_Flags_ShowSpeed|MIAMIPANELV_Init_Flags_ShowDataTransferRate|MIAMIPANELV_Init_Flags_ShowUpTime|MIAMIPANELV_Init_Flags_ShowTotal)

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_If__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelIfClass_DATA      temp;
    struct TagItem   *attrs = message->ops_AttrList, *tag;
    struct MPS_Prefs *prefs = (struct MPS_Prefs *)GetTagData(MPA_Prefs,NULL,attrs);
    ULONG            show, lbutton, ontime, traffic, rate, speed, some;
    UBYTE            *name;

    memset(&temp,0,sizeof(temp));

    name = (UBYTE *)GetTagData(MPA_If_Name,NULL,attrs);
    temp.unitv = GetTagData(MPA_If_Unit,-1,attrs);

    show    = (tag = FindTagItem(MPA_Show,attrs)) ? tag->ti_Data : SHOW_ALL;
    lbutton = show & MIAMIPANELV_Init_Flags_ShowStatusButton;
    ontime  = show & MIAMIPANELV_Init_Flags_ShowUpTime;
    traffic = show & MIAMIPANELV_Init_Flags_ShowTotal;
    rate    = show & MIAMIPANELV_Init_Flags_ShowDataTransferRate;
    speed   = show & MIAMIPANELV_Init_Flags_ShowSpeed;
    some    = ontime || traffic || rate || speed;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            Child, temp.g = HGroup,

                Child, temp.name = TextObject,
                    MUIA_Text_PreParse, MUIX_B MUIX_U MUIX_C,
                    MUIA_Font,          MUIV_Font_Big,
                    MUIA_FixWidthTxt,   "XXXXXXXX",
                End,

                //Child, temp.slabel = olabel1(MSG_IF_Title_State, MiamiPanelBaseIntern),

                Child, temp.state = TextObject,
                    //MUIA_Frame,      MUIV_Frame_Text,
                    //MUIA_Background, MUII_TextBack,
                End,

                lbutton ? Child : TAG_IGNORE, lbutton ? (temp.lbutton = NewObject(MiamiPanelBaseIntern->mpb_lbuttonClass->mcc_Class, NULL, TAG_DONE)) : NULL,
            End,

            /*some ? Child : TAG_IGNORE, some ? HGroup,
                MUIA_Weight, 0,
                Child, RectangleObject, MUIA_Weight, 10, End,
                Child, RectangleObject, MUIA_Rectangle_HBar, TRUE, End,
                Child, RectangleObject, MUIA_Weight, 10, End,
            End : 0,*/

            some ? Child : TAG_IGNORE, some ? VSpace(4) : 0,

            some ? Child : TAG_IGNORE, some ? ColGroup(2),
                ontime  ? Child : TAG_IGNORE, ontime  ? olabel(MSG_IF_Title_OnTime, MiamiPanelBaseIntern)  : NULL,
                ontime  ? Child : TAG_IGNORE, ontime  ? (temp.ontime  = NewObject(MiamiPanelBaseIntern->mpb_timeTextClass->mcc_Class, NULL, TAG_DONE))                  : NULL,

                traffic ? Child : TAG_IGNORE, traffic ? olabel(MSG_IF_Title_Traffic, MiamiPanelBaseIntern) : NULL,
                traffic ? Child : TAG_IGNORE, traffic ? (temp.traffic = NewObject(MiamiPanelBaseIntern->mpb_trafficClass->mcc_Class, NULL, MPA_Prefs, prefs, TAG_DONE)) : NULL,

                rate    ? Child : TAG_IGNORE, rate    ? olabel(MSG_IF_Title_Rate, MiamiPanelBaseIntern)    : NULL,
                rate    ? Child : TAG_IGNORE, rate    ? (temp.rate = NewObject(MiamiPanelBaseIntern->mpb_rateClass->mcc_Class, NULL, MPA_Prefs, prefs, TAG_DONE))       : NULL,

                speed   ? Child : TAG_IGNORE, speed   ? olabel(MSG_IF_Title_Speed, MiamiPanelBaseIntern)   : NULL,
                speed   ? Child : TAG_IGNORE, speed   ? (temp.speed = TextObject, End)                        : NULL,
            End : 0,

            TAG_MORE,attrs))
    {
        struct MiamiPanelIfClass_DATA *data = INST_DATA(CLASS,self);

        CopyMem(&temp,data,sizeof(struct MiamiPanelIfClass_DATA));

        data->busy = BusyObject,
            MUIA_Weight,          50,
            MUIA_Busy_ShowHideIH, TRUE,
            MUIA_Busy_Speed,      MUIV_Busy_Speed_User,
        End;

        if (data->cmenu = (APTR)GetTagData(MUIA_ContextMenu,NULL,attrs))
        {
            data->cmtitle   = (APTR)DoMethod(data->cmenu,MUIM_FindUData,MSG_Menu_Project);
            data->cmonline  = (APTR)DoMethod(data->cmenu,MUIM_FindUData,MSG_IFGroup_CItem_Online);
            data->cmoffline = (APTR)DoMethod(data->cmenu,MUIM_FindUData,MSG_IFGroup_CItem_Offline);
        }

        if (data->lbutton)
            DoMethod(data->lbutton,MUIM_Notify,MUIA_Pressed,0,(ULONG)self,1,MPM_If_Switch);

        if (data->rate)
        {
            struct ifnode *ifnode;

            if (ifnode = findIFNode(prefs,name)) data->scale = ifnode->scale;
            else data->scale = DEF_Scale;

            set(data->rate,MUIA_Gauge_Max,data->scale);
        }

        data->this = self;

        message->MethodID = OM_SET;
        DoMethodA(self,(Msg)message);
        message->MethodID = OM_NEW;
    }

    return (ULONG)self;
}

/***********************************************************************/

static void
showBar(Object *self,struct MiamiPanelIfClass_DATA *data,ULONG show)
{
    if (data->busy)
    {
        Object *parent;

        get(self,MUIA_Parent,&parent);
        if (parent) DoMethod(parent,MUIM_Group_InitChange);
        DoMethod(data->g,MUIM_Group_InitChange);

        if (show)
        {
            DoMethod(data->g,OM_ADDMEMBER,(ULONG)data->busy);
            DoMethod(data->g,MUIM_Group_Sort,(ULONG)data->name,/*(ULONG)data->slabel,*/(ULONG)data->state,(ULONG)data->busy,(ULONG)data->lbutton,NULL);
        }
        else DoMethod(data->g,OM_REMMEMBER,(ULONG)data->busy);

        DoMethod(data->g,MUIM_Group_ExitChange);
        if (parent) DoMethod(parent,MUIM_Group_ExitChange);
    }
}

/***********************************************************************/

static ULONG
changeState(Object *self,struct MiamiPanelIfClass_DATA *data,ULONG state)
{
    ULONG sstring, busy;

    if (state & MIAMIPANELV_AddInterface_State_GoingOnline) state = MIAMIPANELV_AddInterface_State_GoingOnline, sstring = MSG_IF_Status_GoingOnline, busy = TRUE;
    else if (state & MIAMIPANELV_AddInterface_State_GoingOffline) state = MIAMIPANELV_AddInterface_State_GoingOffline, sstring = MSG_IF_Status_GoingOffline, busy = TRUE;
         else if (state & MIAMIPANELV_AddInterface_State_Suspending) state = MIAMIPANELV_AddInterface_State_Suspending, sstring = MSG_IF_Status_Suspending, busy = TRUE;
              else if (state & MIAMIPANELV_AddInterface_State_Offline) state = MIAMIPANELV_AddInterface_State_Offline, sstring = MSG_IF_Status_Offline, busy = FALSE;
                   else if (state & MIAMIPANELV_AddInterface_State_Online) state = MIAMIPANELV_AddInterface_State_Online, sstring = MSG_IF_Status_Online, busy = FALSE;
                        else if (state & MIAMIPANELV_AddInterface_State_Suspended) state = MIAMIPANELV_AddInterface_State_Suspended, sstring = MSG_IF_Status_Suspended, busy = FALSE;
                             else state = MIAMIPANELV_AddInterface_State_Offline, sstring = MSG_IF_Status_Offline, busy = FALSE;

    data->statev = state;

    if (data->lbutton) set(data->lbutton,MPA_LButton_State,state);
    set(data->state,MUIA_Text_Contents, __(sstring));

    if ((data->flags & FLG_UseBusyBar) && !BOOLSAME(data->flags & FLG_BusyBarInUse,busy))
        showBar(self,data,busy);

    if (busy) data->flags |= FLG_BusyBarInUse;
    else data->flags &= ~FLG_BusyBarInUse;

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_If__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelIfClass_DATA    *data = INST_DATA(CLASS,self);
    struct TagItem *tag;
    struct TagItem *tstate;

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_If_Name:
                stccpy(data->namev,(UBYTE *)tidata,sizeof(data->namev));
                set(data->name,MUIA_Text_Contents,tidata);
                break;

            case MPA_If_State:
                changeState(self,data,tidata);
                break;

            case MPA_If_Ontime:
                data->ontimev = tidata;
                break;

            case MPA_If_Now:
                if (data->ontime) set(data->ontime,MPA_Value,tidata-data->ontimev);
                break;

            case MPA_If_Traffic:
                if (data->traffic) set(data->traffic,MPA_Value,tidata);
                break;

            case MPA_If_Rate:
                if (data->rate) set(data->rate,MPA_Value,tidata);
                break;

            case MPA_If_Speed:
                if (data->speed) set(data->speed,MUIA_Text_Contents,tidata);
                break;

            case MPA_Prefs:
                if (!BOOLSAME(data->flags & FLG_UseBusyBar,PREFS(tidata)->flags & MPV_Flags_UseBusyBar))
                {
                    if (PREFS(tidata)->flags & MPV_Flags_UseBusyBar) data->flags |= FLG_UseBusyBar;
                    else data->flags &= ~FLG_UseBusyBar;

                    if (data->flags & FLG_BusyBarInUse)
                        showBar(self,data,data->flags & FLG_UseBusyBar);
                }

                if (data->rate)
                {
                    struct ifnode *ifnode;

                    if (ifnode = findIFNode(PREFS(tidata),data->namev))
                        set(data->rate, MUIA_Gauge_Max, (data->scale = ifnode->scale));
                }
                break;
        }
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

static ULONG
MUIPC_If__OM_GET(struct IClass *CLASS,Object *self,struct opGet *message)
{
    struct MiamiPanelIfClass_DATA *data = INST_DATA(CLASS,self);

    switch(message->opg_AttrID)
    {
        case MPA_If_Unit:   *message->opg_Storage = data->unitv;        return TRUE;
        case MPA_If_State:  *message->opg_Storage = data->statev;       return TRUE;
        case MPA_If_Name:   *message->opg_Storage = (ULONG)data->namev; return TRUE;
        case MPA_If_Scale:  *message->opg_Storage = data->scale;        return TRUE;
        default: return DoSuperMethodA(CLASS,self,(Msg)message);
    }
}

/***********************************************************************/

static ULONG
MUIPC_If__MPM_If_Switch(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelIfClass_DATA *data = INST_DATA(CLASS,self);

    switch (data->statev)
    {
        case MIAMIPANELV_AddInterface_State_GoingOnline:
        case MIAMIPANELV_AddInterface_State_Online:
            MiamiPanelFun(MIAMIPANELV_CallBack_Code_UnitOffline,data->unitv);
            break;

        case MIAMIPANELV_AddInterface_State_Suspending:
        case MIAMIPANELV_AddInterface_State_Suspended:
            break;

        default:
            MiamiPanelFun(MIAMIPANELV_CallBack_Code_UnitOnline,data->unitv);
            break;
    }

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_If__MUIM_ContextMenuBuild(struct IClass *CLASS,Object *self,struct MUIP_ContextMenuBuild *message)
{
    struct MiamiPanelIfClass_DATA *data = INST_DATA(CLASS,self);

    if (data->cmenu)
    {
        ULONG on, off;

        switch (data->statev)
        {
            case MIAMIPANELV_AddInterface_State_GoingOnline:
            case MIAMIPANELV_AddInterface_State_Online:
                on  = FALSE;
                off = TRUE;
                break;

            case MIAMIPANELV_AddInterface_State_Suspending:
            case MIAMIPANELV_AddInterface_State_Suspended:
                on  = off = FALSE;
                break;

            default:
                on  = TRUE;
                off = FALSE;
                break;
        }

        set(data->cmtitle,MUIA_Menu_Title, data->namev);
        set(data->cmonline,MUIA_Menuitem_Enabled,on);
        set(data->cmoffline,MUIA_Menuitem_Enabled,off);

        if (data->rate)
            set((Object *)DoMethod(data->cmenu,MUIM_FindUData,valueToID(data->scale)),MUIA_Menuitem_Checked,TRUE);

        return (ULONG)data->cmenu;
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

static ULONG
MUIPC_If__MUIM_ContextMenuChoice(struct IClass *CLASS,Object *self,struct MUIP_ContextMenuChoice *message)
{
    struct MiamiPanelIfClass_DATA *data = INST_DATA(CLASS,self);
    ULONG       item = muiUserData(message->item);

    switch (item)
    {
        case MSG_IFGroup_CItem_Online:
        case MSG_IFGroup_CItem_Offline:
            DoMethod(data->this,MPM_If_Switch);
            break;

        case TAG_SCALE_1: case TAG_SCALE_2:  case TAG_SCALE_3:
        case TAG_SCALE_4: case TAG_SCALE_5:  case TAG_SCALE_6:
        case TAG_SCALE_7: case TAG_SCALE_8:
            if (data->rate)
            {
                data->scale = IDToValue((ULONG)item, MiamiPanelBaseIntern);
                set(data->rate,MUIA_Gauge_Max,data->scale);
            }
            break;
    }

    return 0;
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_If_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                 return MUIPC_If__OM_NEW(CLASS,self,(APTR)message);
        case OM_SET:                 return MUIPC_If__OM_SET(CLASS,self,(APTR)message);
        case OM_GET:                 return MUIPC_If__OM_GET(CLASS,self,(APTR)message);
        case MUIM_ContextMenuChoice: return MUIPC_If__MUIM_ContextMenuChoice(CLASS,self,(APTR)message);
        case MUIM_ContextMenuBuild:  return MUIPC_If__MUIM_ContextMenuBuild(CLASS,self,(APTR)message);
        case MPM_If_Switch:          return MUIPC_If__MPM_If_Switch(CLASS,self,(APTR)message);
        default:                     return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_If_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    return (ULONG)(MiamiPanelBaseIntern->mpb_ifClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct MiamiPanelIfClass_DATA), MUIPC_If_Dispatcher));
}

/***********************************************************************/
