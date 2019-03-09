/*
    Copyright (C) 2018-2019, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <mui/NFloattext_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include "locale.h"

#include "storage_classes.h"
#include "storage_intern.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if (1) // TODO : Move into libbase
extern OOP_AttrBase HiddStorageUnitAB;
#endif

/*** Instance Data **********************************************************/
struct StorageUnitWindow_DATA
{
    /* Nothing to add */
};

static const char *const unitTypeNames[] =
{
    "Unknown" ,
    "Raid Array" ,
    "Fixed Disk" ,
    "Solid State Disk" ,
    "Optical Disc" ,
    "Magnetic Tape" ,
    NULL
};

static Object *StorageUnitWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    IPTR model, revision, serial;
    IPTR unitdev, val;
    char unit_str[32];
    const char *type;
    LONG removable;

    if (!dev)
        return NULL;

    /* Generic Storage attributes ... */
    OOP_GetAttr(dev, aHidd_StorageUnit_Type, &val);
    if (val < 6)
        type = unitTypeNames[val];
    else
        type = unitTypeNames[0];

    OOP_GetAttr(dev, aHidd_StorageUnit_Model   , &model);
    OOP_GetAttr(dev, aHidd_StorageUnit_Revision, &revision);
    OOP_GetAttr(dev, aHidd_StorageUnit_Serial  , &serial);

    removable = OOP_GET(dev, aHidd_StorageUnit_Removable) ? IDS_SELECTED : IDS_NORMAL;

    OOP_GetAttr(dev, aHidd_StorageUnit_Device, &unitdev);
    OOP_GetAttr(dev, aHidd_StorageUnit_Number, &val);
    snprintf(unit_str, sizeof(unit_str), "%s/%ld", (char *)unitdev, val);

    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, "Unit Properties",
        MUIA_Window_ID, MAKE_ID('S', 'U', 'N', 'P'),
        WindowContents, (IPTR)MUI_NewObject(MUIC_Group,
            MUIA_Group_SameSize, TRUE,
            Child, (IPTR)(ColGroup(2),
                MUIA_Group_SameSize, TRUE,
                MUIA_FrameTitle, __(MSG_GENERAL),
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label("Unit"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)unit_str,
                End),
                Child, (IPTR)Label("Type"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)type,
                End),
                Child, (IPTR)Label(_(MSG_MODEL)),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, model,
                End),
                Child, (IPTR)Label(_(MSG_REVISION)),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, revision,
                End),
                Child, (IPTR)Label(_(MSG_SERIAL)),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, serial,
                End),
                Child, (IPTR)Label(_(MSG_REMOVABLE)),
                Child, (IPTR)(HGroup,
                    Child, (IPTR)(ImageObject,
                        MUIA_Image_Spec, MUII_CheckMark,
                        MUIA_Image_State, removable,
                        TextFrame,
                        MUIA_CycleChain, 1,
                        MUIA_Background, MUII_TextBack,
                    End),
                    Child, (IPTR)HSpace(0),
                End),
            End),
            TAG_MORE, (IPTR) msg->ops_AttrList,
        TAG_DONE),
        TAG_DONE
    );
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    StorageUnitWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
