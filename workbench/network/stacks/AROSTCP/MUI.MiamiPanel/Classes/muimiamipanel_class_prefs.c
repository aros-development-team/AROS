
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"

/***********************************************************************/

#define IDSSIZE(n) (sizeof(n)/sizeof(ULONG))

static ULONG registerLabelsIDs[] =
{
    MSG_Prefs_Interfaces,
    MSG_Prefs_MiamiCtrls,
    MSG_Prefs_Options,
    0
};
static UBYTE *registerLabels[IDSSIZE(registerLabelsIDs)];

static ULONG tformatsIDs[] =
{
    MSG_Prefs_IF_Traffic_Format_Long,
    MSG_Prefs_IF_Traffic_Format_Short,
    0
};
static UBYTE *tformats[IDSSIZE(tformatsIDs)];

static ULONG rformatsIDs[] =
{
    MSG_Prefs_IF_Rate_Format_Long,
    MSG_Prefs_IF_Rate_Format_Short,
    0
};
static UBYTE *rformats[IDSSIZE(rformatsIDs)];

static ULONG positionsIDs[] =
{
    MSG_Prefs_Miami_Position_Bottom,
    MSG_Prefs_Miami_Position_Top,
    MSG_Prefs_Miami_Position_Right,
    MSG_Prefs_Miami_Position_Left,
    0
};
static UBYTE *positions[IDSSIZE(positionsIDs)];

static ULONG vmodesIDs[] =
{
    MSG_Prefs_Miami_ViewMode_TextGfx,
    MSG_Prefs_Miami_ViewMode_Gfx,
    MSG_Prefs_Miami_ViewMode_Text,
    0
};
static UBYTE *vmodes[IDSSIZE(vmodesIDs)];

static ULONG lpossIDs[] =
{
    MSG_Prefs_Miami_LabelPosition_Bottom,
    MSG_Prefs_Miami_LabelPosition_Top,
    MSG_Prefs_Miami_LabelPosition_Right,
    MSG_Prefs_Miami_LabelPosition_Left,
    0
};
static UBYTE *lposs[IDSSIZE(lpossIDs)];

static ULONG oniconifiesIDs[] =
{
    MSG_Prefs_OI_Quit,
    MSG_Prefs_OI_Hide,
    0
};
static UBYTE *oniconifies[IDSSIZE(oniconifiesIDs)];

static ULONG sblayoutsIDs[] =
{
    MSG_Prefs_Miami_BarLayout_Left,
    MSG_Prefs_Miami_BarLayout_Center,
    MSG_Prefs_Miami_BarLayout_Right,
    0
};
static UBYTE *sblayouts[IDSSIZE(sblayoutsIDs)];

/***********************************************************************/

struct MiamiPanelPrefsClass_DATA
{
    Object           *layout;

    Object           *tformat;
    Object           *tgrouping;
    Object           *rformat;
    Object           *rgrouping;
    Object           *usebusybar;
    Object           *oniconify;
    Object           *bwin;
    Object           *bwinBorders;
    #ifdef __MORPHOS__
    Object       *useTransparency;
    #endif

    Object           *barLayout;
    Object           *viewMode;
    Object           *labelPos;
    Object           *borderless;
    Object           *sunny;
    Object           *raised;
    Object           *scaled;
    Object           *underscore;
    Object           *frame;
    Object           *dragBar;

    struct MPS_Prefs prefs;

    ULONG            flags;
};

enum
{
    FLG_Test = 1<<0,
};

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static void
gadgetsToPrefs(struct MiamiPanelPrefsClass_DATA *data,struct MPS_Prefs *prefs)
{
    /* layout */
    switch (XGET(data->layout,MUIA_Cycle_Active))
    {
        case 0: prefs->layout = MPV_Layout_Bottom; break;
        case 1: prefs->layout = MPV_Layout_Top;    break;
        case 2: prefs->layout = MPV_Layout_Right;  break;
        case 3: prefs->layout = MPV_Layout_Left;   break;
    }

    /* flags */
    prefs->flags = 0;
    if (XGET(data->tformat,MUIA_Cycle_Active)) prefs->flags |= MPV_Flags_TrafficShort;
    if (!XGET(data->tgrouping,MUIA_Selected)) prefs->flags |= MPV_Flags_TrafficNoGrouping;
    if (XGET(data->rformat,MUIA_Cycle_Active)) prefs->flags |= MPV_Flags_RateShort;
    if (!XGET(data->rgrouping,MUIA_Selected)) prefs->flags |= MPV_Flags_RateNoGrouping;
    if (XGET(data->usebusybar,MUIA_Selected)) prefs->flags |= MPV_Flags_UseBusyBar;
    if (XGET(data->oniconify,MUIA_Cycle_Active)) prefs->flags |= MPV_Flags_Iconify;
    if (XGET(data->bwin,MUIA_Selected)) prefs->flags |= MPV_Flags_BWin;
    if (XGET(data->bwinBorders,MUIA_Selected)) prefs->flags |= MPV_Flags_BWinBorders;
    #ifdef __MORPHOS__
    if (XGET(data->useTransparency,MUIA_Selected)) prefs->flags |= MPV_Flags_UseTransparency;
    #endif

    /* barLayout */
    prefs->barLayout = XGET(data->barLayout,MUIA_Cycle_Active);

    /* viewMode */
    prefs->viewMode = XGET(data->viewMode,MUIA_Cycle_Active);

    /* labelPos */
    prefs->labelPos = XGET(data->labelPos,MUIA_Cycle_Active);

    /* btflags */
    prefs->btflags = 0;
    if (XGET(data->borderless,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Borderless;
    if (XGET(data->sunny,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Sunny;
    if (XGET(data->raised,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Raised;
    if (XGET(data->scaled,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Scaled;
    if (XGET(data->underscore,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Underscore;
    if (XGET(data->frame,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_Frame;
    if (XGET(data->dragBar,MUIA_Selected)) prefs->btflags |= MPV_BTFlags_DragBar;
}

/***********************************************************************/

static void
prefsToGadgets(struct MiamiPanelPrefsClass_DATA *data,struct MPS_Prefs *prefs)
{
    register ULONG v = 0; // gcc

    /* layout */
    if (prefs->layout & MPV_Layout_PureBottom) v = 0;
    else if (prefs->layout & MPV_Layout_PureTop) v = 1;
         else if (prefs->layout & MPV_Layout_Right) v = 2;
              else if (prefs->layout & MPV_Layout_Left) v = 3;
    set(data->layout,MUIA_Cycle_Active,v);

    /* flags */
    set(data->tformat,MUIA_Cycle_Active,prefs->flags & MPV_Flags_TrafficShort);
    set(data->tgrouping,MUIA_Selected,!(prefs->flags & MPV_Flags_TrafficNoGrouping));
    set(data->rformat,MUIA_Cycle_Active,prefs->flags & MPV_Flags_RateShort);
    set(data->rgrouping,MUIA_Selected,!(prefs->flags & MPV_Flags_RateNoGrouping));
    set(data->usebusybar,MUIA_Selected,prefs->flags & MPV_Flags_UseBusyBar);
    set(data->oniconify,MUIA_Cycle_Active,prefs->flags & MPV_Flags_Iconify);
    set(data->bwin,MUIA_Selected,prefs->flags & MPV_Flags_BWin);
    set(data->bwinBorders,MUIA_Selected,prefs->flags & MPV_Flags_BWinBorders);
    #ifdef __MORPHOS__
    set(data->useTransparency,MUIA_Selected,prefs->flags & MPV_Flags_UseTransparency);
    #endif

    /* barLayout */
    set(data->barLayout,MUIA_Cycle_Active,prefs->barLayout);

    /* viewMode */
    set(data->viewMode,MUIA_Cycle_Active,prefs->viewMode);

    /* labelPos */
    set(data->labelPos,MUIA_Cycle_Active,prefs->labelPos);

    /* btflags */
    set(data->borderless,MUIA_Selected,prefs->btflags & MPV_BTFlags_Borderless);
    set(data->sunny,MUIA_Selected,prefs->btflags & MPV_BTFlags_Sunny);
    set(data->raised,MUIA_Selected,prefs->btflags & MPV_BTFlags_Raised);
    set(data->scaled,MUIA_Selected,prefs->btflags & MPV_BTFlags_Scaled);
    set(data->underscore,MUIA_Selected,prefs->btflags & MPV_BTFlags_Underscore);
    set(data->frame,MUIA_Selected,prefs->btflags & MPV_BTFlags_Frame);
    set(data->dragBar,MUIA_Selected,prefs->btflags & MPV_BTFlags_DragBar);

    CopyMem(prefs,&data->prefs,sizeof(struct MPS_Prefs));
}

/***********************************************************************/

static ULONG
MUIPC_Prefs__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelPrefsClass_DATA             temp;
    register struct TagItem *attrs = message->ops_AttrList;
    register Object         *save, *use, *apply, *test, *cancel;

    memset(&temp,0,sizeof(temp));

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Window_Title, __(MSG_Prefs_WinTitle),
            MUIA_Window_ID, MAKE_ID('W','P','R','E'),
            MUIA_HelpNode, "Prefs",

            WindowContents, VGroup, // Window contents

                Child, RegisterGroup(registerLabels), // Register
                    MUIA_CycleChain, TRUE,

                    Child, VGroup,  // Interfaces
                        VirtualFrame,

                        Child, ColGroup(2),
                            Child, olabel2(MSG_Prefs_IF_Traffic_Format, MiamiPanelBaseIntern),
                            Child, HGroup,
                                Child, temp.tformat = ocycle(MSG_Prefs_IF_Traffic_Format,tformats,MSG_Prefs_IF_Traffic_Format_Help, MiamiPanelBaseIntern),
                                Child, olabel1(MSG_Prefs_IF_Traffic_Grouping, MiamiPanelBaseIntern),
                                Child, temp.tgrouping = ocheck(MSG_Prefs_IF_Traffic_Grouping,MSG_Prefs_IF_Traffic_Grouping_Help, MiamiPanelBaseIntern),
                            End,

                            Child, olabel2(MSG_Prefs_IF_Rate_Format, MiamiPanelBaseIntern),
                            Child, HGroup,
                                Child, temp.rformat = ocycle(MSG_Prefs_IF_Rate_Format,rformats,MSG_Prefs_IF_Rate_Format_Help, MiamiPanelBaseIntern),
                                Child, olabel1(MSG_Prefs_IF_Rate_Grouping, MiamiPanelBaseIntern),
                                Child, temp.rgrouping = ocheck(MSG_Prefs_IF_Rate_Grouping,MSG_Prefs_IF_Rate_Grouping_Help, MiamiPanelBaseIntern),
                            End,
                        End,

                    Child, ScrollgroupObject,
                        MUIA_Scrollgroup_FreeHoriz, FALSE,
                        MUIA_Scrollgroup_Contents, VirtgroupObject,
                            MUIA_Frame, MUIV_Frame_Virtual,
                            Child, HGroup,
                                Child, ColGroup(2),
                                    Child, temp.usebusybar = ocheck(MSG_Prefs_IF_UseBusyBar,MSG_Prefs_IF_UseBusyBar_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_IF_UseBusyBar, MiamiPanelBaseIntern),
                                End,
                                Child, HSpace(0),
                            End,
                                Child, VSpace(0),
                        End,
                    End,
                    End, // Interfaces

                    Child, VGroup, // Miami ctrls
                        VirtualFrame,

                        Child, ColGroup(2),
                            Child, olabel2(MSG_Prefs_Miami_Position, MiamiPanelBaseIntern),
                            Child, temp.layout = ocycle(MSG_Prefs_Miami_Position,positions,MSG_Prefs_Miami_Position_Help, MiamiPanelBaseIntern),
                            Child, olabel2(MSG_Prefs_Miami_ViewMode, MiamiPanelBaseIntern),
                            Child, temp.viewMode = ocycle(MSG_Prefs_Miami_ViewMode,vmodes,MSG_Prefs_Miami_ViewMode_Help, MiamiPanelBaseIntern),
                            Child, olabel2(MSG_Prefs_Miami_BarLayout, MiamiPanelBaseIntern),
                            Child, temp.barLayout = ocycle(MSG_Prefs_Miami_BarLayout,sblayouts,MSG_Prefs_Miami_BarLayout_Help, MiamiPanelBaseIntern),
                            Child, olabel2(MSG_Prefs_Miami_LabelPosition, MiamiPanelBaseIntern),
                            Child, temp.labelPos = ocycle(MSG_Prefs_Miami_LabelPosition,lposs,MSG_Prefs_Miami_LabelPosition_Help, MiamiPanelBaseIntern),
                        End,

                        Child, ScrollgroupObject,
                            MUIA_Scrollgroup_FreeHoriz, FALSE,
                            MUIA_Scrollgroup_Contents, VirtgroupObject,
                                MUIA_Frame, MUIV_Frame_Virtual,
                                Child, HGroup,
                                    Child, ColGroup(2),
                                        Child, temp.borderless = ocheck(MSG_Prefs_Miami_Borderless,MSG_Prefs_Miami_Borderless_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Borderless, MiamiPanelBaseIntern),
                                        Child, temp.sunny = ocheck(MSG_Prefs_Miami_Sunny,MSG_Prefs_Miami_Sunny_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Sunny, MiamiPanelBaseIntern),
                                        Child, temp.raised = ocheck(MSG_Prefs_Miami_Raised,MSG_Prefs_Miami_Raised_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Raised, MiamiPanelBaseIntern),
                                        Child, temp.scaled = ocheck(MSG_Prefs_Miami_Scaled,MSG_Prefs_Miami_Scaled_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Scaled, MiamiPanelBaseIntern),
                                        Child, temp.underscore = ocheck(MSG_Prefs_Miami_Underscore,MSG_Prefs_Miami_Underscore_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Underscore, MiamiPanelBaseIntern),
                                        Child, temp.frame = ocheck(MSG_Prefs_Miami_Frame,MSG_Prefs_Miami_Frame_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_Frame, MiamiPanelBaseIntern),
                                        Child, temp.dragBar = ocheck(MSG_Prefs_Miami_DragBar,MSG_Prefs_Miami_DragBar_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_Miami_DragBar, MiamiPanelBaseIntern),
                                    End,
                                    Child, HSpace(0),
                                End,
                                Child, VSpace(0),
                            End,
                        End,
                    End, // Miami ctrls


                    Child, VGroup, // Options
                        VirtualFrame,

                        Child, ColGroup(2),
                            Child, olabel2(MSG_Prefs_OnIconify, MiamiPanelBaseIntern),
                            Child, temp.oniconify = ocycle(MSG_Prefs_OnIconify,oniconifies,MSG_Prefs_OnIconify_Help, MiamiPanelBaseIntern),
                        End,

                        Child, ScrollgroupObject,
                            MUIA_Scrollgroup_FreeHoriz, FALSE,
                            MUIA_Scrollgroup_Contents, VirtgroupObject,
                                MUIA_Frame, MUIV_Frame_Virtual,
                                Child, HGroup,
                                    Child, ColGroup(2),
                                        Child, temp.bwin = ocheck(MSG_Prefs_BWin,MSG_Prefs_BWin_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_BWin, MiamiPanelBaseIntern),
                                        Child, temp.bwinBorders = ocheck(MSG_Prefs_BWinBorders,MSG_Prefs_BWinBorders_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_BWinBorders, MiamiPanelBaseIntern),
                                        #ifdef __MORPHOS__
                                        Child, temp.useTransparency = ocheck(MSG_Prefs_UseTransparency,MSG_Prefs_UseTransparency_Help, MiamiPanelBaseIntern),
                                        Child, ollabel1(MSG_Prefs_UseTransparency, MiamiPanelBaseIntern),
                                        #endif
                                    End,
                                    Child, HSpace(0),
                                End,
                                Child, VSpace(0),
                            End,
                        End,
                     End, // Options
                End,  // Register

                Child, HGroup, // Buttons
                    Child, save = obutton(MSG_Prefs_Save,MSG_Prefs_Save_Help, MiamiPanelBaseIntern),
                    Child, owspace(4),
                    Child, use = obutton(MSG_Prefs_Use,MSG_Prefs_Use_Help, MiamiPanelBaseIntern),
                    Child, owspace(4),
                    Child, apply = obutton(MSG_Prefs_Apply,MSG_Prefs_Apply_Help, MiamiPanelBaseIntern),
                    Child, owspace(4),
                    Child, test = obutton(MSG_Prefs_Test,MSG_Prefs_Test_Help, MiamiPanelBaseIntern),
                    Child, owspace(4),
                    Child, cancel = obutton(MSG_Prefs_Cancel,MSG_Prefs_Cancel_Help, MiamiPanelBaseIntern),
                End, // Buttons

            End, // Window contents

            TAG_MORE,attrs))
    {
        struct MiamiPanelPrefsClass_DATA      *data = INST_DATA(CLASS,self);
        struct MPS_Prefs *prefs = (struct MPS_Prefs *)GetTagData(MPA_Prefs,NULL,attrs);

        CopyMem(&temp,data,sizeof(struct MiamiPanelPrefsClass_DATA));

        DoMethod(self,MUIM_MultiSet,MUIA_Disabled,TRUE,(ULONG)data->raised,(ULONG)data->bwinBorders,TAG_DONE);

        DoSuperMethod(CLASS,self,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,
            MUIV_Notify_Self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Cancel);

        DoMethod(data->borderless,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,
            (ULONG)data->raised,3,MUIM_Set,MUIA_Disabled,MUIV_NotTriggerValue);

        DoMethod(data->bwin,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,
            (ULONG)data->bwinBorders,3,MUIM_Set,MUIA_Disabled,MUIV_NotTriggerValue);

        DoMethod(save,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Save);
        DoMethod(use,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Use);
        DoMethod(apply,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Apply);
        DoMethod(test,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Test);
        DoMethod(cancel,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)self,2,MPM_Prefs_UsePrefs,MPV_Prefs_UsePrefs_Cancel);

        prefsToGadgets(data,prefs);
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_Prefs__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct MiamiPanelPrefsClass_DATA    *data = INST_DATA(CLASS,self);
    register struct TagItem *tag;
    struct TagItem          *tstate;

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Prefs:
                prefsToGadgets(data,PREFS(tidata));
                break;
        }
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

static ULONG
MUIPC_Prefs__MPM_Prefs_UsePrefs(struct IClass *CLASS,Object *self,struct MPP_Prefs_UsePrefs *message)
{
    register struct MiamiPanelPrefsClass_DATA *data = INST_DATA(CLASS,self);
    register ULONG       close;

    if (close = message->mode!=MPV_Prefs_UsePrefs_Test)
        SetSuperAttrs(CLASS,self,MUIA_Window_Open,FALSE,TAG_DONE);

    switch (message->mode)
    {
        case MPV_Prefs_UsePrefs_Save:
            gadgetsToPrefs(data,&data->prefs);
            SetAttrs(_app(self),MPA_Prefs,(ULONG)&data->prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
            DoMethod(_app(self),MPM_Save,TRUE);
            DoMethod(_app(self),MPM_Save,FALSE);
            break;

        case MPV_Prefs_UsePrefs_Use:
            gadgetsToPrefs(data,&data->prefs);
            SetAttrs(_app(self),MPA_Prefs,(ULONG)&data->prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
            DoMethod(_app(self),MPM_Save,FALSE);
            break;

        case MPV_Prefs_UsePrefs_Apply:
            gadgetsToPrefs(data,&data->prefs);
            SetAttrs(_app(self),MPA_Prefs,(ULONG)&data->prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
            break;

        case MPV_Prefs_UsePrefs_Test:
        {
            struct MPS_Prefs prefs;

            gadgetsToPrefs(data,&prefs);
            SetAttrs(_app(self),MPA_Prefs,(ULONG)&prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
            data->flags |= FLG_Test;
            break;
        }

        case MPV_Prefs_UsePrefs_Cancel:
            SetSuperAttrs(CLASS,self,MUIA_Window_Open,FALSE,TAG_DONE);
            if (data->flags & FLG_Test) SetAttrs(_app(self),MPA_Prefs,(ULONG)&data->prefs,MPA_OneWay,TRUE,MPA_NoIfList,TRUE,TAG_DONE);
            break;
    }

    if (close)
        DoMethod(_app(self),MUIM_Application_PushMethod,(ULONG)_app(self),2,MPM_DisposeWin,(ULONG)self);

    return 0;
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_Prefs_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:             return MUIPC_Prefs__OM_NEW(CLASS,self,(APTR)message);
        case OM_SET:             return MUIPC_Prefs__OM_SET(CLASS,self,(APTR)message);
        case MPM_Prefs_UsePrefs: return MUIPC_Prefs__MPM_Prefs_UsePrefs(CLASS,self,(APTR)message);
        default:                 return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_Prefs_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_prefsClass = MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct MiamiPanelPrefsClass_DATA),MUIPC_Prefs_Dispatcher))
    {
        localizeArray(registerLabels,registerLabelsIDs);
        localizeArray(tformats,tformatsIDs);
        localizeArray(rformats,rformatsIDs);
        localizeArray(positions,positionsIDs);
        localizeArray(vmodes,vmodesIDs);
        localizeArray(lposs,lpossIDs);
        localizeArray(oniconifies,oniconifiesIDs);
        localizeArray(sblayouts,sblayoutsIDs);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
