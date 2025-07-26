/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#define INTUITION_NO_INLINE_STDARG
#include	<aros/debug.h>

#include	<proto/expansion.h>
#include	<proto/exec.h>
#include	<proto/partition.h>
#include	<proto/dos.h>
#include	<proto/intuition.h>
#include	<proto/muimaster.h>
#include	<proto/locale.h>
#include	<proto/utility.h>

#include   <proto/alib.h>

#include	<libraries/configvars.h>
#include	<libraries/expansionbase.h>
#include	<libraries/partition.h>
#include	<libraries/mui.h>

#include	<devices/trackdisk.h>
#include	<devices/scsidisk.h>

#include	<dos/dos.h>
#include	<dos/filehandler.h>

#include <utility/tagitem.h>

#include	<exec/memory.h>
#include	<exec/execbase.h>
#include	<exec/lists.h>
#include	<exec/nodes.h>
#include	<exec/types.h>
#include	<exec/ports.h>

#include	<zune/systemprefswindow.h>
#include	<zune/prefseditor.h>
#include	<zune/aboutwindow.h>

#include	<stdlib.h>
#include	<stdio.h>
#include	<strings.h>

#include <aros/locale.h>

#include "QP_Intern.h"

#define _QP_CCOPERATION_C

#include "QP_ccOperation.h"

/* QuickPart Text Custom Class */

#define SETUP_INST_DATA struct QPOp_DATA *data = INST_DATA(CLASS, self)

/** Standard Class Methods **/
static IPTR QPOp__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct      TagItem             *tags = NULL,
                                    *tag = NULL;
	Object *opTarget = NULL, *tmpObj, *labelObj;
	char *opStr = NULL;
	IPTR opType = 0;

    D(bug("[QuickPart:Op] Creating Operation Object\n"));
    
    tags = message->ops_AttrList;
    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
		case MUIA_QPart_ccOperation_Type:
			{
				opType = tag->ti_Data;
				switch (opType)
				{
					case MUIV_QPart_ccOperation_Delete:
						opStr = "Delete Partition";
						break;
					case MUIV_QPart_ccOperation_Create:
						opStr = "Create Partition";
						break;
				}
			}
			break;

		case MUIA_QPart_ccOperation_Target:
			opTarget = (Object *)tag->ti_Data;
			break;

        default:
            continue; /* Don't supress non-processed tags */
        }
        tag->ti_Tag = TAG_IGNORE;
    }

	if (!opTarget || !opType)
		return (IPTR)NULL;

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,
			MUIA_FixHeight, 60,
			MUIA_FixWidth, 50,
			Child, (IPTR)(tmpObj = HVSpace),
			Child, (IPTR)(labelObj = TextObject,
				MUIA_Font, MUIV_Font_Tiny,
				MUIA_Text_SetMin, FALSE,
				MUIA_Text_SetMax, FALSE,
				MUIA_Text_Contents, (IPTR)opStr,
				MUIA_Text_PreParse, "\33c",
			End),
            TAG_MORE, (IPTR)message->ops_AttrList   
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

		data->qpod_LabelObj = labelObj;
		data->qpod_TmpObj = tmpObj; 
		data->qpod_TargetObj = opTarget;
		data->qpod_Operation = opType;
        D(bug("[QuickPart:Op] Operation Object @ 0x%p <Class @ 0x%p, Data @ 0x%p>\n", self, OCLASS(self), data));
    }

    return (IPTR)self;
}

static IPTR QPOp__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    SETUP_INST_DATA;
    IPTR                          *store = message->opg_Storage;
    IPTR    	      		 retval = TRUE;

    switch(message->opg_AttrID)
    {
    default:
        retval = DoSuperMethodA(CLASS, self, (Msg)message);
        break;
    }

    return retval;
}

static IPTR QPOp__MUIM_AskMinMax
(
    Class *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{

    DoSuperMethodA(CLASS, self, (Msg) message);
//    if (message->MinMaxInfo->MinWidth < 10)
//        message->MinMaxInfo->MinWidth = 10;

    return TRUE;
}

static IPTR QPOp__MUIM_Setup(Class * CLASS, Object * self, struct MUIP_Setup *message)
{
    SETUP_INST_DATA;
    IPTR retval;

    D(bug("[QuickPart:Op] %s()\n", __func__));

    retval = DoSuperMethodA(CLASS, self, message);

    if (data->qpod_TmpObj)
	{
		if (data->qpod_Operation == MUIV_QPart_ccOperation_Delete)
		{
			if (DoMethod(self, MUIM_Group_InitChange))
			{
				DoMethod(self, OM_REMMEMBER, data->qpod_LabelObj);
				DoMethod(self, OM_ADDMEMBER, data->qpod_TargetObj);
				DoMethod(self, OM_REMMEMBER, data->qpod_TmpObj);
				DoMethod(self, OM_ADDMEMBER, data->qpod_LabelObj);
				DoMethod(self, MUIM_Group_ExitChange);
				MUI_DisposeObject(data->qpod_TmpObj);
				data->qpod_TmpObj = NULL;
			}
		}
	}

    return retval;
}

BOOPSI_DISPATCHER(IPTR, QPOp_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPOp__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_GET:
        return QPOp__OM_GET(CLASS, self,  (struct opGet *) message);
    case MUIM_AskMinMax:
        return QPOp__MUIM_AskMinMax(CLASS, self,  (struct MUIP_AskMinMax *) message);
	case MUIM_Setup:
		return QPOp__MUIM_Setup(CLASS, self, (struct MUIP_Setup *)message);

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
