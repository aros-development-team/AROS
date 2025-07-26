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

#define _QP_CCFREE_H

#include "QP_ccFree.h"
#include "QP_ccTxt.h"
#include "QP_PartionColors.h"

/* QuickPart Text Custom Class */

#define SETUP_INST_DATA struct QPFree_DATA *data = INST_DATA(CLASS, self)
extern struct   MUI_CustomClass      *mcc_qptxt;

CONST_STRPTR str_unused = "Unused";

/** Standard Class Methods **/
static IPTR QPFree__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct TagItem *udTag = FindTagItem(MUIA_UserData, message->ops_AttrList);
    char *fsLabel = NULL;

    // MUIA_UserData
    D(bug("[QuickPart:FreeS] Creating FreeSpace Object\n"));
    if (udTag)
    {
        struct PartCont_ChildIntern *freeSpaceNode = (struct PartCont_ChildIntern *)udTag->ti_Data;
        fsLabel = (char *)str_unused;
    }
    else
        fsLabel = (char *)str_unused;

    self = (Object *)DoSuperNewTags
         (
            CLASS, self, NULL,
                MUIA_Background, (IPTR)DEF_PART_UNUSED,
                MUIA_InnerLeft, 1,
                MUIA_InnerTop, 0,
                MUIA_InnerRight, 1,
                MUIA_InnerBottom, 1,
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_CycleChain, 1,
                MUIA_Group_SameWidth, FALSE,
                Child, HGroup,
                    MUIA_FixHeight, 10,
                    MUIA_InnerLeft, 1,
                    MUIA_InnerTop, 1,
                    Child, (IPTR)(NewObject(mcc_qptxt->mcc_Class, NULL,
                        MUIA_Font, MUIV_Font_Tiny,
                        MUIA_Text_SetMin, FALSE,
                        MUIA_Text_SetMax, FALSE,
                        MUIA_Text_Contents, (IPTR)fsLabel,
                    TAG_DONE)),
                    Child, RectangleObject,
                    End,
                End,
                Child, RectangleObject,
                End,
            TAG_MORE, (IPTR)message->ops_AttrList
         );

    if (self != NULL)
    {
        /* Initial local instance data */
        SETUP_INST_DATA;

        D(bug("[QuickPart:FreeS] FreeSpace Object @ 0x%p, Data @ 0x%p\n", self, data));
    }

    return (IPTR)self;
}

static IPTR QPFree__MUIM_AskMinMax
(
    Class *CLASS, Object *self, struct MUIP_AskMinMax *message
)
{
    DoSuperMethodA(CLASS, self, (Msg) message);
    if (message->MinMaxInfo->MinWidth < 3)
        message->MinMaxInfo->MinWidth = 3;

    return TRUE;
}

BOOPSI_DISPATCHER(IPTR, QPFree_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
    case OM_NEW: 
        return QPFree__OM_NEW(CLASS, self, (struct opSet *) message);

    case MUIM_AskMinMax:
        return QPFree__MUIM_AskMinMax(CLASS, self,  (struct MUIP_AskMinMax *) message);

    default:     
        return DoSuperMethodA(CLASS, self, message);
    }

    return 0;
}
BOOPSI_DISPATCHER_END
