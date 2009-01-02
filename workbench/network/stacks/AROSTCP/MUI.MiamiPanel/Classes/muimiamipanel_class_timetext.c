
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include <stdio.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"

/***********************************************************************/

struct MiamiPanelTimeTextClass_DATA
{
    ULONG secs;
};

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_TimeText__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct TagItem *attrs = message->ops_AttrList;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Text_Contents, "00:00:00",
            TAG_MORE,attrs))
    {
        register struct MiamiPanelTimeTextClass_DATA *data = INST_DATA(CLASS,self);

        data->secs = GetTagData(MPA_Value,0,attrs);
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_TimeText__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    register struct MiamiPanelTimeTextClass_DATA    *data = INST_DATA(CLASS,self);
    register struct TagItem *tag;
    struct TagItem          *tstate;

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_Value:
                if (tidata!=data->secs)
                {
                    UBYTE buf[64];

                    if (tidata>0)
                    {
                        register ULONG d, h, m;
                        ULONG          s = tidata;

                        d = s/86400;
                        s = s%86400;
                        h = s/3600;
                        s = s%3600;
                        m = s/60;
                        s = s%60;

                        if (d>0) sprintf(buf,"%lu %02lu:%02lu:%02lu",d,h,m,s);
                        else sprintf(buf,"%02lu:%02lu:%02lu",h,m,s);
                    }
                    else strcpy(buf,"00:00:00");

                    data->secs = tidata;
					SetSuperAttrs(CLASS, self, MUIA_Text_Contents, buf, TAG_DONE);
                }
                break;
        }
    }

    return DoSuperMethodA(CLASS,self,(APTR)message);
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_TimeText_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_SET: return MUIPC_TimeText__OM_SET(CLASS,self,(APTR)message);
        case OM_NEW: return MUIPC_TimeText__OM_NEW(CLASS,self,(APTR)message);
        default:     return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_TimeText_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    return (ULONG)(MiamiPanelBaseIntern->mpb_timeTextClass = MUI_CreateCustomClass(NULL, MUIC_Text, NULL, sizeof(struct MiamiPanelTimeTextClass_DATA), MUIPC_TimeText_Dispatcher));
}

/***********************************************************************/
