
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"

#include <mui/TheBar_mcc.h>

/***********************************************************************/

struct MiamiPanelMGroupClass_DATA
{
    Object                      *ifGroup;
    Object                      *ifList;
    Object                      *bar;
    Object                      *scrollBar;

    struct MPS_TinyPrefs        prefs;

    ULONG                       layout;

    #ifdef __MORPHOS__
    struct MUI_EventHandlerNode eh;
    struct Window               *win;
    #endif

    ULONG                       flags;
};

enum
{
    FLG_Visible         = 1<<0,
    FLG_BWin            = 1<<1,
    FLG_Handler         = 1<<2,
    FLG_Special         = 1<<3,
    FLG_Border          = 1<<4,
};

/***********************************************************************/

enum
{
    B_SHOW = 0,
    B_HIDE = 1,
    B_QUIT = 2,
};

UBYTE *pics[] =
{
    "Show",
    "Hide",
    "Quit",
    NULL
};

static ULONG buttonsIDs[] =
{
    MSG_Button_Show, MSG_Button_Show_Help,
    MSG_Button_Hide, MSG_Button_Hide_Help,
    MSG_Button_Quit, MSG_Button_Quit_Help
};

static struct MUIS_TheBar_Button buttons[] =
{
    {0, B_SHOW},
    {1, B_HIDE},
    {2, B_QUIT},
    {MUIV_TheBar_End},
};

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_MGroup__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct TagItem   *attrs = message->ops_AttrList;
    Object           *ifGroup, *ifList;
    struct MPS_Prefs *prefs = (struct MPS_Prefs *)GetTagData(MPA_Prefs,NULL,attrs);
    ULONG            show = GetTagData(MPA_Show,MIAMIPANELV_Init_Flags_Control,attrs);

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Group_Horiz, !(prefs->layout & MPV_Layout_Horiz),
            Child, ifGroup = ScrollgroupObject,
                MUIA_ShortHelp,                __(MSG_IFGroup_Help),
                MUIA_CycleChain,               TRUE,
                MUIA_Scrollgroup_FreeHoriz,    FALSE,
                MUIA_Scrollgroup_UseWinBorder, TRUE,
                MUIA_Scrollgroup_Contents, (ifList = NewObject(MiamiPanelBaseIntern->mpb_ifGroupClass->mcc_Class, NULL,
                    MPA_Prefs, prefs,
                    MPA_Show,  show,
                TAG_DONE)),
            End,
            TAG_MORE, attrs))
    {
        struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

        data->ifGroup = ifGroup;
        data->ifList  = ifList;

        if ((show & MIAMIPANELV_Init_Flags_Control) &&
            (data->bar = TheBarVirtObject,
                    MUIA_Group_Horiz,             prefs->layout & MPV_Layout_Horiz,
                    MUIA_TheBar_ViewMode,         prefs->viewMode,
                    MUIA_TheBar_LabelPos,         prefs->labelPos,
                    MUIA_TheBar_BarPos,           prefs->barLayout,
                    MUIA_TheBar_Borderless,       prefs->btflags & MPV_BTFlags_Borderless,
                    MUIA_TheBar_Raised,           prefs->btflags & MPV_BTFlags_Raised,
                    MUIA_TheBar_Sunny,            prefs->btflags & MPV_BTFlags_Sunny,
                    MUIA_TheBar_Scaled,           prefs->btflags & MPV_BTFlags_Scaled,
                    MUIA_TheBar_EnableKeys,       prefs->btflags & MPV_BTFlags_Underscore,
                    MUIA_TheBar_DragBar,          prefs->btflags & MPV_BTFlags_DragBar,
                    MUIA_TheBar_Frame,            prefs->btflags & MPV_BTFlags_Frame,
                    MUIA_TheBar_Buttons,          buttons,
                    MUIA_TheBar_PicsDrawer,       "Miami:Libs/Pics",
                    MUIA_TheBar_Pics,             pics,
            End))
        {
            DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)data->bar);

            if (prefs->layout & (MPV_Layout_Left|MPV_Layout_PureTop))
                DoSuperMethod(CLASS,self,MUIM_Group_Sort,(ULONG)data->bar,(ULONG)data->ifGroup,NULL);

            DoMethod(data->bar,MUIM_TheBar_DoOnButton,B_SHOW,MUIM_Notify,
                MUIA_Pressed,FALSE,MUIV_Notify_Application,2,MPM_Miami,MPV_Miami_Show);

            DoMethod(data->bar,MUIM_TheBar_DoOnButton,B_HIDE,MUIM_Notify,
                MUIA_Pressed,FALSE,MUIV_Notify_Application,2,MPM_Miami,MPV_Miami_Hide);

            DoMethod(data->bar,MUIM_TheBar_DoOnButton,B_QUIT,MUIM_Notify,
                MUIA_Pressed,FALSE,MUIV_Notify_Application,2,MPM_Miami,MPV_Miami_Quit);
        }

        get(data->ifGroup,MUIA_Scrollgroup_VertBar,&data->scrollBar);
        if (prefs->flags & MPV_Flags_BWin)
        {
            data->flags |= FLG_BWin;
            set(data->scrollBar,MUIA_ShowMe,FALSE);
        }

        CopyMem(prefs,&data->prefs,sizeof(data->prefs));
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MUIM_Setup(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (!DoSuperMethodA(CLASS,self,(Msg)message)) return FALSE;
    set(_win(self),MUIA_Window_DefaultObject,data->ifGroup);

    return TRUE;
}

/***********************************************************************/

#ifdef __MORPHOS__

static ULONG
isMouseOnBorder(struct Window *window)
{
    if (window->MouseX<0 || window->MouseY<0 || window->MouseX>window->Width-1 || window->MouseY>window->Height-1) return FALSE;

    if (window->MouseY>0 && window->MouseY < window->BorderTop) return TRUE;
    if (window->MouseY>window->Height-window->BorderBottom-1 && window->MouseY<window->Height) return TRUE;

    if (window->MouseX>0 && window->MouseX<window->BorderLeft) return TRUE;
    if (window->MouseX>window->Width-window->BorderRight-1 && window->MouseX<window->Width) return TRUE;

    return FALSE;
}

M_HOOK(trans,struct Window *window,struct TransparencyMessage *message)
{
    struct MiamiPanelMGroupClass_DATA      *data = hook->h_Data;
    struct Rectangle rect;
    ULONG            b;

    /* use transp only when window is active */
    if (!(window->Flags & WFLG_WINDOWACTIVE)) return 0;

    /* use transp only when pointer NOT on border */
    if (isMouseOnBorder(window)) return 0;

    b = (data->flags & FLG_BWin) ? 1 : 0;

    /* make top border transparent */
    rect.MinX = 0;
    rect.MinY = 0;
    rect.MaxX = window->Width - 1;
    rect.MaxY = window->BorderTop + b;
    
    OrRectRegion(message->Region,&rect);

    /* left border */
    rect.MinX = 0;
    rect.MinY = window->BorderTop;
    rect.MaxX = window->BorderLeft - 1 + b;
    rect.MaxY = window->Height - window->BorderBottom - 1;

    OrRectRegion(message->Region,&rect);

    /* right border */
    rect.MinX = window->Width - window->BorderRight - b;
    rect.MinY = window->BorderTop;
    rect.MaxX = window->Width - 1;
    rect.MaxY = window->Height - window->BorderBottom - 1;

    OrRectRegion(message->Region,&rect);

    /* bottom border */
    rect.MinX = 0;
    rect.MinY = window->Height - window->BorderBottom - b;
    rect.MaxX = window->Width - 1;
    rect.MaxY = window->Height - 1;

    OrRectRegion(message->Region,&rect);

    return 0;
}

static void
switchTransparency(struct IClass *CLASS,Object *self,struct MiamiPanelMGroupClass_DATA *data,ULONG use)
{
    struct TagItem tags[] = {{TRANSPCONTROL_REGIONHOOK,0},{TAG_DONE,0}};

    if (use)
    {
        if (!(data->flags & FLG_Handler))
        {
            trans_hook.h_Data = data;

            tags[0].ti_Data   = (ULONG)&trans_hook;
            TransparencyControl(data->win,TRANSPCONTROLMETHOD_INSTALLREGIONHOOK,tags);

            memset(&data->eh,0,sizeof(data->eh));
            data->eh.ehn_Class  = CLASS;
            data->eh.ehn_Object = self;
            data->eh.ehn_Events = IDCMP_INTUITICKS;
            DoMethod(_win(self),MUIM_Window_AddEventHandler,(ULONG)&data->eh);

            data->flags |= FLG_Handler;
        }
    }
    else
    {
        if (MiamiPanelMGroupClass_DATA->flags & FLG_Handler)
        {
            DoMethod(_win(self),MUIM_Window_RemEventHandler,(ULONG)&data->eh);

            TransparencyControl(data->win,TRANSPCONTROLMETHOD_INSTALLREGIONHOOK,tags);

        data->flags &= ~FLG_Handler;
        }
    }
}

static ULONG
MUIPC_MGroup__MUIM_Show(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (!DoSuperMethodA(CLASS,self,(Msg)message)) return FALSE;

    get(_win(self),MUIA_Window_Window,&data->win);
    data->flags |= FLG_Visible;

    switchTransparency(CLASS,self,data,data->prefs.flags & MPV_Flags_UseTransparency);

    return TRUE;
}

static ULONG
MUIPC_MGroup__MUIM_Hide(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);
    
    switchTransparency(CLASS,self,data,FALSE);

    data->flags &= ~FLG_Visible;

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

static ULONG
MUIPC_MGroup__MUIM_HandleEvent(struct IClass *CLASS,Object *self,struct MUIP_HandleEvent *message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (data->flags & FLG_Handler)
    {
        ULONG b;

        if (!BOOLSAME(b = isMouseOnBorder(data->win),data->flags & FLG_Border))
    {
            if (b) data->flags |= FLG_Border;
            else data->flags &= ~FLG_Border;

            TransparencyControl(data->win,TRANSPCONTROLMETHOD_UPDATETRANSPARENCY,NULL);
    }
    }

    return 0;
}
#endif

/***********************************************************************/

static ULONG
MUIPC_MGroup__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelMGroupClass_DATA             *data = INST_DATA(CLASS,self);
    register struct TagItem *tag;
    struct TagItem          *tstate;
    #ifdef __MORPHOS__
    register ULONG          res, win = FALSE;
    #endif

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Prefs:
                if (memcmp(&data->prefs,PREFS(tidata),sizeof(data->prefs)))
                {
                    if (data->bar)
                    {
                        ULONG layoutChanged = data->prefs.layout!=PREFS(tidata)->layout;

                        if (layoutChanged)
                        {
                            DoSuperMethod(CLASS,self,MUIM_Group_InitChange);

                            SetSuperAttrs(CLASS,self,MUIA_Group_Horiz,!(PREFS(tidata)->layout & MPV_Layout_Horiz),TAG_DONE);

                            if (PREFS(tidata)->layout & (MPV_Layout_Left|MPV_Layout_PureTop))
                                DoSuperMethod(CLASS,self,MUIM_Group_Sort,(ULONG)data->bar,(ULONG)data->ifGroup,NULL);
                            else DoSuperMethod(CLASS,self,MUIM_Group_Sort,(ULONG)data->ifGroup,(ULONG)data->bar,NULL);
                        }

                        if (layoutChanged || memcmp(&data->prefs.barLayout,&PREFS(tidata)->barLayout,sizeof(ULONG)*4))
                        {
                            DoMethod(data->bar,MUIM_Group_InitChange);

                            SetAttrs(data->bar,MUIA_Group_Horiz,             PREFS(tidata)->layout & MPV_Layout_Horiz,
                                               MUIA_TheBar_BarPos,           PREFS(tidata)->barLayout,
                                               MUIA_TheBar_ViewMode,         PREFS(tidata)->viewMode,
                                               MUIA_TheBar_LabelPos,         PREFS(tidata)->labelPos,
                                               MUIA_TheBar_Borderless,       PREFS(tidata)->btflags & MPV_BTFlags_Borderless,
                                               MUIA_TheBar_Sunny,            PREFS(tidata)->btflags & MPV_BTFlags_Sunny,
                                               MUIA_TheBar_Raised,           PREFS(tidata)->btflags & MPV_BTFlags_Raised,
                                               MUIA_TheBar_Scaled,           PREFS(tidata)->btflags & MPV_BTFlags_Scaled,
                                               MUIA_TheBar_EnableKeys,       PREFS(tidata)->btflags & MPV_BTFlags_Underscore,
                                               MUIA_TheBar_Frame,            PREFS(tidata)->btflags & MPV_BTFlags_Frame,
                                               MUIA_TheBar_DragBar,          PREFS(tidata)->btflags & MPV_BTFlags_DragBar,
                                               TAG_DONE);

                            DoMethod(data->bar,MUIM_Group_ExitChange);
                        }

                        if (layoutChanged) DoSuperMethod(CLASS,self,MUIM_Group_ExitChange);
                    }

                    if (PREFS(tidata)->flags & MPV_Flags_BWin) data->flags |= FLG_BWin;
                    else data->flags &= ~FLG_BWin;

                    #ifdef __MORPHOS__
                    if ((data->flags & FLG_Visible) && !BOOLSAME(PREFS(tidata)->flags & MPV_Flags_UseTransparency,data->prefs.flags & MPV_Flags_UseTransparency))
                        if (PREFS(tidata)->flags & MPV_Flags_UseTransparency) win = TRUE;
                        else switchTransparency(CLASS,self,data,FALSE);
                    #endif

                    CopyMem(PREFS(tidata),&data->prefs,sizeof(data->prefs));
                }
                break;

            case MPA_Bar:
                set(data->scrollBar,MUIA_ShowMe,tidata);
                break;
        }
    }

    #ifdef __MORPHOS__
    res = DoSuperMethodA(CLASS,self,(Msg)message);

    if (win)
    {
        DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_win(self),3,MUIM_Set,MUIA_Window_Open,FALSE);
        DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_win(self),3,MUIM_Set,MUIA_Window_Open,TRUE);
        DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_win(self),3,MUIM_Set,MUIA_Window_Activate,TRUE);
        DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)self,1,MPM_MGroup_UpdateTransparency);
    }

    return res;
    #else
    return DoSuperMethodA(CLASS,self,(Msg)message);
    #endif
}

/***********************************************************************/
#define DD_FACT 5

static ULONG
MUIPC_MGroup__MUIM_DragQuery(struct IClass *CLASS,Object *self,struct MUIP_DragQuery *message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (message->obj!=data->bar) return MUIV_DragQuery_Refuse;

    return MUIV_DragQuery_Accept;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MUIM_DragBegin(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    data->layout = data->prefs.layout;
    set(data->ifList,MPA_SkipBar,TRUE);

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MUIM_DragReport(struct IClass *CLASS,Object *self,struct MUIP_DragReport *message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);
    ULONG       layout;
    LONG        x, y, l, t, r, b, w, h;

    if (!message->update) return MUIV_DragReport_Refresh;

    x = message->x;
    y = message->y;

    l = _mleft(self);
    t = _mtop(self);
    r = _mright(self);
    b = _mbottom(self);

    w = _mwidth(self)/DD_FACT;
    h = _mheight(self)/DD_FACT;

    layout = 0;

    switch (data->prefs.layout)
    {
        case MPV_Layout_Left:
            if (x<l+w) layout = MPV_Layout_Left;
            else if (y<t+h) layout = MPV_Layout_Top;
                 else if (y>b-h) layout = MPV_Layout_Bottom;
                      else if (x>r-w) layout = MPV_Layout_Right;
            break;

        case MPV_Layout_Right:
            if (x>r-w) layout = MPV_Layout_Right;
            else if (y<t+h) layout = MPV_Layout_Top;
                 else if (y>b-h) layout = MPV_Layout_Bottom;
                      else if (x<l+w) layout = MPV_Layout_Left;
            break;

        case MPV_Layout_Top:
            if (y<t+h) layout = MPV_Layout_Top;
            else if (y>b-h) layout = MPV_Layout_Bottom;
                 else if (x<l+w) layout = MPV_Layout_Left;
                      else if (x>r-w) layout = MPV_Layout_Right;
                           else data->layout = 0;
            break;

        case MPV_Layout_Bottom:
            if (y>b-h) layout = MPV_Layout_Bottom;
            else if (y<t+h) layout = MPV_Layout_Top;
                 else if (x<l+w) layout = MPV_Layout_Left;
                      else if (x>r-w) layout = MPV_Layout_Right;
                           else data->layout = 0;
            break;
    }

    if (!layout) return MUIV_DragReport_Abort;

    if (data->prefs.layout!=layout)
    {
        set(data->bar,MUIA_TheBar_Limbo,TRUE);
        DoSuperMethod(CLASS,self,MUIM_Group_InitChange);

        SetSuperAttrs(CLASS,self,MUIA_Group_Horiz,!(layout & MPV_Layout_Horiz),TAG_DONE);
        set(data->bar,MUIA_Group_Horiz,layout & MPV_Layout_Horiz);

        if (layout & (MPV_Layout_Left|MPV_Layout_PureTop))
            DoSuperMethod(CLASS,self,MUIM_Group_Sort,(ULONG)data->bar,(ULONG)data->ifGroup,NULL);
        else DoSuperMethod(CLASS,self,MUIM_Group_Sort,(ULONG)data->ifGroup,(ULONG)data->bar,NULL);

        DoSuperMethod(CLASS,self,MUIM_Group_ExitChange);
        set(data->bar,MUIA_TheBar_Limbo,FALSE);

        if (!BOOLSAME(data->prefs.layout & MPV_Layout_Horiz,layout & MPV_Layout_Horiz))
            data->flags |= FLG_Special;
        data->prefs.layout = layout;

        return MUIV_DragReport_Abort;
    }

    return MUIV_DragReport_Continue;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MUIM_DragFinish(struct IClass *CLASS,Object *self,struct MUIP_DragFinish *message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (data->flags & FLG_Special)
    {
        DoSuperMethod(CLASS,self,MUIM_Group_InitChange);
        DoSuperMethod(CLASS,self,MUIM_Group_ExitChange);
        data->flags &= ~FLG_Special;
    }

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MUIM_DragDrop(struct IClass *CLASS,Object *self,struct MUIP_DragDrop *message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    if (data->prefs.layout!=data->layout)
    {
        struct MPS_Prefs prefs, *p;
        Object           *app = _app(self);

        get(app,MPA_Prefs,&p);
        CopyMem(p,&prefs,sizeof(prefs));
        prefs.layout = data->prefs.layout;
        SetAttrs(app,MPA_Prefs,(ULONG)&prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
    }

    set(data->ifList,MPA_SkipBar,FALSE);

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_MGroup__MPM_MGroup_GrabIFList(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    return DoMethod(data->ifList,MPM_IfGroup_GrabIFList);
}

/***********************************************************************/

#ifdef __MORPHOS__
static ULONG
MUIPC_MGroup__MPM_MGroup_UpdateTransparency(struct IClass *CLASS,Object *self,Msg message)
{
    struct MiamiPanelMGroupClass_DATA *data = INST_DATA(CLASS,self);

    TransparencyControl(data->win,TRANSPCONTROLMETHOD_UPDATETRANSPARENCY,NULL);
    return 0;
}
#endif

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_MGroup_Dispatcher, CLASS, self, message)
{
    switch(message->MethodID)
    {
        case OM_NEW:                         return MUIPC_MGroup__OM_NEW(CLASS,self,(APTR)message);
        case OM_SET:                         return MUIPC_MGroup__OM_SET(CLASS,self,(APTR)message);
        case MUIM_Setup:                     return MUIPC_MGroup__MUIM_Setup(CLASS,self,(APTR)message);

        #ifdef __MORPHOS__
        case MUIM_Show:                      return MUIPC_MGroup__MUIM_Show(CLASS,self,(APTR)message);
        case MUIM_Hide:                      return MUIPC_MGroup__MUIM_Hide(CLASS,self,(APTR)message);
        case MUIM_HandleEvent:               return MUIPC_MGroup__MUIM_HandleEvent(CLASS,self,(APTR)message);
        #endif

        case MUIM_DragQuery:                 return MUIPC_MGroup__MUIM_DragQuery(CLASS,self,(APTR)message);
        case MUIM_DragBegin:                 return MUIPC_MGroup__MUIM_DragBegin(CLASS,self,(APTR)message);
        case MUIM_DragDrop:                  return MUIPC_MGroup__MUIM_DragDrop(CLASS,self,(APTR)message);
        case MUIM_DragReport:                return MUIPC_MGroup__MUIM_DragReport(CLASS,self,(APTR)message);
        case MUIM_DragFinish:                return MUIPC_MGroup__MUIM_DragFinish(CLASS,self,(APTR)message);

        case MPM_MGroup_GrabIFList:          return MUIPC_MGroup__MPM_MGroup_GrabIFList(CLASS,self,(APTR)message);
        #ifdef __MORPHOS__
        case MPM_MGroup_UpdateTransparency:  return MUIPC_MGroup__MPM_MGroup_UpdateTransparency(CLASS,self,(APTR)message);
        #endif

        default:                             return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_MGroup_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_mgroupClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct MiamiPanelMGroupClass_DATA), MUIPC_MGroup_Dispatcher))
    {
        localizeButtonsBar(buttons,buttonsIDs);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/

