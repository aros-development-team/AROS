/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "preferenceswindow.h"

/*** Methods ****************************************************************/

IPTR PreferencesWindow$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct TagItem *tag      = NULL;    
    Object         *contents = NULL;
    Object         *testButton, *revertButton, 
                   *saveButton, *useButton, *cancelButton;
    
    struct TagItem tags[] =
    {
        { MUIA_Window_CloseGadget,        FALSE                 },
        { WindowContents,                 NULL                  },
        { TAG_MORE,                (IPTR) message->ops_AttrList }
    };
    
    tag = FindTagItem( WindowContents, message->ops_AttrList );
    if( tag != NULL )
    {
        tag->ti_Tag = TAG_IGNORE;
        contents    = (Object *) tag->ti_Data;
    }
    
    tags[1].ti_Data = (IPTR) VGroup,
        Child, contents,
        Child, RectangleObject, 
            MUIA_Rectangle_HBar, TRUE, 
            MUIA_FixHeight, 2, 
        End,
        Child, HGroup,
            Child, HGroup,
                MUIA_Group_SameWidth, TRUE,
                MUIA_Weight,             0,
                
                Child, testButton   = SimpleButton( "_Test" ),
                Child, revertButton = SimpleButton( "_Revert" ),
            End,
            Child, RectangleObject,
                MUIA_Weight, 50,
            End,
            Child, HGroup,
                MUIA_Group_SameWidth, TRUE,
                MUIA_Weight,             0,
                
                Child, saveButton   = SimpleButton( "_Save " ),
                Child, useButton    = SimpleButton( "_Use " ),
                Child, cancelButton = SimpleButton( "_Cancel " ),
            End,
        End,
    End;
    
    message->ops_AttrList = tags;
          
    self = (Object *) DoSuperMethodA( CLASS, self, (Msg) message );
    if( self == NULL ) return FALSE;
    
    DoMethod
    ( 
        testButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Test 
    );
    DoMethod
    ( 
        revertButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Revert
    );
    DoMethod
    ( 
        saveButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Save
    );
    DoMethod
    ( 
        useButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Use
    );
    DoMethod
    ( 
        cancelButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Cancel
    );
    
    return self;
}
