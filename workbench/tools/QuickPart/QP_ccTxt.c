/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#define _QP_CCTXT_C

#include "QP_ccTxt.h"

/* QuickPart Text Custom Class */

#define SETUP_INST_DATA struct QPTxt_DATA *data = INST_DATA(CLASS, self)

/** Standard Class Methods **/
static IPTR QPTxt__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    D(bug("[QuickPart:Txt] Creating Text Object\n"));

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,
            TAG_MORE, (IPTR)message->ops_AttrList   
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        D(bug("[QuickPart:Txt] Text Object @ 0x%p, Data @ 0x%p\n", self, data));
    }

    return (IPTR)self;
}

static IPTR QPTxt__OM_GET
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

static IPTR QPTxt__MUIM_AskMinMax
(
    Class *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{

    DoSuperMethodA(CLASS, self, (Msg) message);
    if (message->MinMaxInfo->MinWidth < 10)
        message->MinMaxInfo->MinWidth = 10;

    return TRUE;
}

BOOPSI_DISPATCHER(IPTR, QPTxt_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPTxt__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_GET:
        return QPTxt__OM_GET(CLASS, self,  (struct opGet *) message);
    case MUIM_AskMinMax:
        return QPTxt__MUIM_AskMinMax(CLASS, self,  (struct MUIP_AskMinMax *) message);

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
