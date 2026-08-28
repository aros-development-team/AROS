/*
    Copyright © 2026, The AROS Development Team. All rights reserved.

    PopInterface custom class - see popinterface.h.
*/

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/alib.h>

/* MUIC_Popobject / MUIA_Popstring_* / MUIM_Popstring_* come from <libraries/mui.h> */

#include "locale.h"
#include "popinterface.h"

/*** Instance data **********************************************************/
struct PopInterface_DATA
{
    LONG    pif_Chosen;
    Object *pif_AddIfButton;
    Object *pif_AddTunnelButton;
};

/*** OM_NEW *****************************************************************/
Object *PopInterface__OM_NEW(Class *CLASS, Object *self,
    struct opSet *message)
{
    Object *addIfButton, *popButton, *addTunnelButton;

    addIfButton     = SimpleButton(_(MSG_BUTTON_ADD_INTERFACE));
    popButton       = PopButton(MUII_PopUp);
    addTunnelButton = SimpleButton(_(MSG_BUTTON_ADD_TUNNEL));

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Popstring_String, (IPTR)addIfButton,
        MUIA_Popstring_Button, (IPTR)popButton,
        MUIA_Popobject_Object, (IPTR)(VGroup,
            Child, (IPTR)addTunnelButton,
        End),
        TAG_MORE, (IPTR)message->ops_AttrList
    );

    if (self != NULL)
    {
        struct PopInterface_DATA *data = INST_DATA(CLASS, self);

        data->pif_Chosen         = MUIV_PopInterface_None;
        data->pif_AddIfButton     = addIfButton;
        data->pif_AddTunnelButton = addTunnelButton;

        /* Main button -> request a normal interface.  (The pop button is wired
           by the Popstring superclass to open the popup, so it does NOT report
           a choice.) */
        DoMethod
        (
            addIfButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 3, MUIM_Set, MUIA_PopInterface_Chosen,
            MUIV_PopInterface_DeviceInterface
        );

        /* Popped-down button -> request a tunnel, then close the popup. */
        DoMethod
        (
            addTunnelButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 3, MUIM_Set, MUIA_PopInterface_Chosen,
            MUIV_PopInterface_TunnelInterface
        );
        DoMethod
        (
            addTunnelButton, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)self, 2, MUIM_Popstring_Close, FALSE
        );
    }

    return self;
}

/*** OM_SET *****************************************************************/
IPTR PopInterface__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct PopInterface_DATA *data = INST_DATA(CLASS, self);
    const struct TagItem *tags = message->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PopInterface_Chosen:
                data->pif_Chosen = (LONG)tag->ti_Data;
                break;
        }
    }

    /* Pass through so the Notify superclass fires MUIA_PopInterface_Chosen
       notifications (the attribute is not consumed here). */
    return DoSuperMethodA(CLASS, self, (Msg)message);
}

/*** OM_GET *****************************************************************/
IPTR PopInterface__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    struct PopInterface_DATA *data = INST_DATA(CLASS, self);

    switch (message->opg_AttrID)
    {
        case MUIA_PopInterface_Chosen:
            *message->opg_Storage = (IPTR)data->pif_Chosen;
            return TRUE;
    }

    return DoSuperMethodA(CLASS, self, (Msg)message);
}

/*** Class boilerplate ******************************************************/
ZUNE_CUSTOMCLASS_3
(
    PopInterface, NULL, MUIC_Popobject, NULL,
    OM_NEW,  struct opSet *,
    OM_SET,  struct opSet *,
    OM_GET,  struct opGet *
);
