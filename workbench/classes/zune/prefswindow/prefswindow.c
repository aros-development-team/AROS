/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>

#include <string.h>

#include "prefswindow.h"
#include "prefswindow_private.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Locale functions *******************************************************/
CONST_STRPTR MSG(struct Catalog *catalog, ULONG id)
{
    if (catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[id].cca_Str);
    } 
    else 
    {
        return CatCompArray[id].cca_Str;
    }
}

#define _(id) MSG(catalog, id)

/*** Methods ****************************************************************/
Object *PrefsWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct PrefsWindow_DATA *data = NULL; 
    struct TagItem *tag        = NULL;    
    struct Catalog *catalog    = NULL;
    Object         *contents   = NULL;
    Object         *testButton, *revertButton, 
                   *saveButton, *useButton, *cancelButton;
       
    catalog = OpenCatalogA(NULL, "System/Classes/Zune/PrefsWindow.catalog", NULL);
    
    tag = FindTagItem(WindowContents, message->ops_AttrList);
    if (tag != NULL)
    {
        tag->ti_Tag = TAG_IGNORE;
        contents    = (Object *) tag->ti_Data;
    }
    
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
    
        MUIA_Window_CloseGadget, FALSE,
        
        WindowContents, (IPTR) VGroup,
            Child, (IPTR) contents,
            Child, (IPTR) RectangleObject, 
                MUIA_Rectangle_HBar, TRUE, 
                MUIA_FixHeight,      2, 
            End,
            Child, (IPTR) HGroup,
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    MUIA_Weight,             0,
                    
                    Child, (IPTR) testButton   = ImageButton(_(MSG_TEST), "THEME:Images/Gadgets/Preferences/Test.png"),
                    Child, (IPTR) revertButton = ImageButton(_(MSG_REVERT), "THEME:Images/Gadgets/Preferences/Revert.png"),
                End,
                Child, (IPTR) RectangleObject,
                    MUIA_Weight, 50,
                End,
                Child, (IPTR) RectangleObject,
                    MUIA_Weight, 50,
                End,
                Child, (IPTR) RectangleObject,
                    MUIA_Weight, 50,
                End,
                Child, (IPTR) HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    MUIA_Weight,             0,
                    
                    Child, (IPTR) saveButton   = ImageButton(_(MSG_SAVE), "THEME:Images/Gadgets/Preferences/Save.png"),
                    Child, (IPTR) useButton    = ImageButton(_(MSG_USE), "THEME:Images/Gadgets/Preferences/Use.png"),
                    Child, (IPTR) cancelButton = ImageButton(_(MSG_CANCEL), "THEME:Images/Gadgets/Preferences/Cancel.png"),
                End,
            End,
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        data->pwd_Catalog      = catalog;
        data->pwd_TestButton   = testButton;
        data->pwd_RevertButton = revertButton;
        data->pwd_SaveButton   = saveButton;
        data->pwd_UseButton    = useButton;
        data->pwd_CancelButton = cancelButton;
        
        /*-- Handle initial attribute values -------------------------------*/
        SetAttrsA(self, message->ops_AttrList);
       
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        ( 
            self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Cancel 
        );
        
        DoMethod
        ( 
            testButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Test 
        );
        DoMethod
        ( 
            revertButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Revert
        );
        DoMethod
        ( 
            saveButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Save
        );
        DoMethod
        ( 
            useButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Use
        );
        DoMethod
        ( 
            cancelButton, MUIM_Notify, MUIA_Pressed, FALSE, 
            (IPTR) self, 1, MUIM_PrefsWindow_Cancel
        );
    }
    else
    {
        if (catalog != NULL) CloseCatalog(catalog);
    }
    
    return self;
}

IPTR PrefsWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct PrefsWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->pwd_Catalog != NULL) CloseCatalog(data->pwd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}

IPTR PrefsWindow__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct PrefsWindow_DATA *data   = INST_DATA(CLASS, self);
    struct TagItem                *tstate = message->ops_AttrList,
                                  *tag;
                                  
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PrefsWindow_Test_Disabled:
                SET(data->pwd_TestButton, MUIA_Disabled, tag->ti_Data);
                break;
                
            case MUIA_PrefsWindow_Revert_Disabled:
                SET(data->pwd_RevertButton, MUIA_Disabled, tag->ti_Data);
                break;
                
            case MUIA_PrefsWindow_Save_Disabled:
                SET(data->pwd_SaveButton, MUIA_Disabled, tag->ti_Data);
                break;
                
            case MUIA_PrefsWindow_Use_Disabled:
                SET(data->pwd_UseButton, MUIA_Disabled, tag->ti_Data);
                break;
                
            case MUIA_PrefsWindow_Cancel_Disabled:
                SET(data->pwd_CancelButton, MUIA_Disabled, tag->ti_Data);
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR PrefsWindow__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    struct PrefsWindow_DATA *data  = INST_DATA(CLASS, self);
    IPTR                          *store = message->opg_Storage;
    IPTR                           rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_PrefsWindow_Test_Disabled:
            *store = XGET(data->pwd_TestButton, MUIA_Disabled);
            break;
            
        case MUIA_PrefsWindow_Revert_Disabled:
            *store = XGET(data->pwd_RevertButton, MUIA_Disabled);
            break;
            
        case MUIA_PrefsWindow_Save_Disabled:
            *store = XGET(data->pwd_SaveButton, MUIA_Disabled);
            break;
            
        case MUIA_PrefsWindow_Use_Disabled:
            *store = XGET(data->pwd_UseButton, MUIA_Disabled);
            break;
            
        case MUIA_PrefsWindow_Cancel_Disabled:
            *store = XGET(data->pwd_CancelButton, MUIA_Disabled);
            break;
            
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}
