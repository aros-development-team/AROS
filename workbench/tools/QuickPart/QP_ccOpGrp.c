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

#define _QP_CCOPGRP_C

#include "QP_ccOpGrp.h"

/* QuickPart Text Custom Class */

#define SETUP_INST_DATA struct QPOpGrp_DATA *data = INST_DATA(CLASS, self)

struct MUIP_StructWithObj
{
    STACKED ULONG MethodID;
    STACKED Object *Obj;
};

/** Standard Class Methods **/
static IPTR QPOpGrp__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    Object *spacerObj;
    D(bug("[QuickPart:OpGrp] %s: Creating Operations Group Object\n", __func__));

    spacerObj = RectangleObject,
            End;

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,
            MUIA_FixHeight, 60,
            MUIA_Group_Horiz, TRUE,
            TAG_MORE, (IPTR)message->ops_AttrList   
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        D(bug("[QuickPart:OpGrp] %s: Operations Group Object @ 0x%p, Data @ 0x%p\n", __func__, self, data));

        data->qpogd_Spacer = spacerObj;

        struct MUIP_StructWithObj supermsg;
        supermsg.MethodID = OM_ADDMEMBER;
        supermsg.Obj = data->qpogd_Spacer;
        DoSuperMethodA(CLASS, self, (Msg)&supermsg);
    }

    D(bug("[QuickPart:OpGrp] %s: returning 0x%p\n", __func__, self));

    return (IPTR)self;
}

static IPTR QPOpGrp__OM_ADDMEMBER
(
    Class *CLASS, Object *self, struct MUIP_StructWithObj *message
)
{
    SETUP_INST_DATA;

    IPTR retval;
    struct MUIP_StructWithObj supermsg;

    D(bug("[QuickPart:OpGrp] %s(0x%p) <Class 0x%p>\n", __func__, message->Obj, (message->Obj) ? OCLASS(message->Obj) : NULL));
    if (!message->Obj)
        return (IPTR)NULL;
#if (0)
    if (OCLASS(message->Obj) != mcc_qpop->mcc_Class)
        return (IPTR)NULL;
#endif

    supermsg.Obj = data->qpogd_Spacer;
    supermsg.MethodID = OM_REMMEMBER;
    DoSuperMethodA(CLASS, self, (Msg)&supermsg);

    if (data->qpogd_OpCnt > 0)
    {
        supermsg.MethodID = OM_ADDMEMBER;
        supermsg.Obj = VGroup,
                Child, HVSpace,
                Child, ImageObject,
                    MUIA_Image_Spec, (IPTR)"3:THEME:Images/Gadgets/Next",
                End,
                Child, HVSpace,
            End;
        DoSuperMethodA(CLASS, self, (Msg)&supermsg);
        SET(message->Obj, MUIA_UserData, supermsg.Obj);
    }

    retval = DoSuperMethodA(CLASS, self, (Msg)message);

    supermsg.MethodID = OM_ADDMEMBER;
    supermsg.Obj = data->qpogd_Spacer;
    DoSuperMethodA(CLASS, self, (Msg)&supermsg);

    data->qpogd_OpCnt += 1;

    return retval;
}

static IPTR QPOpGrp__OM_REMMEMBER
(
    Class *CLASS, Object *self, struct MUIP_StructWithObj *message
)
{
    SETUP_INST_DATA;

    IPTR retval;
    struct MUIP_StructWithObj supermsg;

    D(bug("[QuickPart:OpGrp] %s(0x%p) <Class 0x%p>\n", __func__, message->Obj, (message->Obj) ? OCLASS(message->Obj) : NULL));
    if (!message->Obj)
        return (IPTR)NULL;
#if (0)
    if (OCLASS(message->Obj) != mcc_qpop->mcc_Class)
        return (IPTR)NULL;
#endif
    GET(message->Obj, MUIA_UserData, &supermsg.Obj);
    retval = DoSuperMethodA(CLASS, self, (Msg)message);
    if (supermsg.Obj)
    {
        supermsg.MethodID = OM_REMMEMBER;
        DoSuperMethodA(CLASS, self, (Msg)&supermsg);
    }
    data->qpogd_OpCnt -= 1;

    return retval;
}

BOOPSI_DISPATCHER(IPTR, QPOpGrp_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPOpGrp__OM_NEW(CLASS, self, (struct opSet *) message);
    case OM_ADDMEMBER:
        return QPOpGrp__OM_ADDMEMBER(CLASS, self, (struct MUIP_StructWithObj *) message);
    case OM_REMMEMBER:
        return QPOpGrp__OM_REMMEMBER(CLASS, self, (struct MUIP_StructWithObj *) message);
    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
