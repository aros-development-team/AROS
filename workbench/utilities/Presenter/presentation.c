/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <graphics/text.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>

#include "presentation.h"

/*** Instance data **********************************************************/
struct Presentation_DATA
{
    ULONG                        ssd_ChildCount;
    struct MUI_EventHandlerNode  ssd_EHN;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct Presentation_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *Presentation__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    self = (Object *) DoSuperNewTags
    ( 
        CLASS, self, NULL,
        
        InnerSpacing(4, 4),
        
        /* Add content here ... */
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );

    return self;
}

IPTR Presentation__OM_ADDMEMBER
(
    Class *CLASS, Object *self, struct opMember *message
)
{
    IPTR success = DoSuperMethodA(CLASS, self, (Msg) message);
    
    if (success)
    {
        SETUP_INST_DATA;
        data->ssd_ChildCount++;
    }
    
    return success;
}

IPTR Presentation__OM_REMMEMBER
(
    Class *CLASS, Object *self, struct opMember *message
)
{
    IPTR success = DoSuperMethodA(CLASS, self, (Msg) message);
    
    if (success)
    {
        SETUP_INST_DATA;
        data->ssd_ChildCount--;
    }
    
    return success;
}

IPTR Presentation__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    const struct TagItem *tstate = message->ops_AttrList;
    struct TagItem       *tag;
    
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Group_ActivePage:
                switch (tag->ti_Data)
                {
                    case MUIV_Group_ActivePage_Next:
                        if
                        (
                               XGET(self, MUIA_Group_ActivePage) 
                            >= data->ssd_ChildCount - 1
                        )
                        {
                            tag->ti_Tag = TAG_IGNORE;
                        }
                        break;
                    
                    case MUIV_Group_ActivePage_Prev:
                        if (XGET(self, MUIA_Group_ActivePage) <= 0)
                        {
                            tag->ti_Tag = TAG_IGNORE;
                        }
                        break;
                }
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Presentation__MUIM_Setup
(
    Class *CLASS, Object *self, struct MUIP_Setup *message 
)
{
    if (DoSuperMethodA(CLASS, self, (Msg) message))
    {
        SETUP_INST_DATA;
    
        data->ssd_EHN.ehn_Events   = IDCMP_RAWKEY;
        data->ssd_EHN.ehn_Priority = 0;
        data->ssd_EHN.ehn_Flags    = 0;
        data->ssd_EHN.ehn_Object   = self;
        data->ssd_EHN.ehn_Class    = CLASS;
    
        DoMethod
        ( 
            _win(self), MUIM_Window_AddEventHandler, (IPTR) &data->ssd_EHN 
        );
    
        return TRUE;
    }
    
    return FALSE;
}

IPTR Presentation__MUIM_Cleanup
(
    Class *CLASS, Object *self, struct MUIP_Cleanup *message
)
{
    SETUP_INST_DATA;
    
    DoMethod
    ( 
        _win(self), MUIM_Window_RemEventHandler, (IPTR) &data->ssd_EHN 
    );

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Presentation__MUIM_HandleEvent
(
    Class *CLASS, Object *self, struct MUIP_HandleEvent *message
)
{
    if (message->imsg != NULL)
    {
        switch (message->imsg->Class)
        {
            case IDCMP_RAWKEY:
                switch (message->imsg->Code)
                {
                    case CURSORRIGHT:
                        /* Next slide --------------------------------------*/
                        SET
                        (
                            self, MUIA_Group_ActivePage, 
                            MUIV_Group_ActivePage_Next
                        );
                        break;
                    
                    case CURSORLEFT:
                        /* Previous slide ----------------------------------*/
                        SET
                        (
                            self, MUIA_Group_ActivePage, 
                            MUIV_Group_ActivePage_Prev
                        );
                        break;
                }
                break;
        }
    }
    
    return MUI_EventHandlerRC_Eat;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    Presentation, NULL, MUIC_Group, NULL,
    OM_NEW,           struct opSet *,
    OM_ADDMEMBER,     struct opMember *,
    OM_REMMEMBER,     struct opMember *,
    OM_SET,           struct opSet *,
    MUIM_Setup,       struct MUIP_Setup *,
    MUIM_Cleanup,     struct MUIP_Cleanup *,
    MUIM_HandleEvent, struct MUIP_HandleEvent *
);
