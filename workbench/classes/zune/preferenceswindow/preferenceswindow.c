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
#include <proto/locale.h>

#include "preferenceswindow.h"

#define CATCOMP_ARRAY
#include "strings.h"

/*** Locale functions *******************************************************/

STRPTR __MSG(struct Catalog *catalog, ULONG id)
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

#define MSG(id) __MSG(catalog,id)

/*** Instance data **********************************************************/

struct PreferencesWindow_DATA
{
    struct Catalog *pwd_Catalog;
};

/*** Methods ****************************************************************/

IPTR PreferencesWindow$OM_NEW
(
    struct IClass *CLASS, Object *self, struct opSet *message 
)
{
    struct PreferencesWindow_DATA *data = NULL; 
    struct TagItem *tag      = NULL;    
    struct Catalog *catalog  = NULL;
    Object         *contents = NULL;
    Object         *testButton, *revertButton, 
                   *saveButton, *useButton, *cancelButton;
    
    struct TagItem tags[] =
    {
        { MUIA_Window_CloseGadget,        FALSE                 },
        { WindowContents,                 NULL                  },
        { TAG_MORE,                (IPTR) message->ops_AttrList }
    };
    
    catalog = OpenCatalogA( NULL, "SYS/Zune/PreferencesWindow.catalog", NULL );
    
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
                
                Child, testButton   = SimpleButton( MSG(MSG_TEST) ),
                Child, revertButton = SimpleButton( MSG(MSG_REVERT) ),
            End,
            Child, RectangleObject,
                MUIA_Weight, 50,
            End,
            Child, HGroup,
                MUIA_Group_SameWidth, TRUE,
                MUIA_Weight,             0,
                
                Child, saveButton   = SimpleButton( MSG(MSG_SAVE) ),
                Child, useButton    = SimpleButton( MSG(MSG_USE) ),
                Child, cancelButton = SimpleButton( MSG(MSG_CANCEL) ),
            End,
        End,
    End;
    
    message->ops_AttrList = tags;
          
    self = (Object *) DoSuperMethodA( CLASS, self, (Msg) message );
    if( self == NULL ) goto error;
    
    data = INST_DATA( CLASS, self );
    data->pwd_Catalog = catalog;
    
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
    
error:
    return NULL;

    if( catalog != NULL ) CloseCatalog( catalog );
}

IPTR PreferencesWindow$OM_DISPOSE
(
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct PreferencesWindow_DATA *data = INST_DATA( CLASS, self );

    if( data->pwd_Catalog != NULL ) CloseCatalog( data->pwd_Catalog );
    
    return DoSuperMethodA( CLASS, self, message );
}