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
#include "presenter.h"

/*** Instance data **********************************************************/
struct Presenter_DATA
{
    Object *pd_Window;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct Presenter_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *Presenter__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    Object *window;
    
    self = (Object *) DoSuperNewTags
    ( 
        CLASS, self, NULL,
        
        SubWindow, (IPTR) (window = WindowObject,
            MUIA_Window_Title,    (IPTR) "Presenter",
            MUIA_Window_Activate,        TRUE,
            MUIA_Window_Width,           800,
            MUIA_Window_Height,          600,
            
            WindowContents, (IPTR) (PresentationObject,
            End),
        End),
    
        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        /* Store instance data ---------------------------------------------*/
        data->pd_Window = window;
        
        /* Setup notifications ---------------------------------------------*/
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
    }

    return self;
}

IPTR Presenter__MUIM_Application_Execute
(
    Class *CLASS, Object *self, Msg message 
)
{
    SETUP_INST_DATA;
    
    SET(data->pd_Window, MUIA_Window_Open, TRUE);
    DoSuperMethodA(CLASS, self, message);
    SET(data->pd_Window, MUIA_Window_Open, FALSE);

    return TRUE;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_2
(
    Presenter, NULL, MUIC_Application, NULL,
    OM_NEW,                   struct opSet *,
    MUIM_Application_Execute, Msg
);
