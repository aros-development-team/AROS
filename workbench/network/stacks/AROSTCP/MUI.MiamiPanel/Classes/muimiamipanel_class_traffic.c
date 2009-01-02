
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

static UBYTE *Kbs, *Mbs;

struct MiamiPanelTrafficClass_DATA
{
    UQUAD traffic;
    ULONG flags;
};

enum
{
    FLG_Short      = 1<<0,
    FLG_NoGrouping = 1<<1,
};

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_Traffic__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct TagItem *attrs = message->ops_AttrList;

    if (self = (Object *)DoSuperNewTags(CLASS, self, NULL, MUIA_Text_Contents, "0", TAG_MORE, attrs))
    {
        message->MethodID = OM_SET;
        DoMethodA(self,(Msg)message);
        message->MethodID = OM_NEW;
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_Traffic__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct MiamiPanelTrafficClass_DATA    *data = INST_DATA(CLASS,self);
    register struct TagItem *tag;
    struct TagItem          *tstate;
    register ULONG          redraw;

    for (redraw = FALSE, tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Prefs:
                if (!BOOLSAME(PREFS(tidata)->flags & MPV_Flags_TrafficShort,data->flags & FLG_Short) ||
                    !BOOLSAME(PREFS(tidata)->flags & MPV_Flags_TrafficNoGrouping,data->flags & FLG_NoGrouping))
                {
                    if (PREFS(tidata)->flags & MPV_Flags_TrafficShort) data->flags |= FLG_Short;
                    else data->flags &= ~FLG_Short;

                    if (PREFS(tidata)->flags & MPV_Flags_TrafficNoGrouping) data->flags |= FLG_NoGrouping;
                    else data->flags &= ~FLG_NoGrouping;

                    redraw = TRUE;
                }
                break;

            case MPA_Value:
            {
                bigint *bi = (bigint *)tidata;
                UQUAD  traffic;

                traffic = ((UQUAD)bi->hi << 32) | bi->lo;
                
                if (data->traffic!=traffic)
                {
                    data->traffic = traffic;
                    redraw = TRUE;
                }
            }
        }
    }

    if (redraw)
    {
        UBYTE cont[64];
        UQUAD tot = data->traffic;

        if (data->flags & FLG_Short)
        {
            UBYTE *dp = MiamiPanelBaseIntern->mpb_decPoint;

            if (tot>>32) /* big int */
            {
                UBYTE buf1[64], buf2[64];
                UQUAD q, r, qd, rd;

                q = tot/1000000ULL;
                r = tot%1000000ULL;
                sprintf(buf1,"%Ld",q);
                if (!(data->flags & FLG_NoGrouping)) grouping(buf1, MiamiPanelBaseIntern);

                qd = r/1000ULL;
                rd = r%1000ULL;
                sprintf(buf1,"%Ld",qd);

                sprintf(cont,"%s%s%3.3s %s",buf1,dp,buf2,Mbs);
            }
            else /* small int */
            {
                UBYTE buf[32];
                ULONG q, r;

                if (tot>=1000000) /* Mb */
                {
                    q = tot/1000000;
                    r = tot%1000000;
					sprintf(buf, "%u", q);
                    r = r/1000;
                    if (!(data->flags & FLG_NoGrouping)) grouping(buf, MiamiPanelBaseIntern);
                    sprintf(cont,"%s%s%3.3lu %s",buf,dp,r,Mbs);
                }
                else /* Kb */
                {
                    q = tot/1000;
                    r = tot%1000;
					sprintf(buf, "%u", q);
                    if (!(data->flags & FLG_NoGrouping)) grouping(buf, MiamiPanelBaseIntern);
                    sprintf(cont,"%s%s%1.1lu %s",buf,dp,(r<100) ? 0 : r,Kbs);
                }
            }
        }
        else
        {
            sprintf(cont,"%Ld",tot);
            
            if (!(data->flags & FLG_NoGrouping)) grouping(cont, MiamiPanelBaseIntern);
        }

		SetSuperAttrs(CLASS, self, MUIA_Text_Contents, cont, TAG_DONE);
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_Traffic_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_SET: return MUIPC_Traffic__OM_SET(CLASS,self,(APTR)message);
        case OM_NEW: return MUIPC_Traffic__OM_NEW(CLASS,self,(APTR)message);
        default:     return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_Traffic_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_trafficClass = MUI_CreateCustomClass(NULL, MUIC_Text, NULL, sizeof(struct MiamiPanelTrafficClass_DATA), MUIPC_Traffic_Dispatcher))
    {
        Kbs = __(MSG_Traffic_KB);
        Mbs = __(MSG_Traffic_MB);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
