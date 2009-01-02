
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include <stdio.h>
#include <string.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"

/***********************************************************************/

static UBYTE *bpss, *Kbss, *Mbss;

struct MiamiPanelRateClass_DATA
{
    ULONG rate;
    ULONG flags;
    UBYTE cont[36];
};

enum
{
    FLG_Short      = 1<<0,
    FLG_NoGrouping = 1<<1,
};

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_Rate__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct TagItem *attrs = message->ops_AttrList;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Frame,          MUIV_Frame_Gauge,
            MUIA_Gauge_Horiz,    TRUE,
            MUIA_Gauge_InfoText, "0",
            TAG_MORE, attrs))
    {
        message->MethodID = OM_SET;
        DoMethodA(self,(Msg)message);
        message->MethodID = OM_NEW;
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_Rate__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct MiamiPanelRateClass_DATA    *data = INST_DATA(CLASS,self);
    register struct TagItem *tag;
    struct TagItem          *tstate;
    register ULONG          redraw;

    for (redraw = FALSE, tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Prefs:
                if (!BOOLSAME(PREFS(tidata)->flags & MPV_Flags_RateShort,data->flags & FLG_Short) ||
                    !BOOLSAME(PREFS(tidata)->flags & MPV_Flags_RateNoGrouping,data->flags & FLG_NoGrouping))
                {
                    if (PREFS(tidata)->flags & MPV_Flags_RateShort) data->flags |= FLG_Short;
                    else data->flags &= ~FLG_Short;

                    if (PREFS(tidata)->flags & MPV_Flags_RateNoGrouping) data->flags |= FLG_NoGrouping;
                    else data->flags &= ~FLG_NoGrouping;

                    redraw = TRUE;
                }
                break;

            case MPA_Value:
                if (data->rate!=tidata)
                {
                    data->rate = tidata;
                    redraw = TRUE;
                }
                SetSuperAttrs(CLASS, self, MUIA_Gauge_Current, tidata, TAG_DONE);
                break;
        }
    }

    if (redraw)
    {
        UBYTE          cont[36];
        register ULONG rate = data->rate;

        if (data->flags & FLG_Short)
        {
            UBYTE buf[16], *dp = MiamiPanelBaseIntern->mpb_decPoint;
            ULONG q, r;

            if (rate>=1000000)
            {
                q = rate/1000000;
                r = rate%1000000;
				sprintf(buf, "%u", q);
                r = r/1000;
                if (!(data->flags & FLG_NoGrouping)) grouping(buf, MiamiPanelBaseIntern);
                sprintf(cont,"%s%s%3.3lu %s",buf,dp,r,Mbss);
            }
            else
            {
                q = rate/1000;
                r = rate%1000;
				sprintf(buf, "%u", q);
                if (!(data->flags & FLG_NoGrouping)) grouping(buf, MiamiPanelBaseIntern);
                sprintf(cont,"%s%s%1.1lu %s",buf,dp,(r<100) ? 0 : r,Kbss);
            }
        }
        else
        {
			sprintf(cont, "%u", rate);
            if (!(data->flags & FLG_NoGrouping)) grouping(cont, MiamiPanelBaseIntern);
            strcat(cont," ");
            strcat(cont,bpss);
        }

        sprintf(data->cont,"\33l%s",cont);
		SetSuperAttrs(CLASS, self, MUIA_Gauge_InfoText, data->cont,TAG_DONE);
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_Rate_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_SET: return MUIPC_Rate__OM_SET(CLASS,self,(APTR)message);
        case OM_NEW: return MUIPC_Rate__OM_NEW(CLASS,self,(APTR)message);
        default:     return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_Rate_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_rateClass = MUI_CreateCustomClass(NULL,MUIC_Gauge,NULL,sizeof(struct MiamiPanelRateClass_DATA), MUIPC_Rate_Dispatcher))
    {
        bpss = __(MSG_Rate_Bs);
        Kbss = __(MSG_Rate_KBs);
        Mbss = __(MSG_Rate_MBs);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
