/*
    Copyright (C) 2013-2019, The AROS Development Team.
    $Id: ata_window_cl.c 51417 2016-01-25 18:10:16Z NicJA $
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/sysexp.h>

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
extern OOP_AttrBase HiddBusAB;
#endif

/*** Instance Data **********************************************************/
struct StorageBusWindow_DATA
{
    /* Nothing to add */
};

static Object *StorageBusWindow__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    struct SysexpStorageBase *StorageBase = (struct SysexpStorageBase *)cl->cl_UserData;
    OOP_Object *dev = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    char maxbusunits_str[4];
    IPTR val;

    D(bug("[storage.sysexp] %s: cl @ %p\n", __func__, cl));

    if ((!dev))
        return NULL;

    /* Generic Storage Bus attributes ... */
    OOP_GetAttr(dev, aHidd_Bus_MaxUnits, &val);
    snprintf(maxbusunits_str, sizeof(maxbusunits_str), "%ld", val);

    return (Object *) DoSuperNewTags
    (
        cl, self, NULL,
        MUIA_Window_Title, (IPTR)"Bus Properties",
        MUIA_Window_ID, MAKE_ID('S', 'B', 'S', 'P'),
        WindowContents, (IPTR)(VGroup,
            Child, (IPTR)(BOOPSIOBJMACRO_START(StorageBase->sesb_DevicePageCLASS->mcc_Class),
                MUIA_PropertyWin_Object, (IPTR)dev,
            End),
            Child, (IPTR)(ColGroup(2),
                MUIA_Group_SameSize, TRUE,
                MUIA_FrameTitle, (IPTR)"Bus",
                GroupFrame,
                MUIA_Background, MUII_GroupBack,
                Child, (IPTR)Label("Max Units"),
                Child, (IPTR)(TextObject,
                    TextFrame,
                    MUIA_Background, MUII_TextBack,
                    MUIA_CycleChain, 1,
                    MUIA_Text_Contents, (IPTR)maxbusunits_str,
                End),
            End),
            TAG_MORE, (IPTR)msg->ops_AttrList,
        End),
        TAG_DONE
    );
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    StorageBusWindow, NULL, MUIC_Window, NULL,
    OM_NEW, struct opSet *
);
