#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

struct MUI_BoopsiData
{
    ULONG *remember;
    LONG remember_len;

    struct IClass *boopsi_class;
    char *boopsi_classid;
    int boopsi_minwidth,boopsi_minheight;
    int boopsi_maxwidth,boopsi_maxheight;
    Object *boopsi_object;
    ULONG boopsi_tagdrawinfo;
    ULONG boopsi_tagscreen;
    ULONG boopsi_tagwindow;
    ULONG boopsi_smart;

    struct TagItem *boopsi_taglist;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Boopsi_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BoopsiData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    data->boopsi_taglist = CloneTagItems(msg->ops_AttrList);
    data->boopsi_maxwidth = data->boopsi_maxheight = MUI_MAXMAX;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Boopsi_Class:
		    data->boopsi_class = (struct IClass*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_ClassID:
		    data->boopsi_classid = (char*)tag->ti_Data;
	    	    break;

	    case    MUIA_Boopsi_MaxHeight:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MaxWidth:
		    data->boopsi_maxwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinHeight:
		    data->boopsi_minheight = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_MinWidth:
		    data->boopsi_minwidth = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_Object:
		    data->boopsi_object = (Object*)tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_Remember:
		    {
			ULONG *new_remember = AllocVec(sizeof(ULONG)*(data->remember_len+1),MEMF_CLEAR);
			if (new_remember)
			{
			    if (data->remember) CopyMem(data->remember,new_remember,sizeof(ULONG)*data->remember_len);
			    new_remember[data->remember_len] = tag->ti_Data;
			    FreeVec(data->remember);
			    data->remember = new_remember;
			}
		    }
		    break;

	    case    MUIA_Boopsi_Smart:
		    data->boopsi_smart = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagDrawInfo:
		    data->boopsi_tagdrawinfo = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagScreen:
		    data->boopsi_tagscreen = tag->ti_Data;
		    break;

	    case    MUIA_Boopsi_TagWindow:
		    data->boopsi_tagwindow = tag->ti_Data;
		    break;
 
	}
    }
    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Boopsi_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BoopsiData *data = INST_DATA(cl, obj);
    if (data->boopsi_taglist) FreeTagItems(data->boopsi_taglist);
    return 0;
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Boopsi_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_BoopsiData *data = INST_DATA(cl, obj);

    /*
    ** let our superclass first fill in what it thinks about sizes.
    ** this will e.g. add the size of frame and inner spacing.
    */
    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth +=  data->boopsi_minwidth;
    msg->MinMaxInfo->MinHeight += data->boopsi_minheight;
    msg->MinMaxInfo->DefWidth +=  data->boopsi_minwidth;
    msg->MinMaxInfo->DefHeight +=  data->boopsi_minheight;
    msg->MinMaxInfo->MaxWidth  += data->boopsi_maxwidth;
    msg->MinMaxInfo->MaxHeight += data->boopsi_maxheight;
    return TRUE;
}


#ifndef _AROS
__asm IPTR Boopsi_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Boopsi_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Boopsi_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Boopsi_Dispose(cl, obj, msg);
//	case OM_GET: return Boopsi_Get(cl, obj, (struct opGet *)msg);
//	case MUIM_Setup: return Boopsi_Setup(cl, obj, (APTR)msg);
//	case MUIM_Cleanup: return Boopsi_Cleanup(cl, obj, (APTR)msg);
	case MUIM_AskMinMax: return Boopsi_AskMinMax(cl, obj, (APTR)msg);
//	case MUIM_Draw: return Boopsi_Draw(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Boopsi_desc = { 
    MUIC_Boopsi, 
    MUIC_Area, 
    sizeof(struct MUI_BoopsiData), 
    Boopsi_Dispatcher 
};
