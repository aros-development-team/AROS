/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
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

#include "preferenceswindow.h"
#include "preferenceswindow_private.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Locale functions *******************************************************/
STRPTR MSG(struct Catalog *catalog, ULONG id)
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
IPTR PreferencesWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct PreferencesWindow_DATA *data = NULL; 
    struct TagItem *tag        = NULL;    
    struct Catalog *catalog    = NULL;
    BPTR            lock       = NULL;
    BOOL            enableSave = TRUE;
    Object         *contents   = NULL;
    Object         *testButton, *revertButton, 
                   *saveButton, *useButton, *cancelButton;
       
    catalog = OpenCatalogA(NULL, "System/Classes/Zune/PreferencesWindow.catalog", NULL);
    
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

    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->pwd_Catalog = catalog;

    /* Disable the save button if ENVARC: is write-protected */
    lock = Lock("ENVARC:", SHARED_LOCK);
    if (lock != NULL)
    {
        struct InfoData id;
        
        if (Info(lock, &id))
        {
            if (id.id_DiskState == ID_WRITE_PROTECTED)
            {
                enableSave = FALSE;
            }
        }
        
        UnLock(lock);
    }
    else
    {
        enableSave = FALSE;
    }
    
    if (!enableSave)
    {
        set(saveButton, MUIA_Disabled, TRUE);
    }

    /* Setup notifications */
    DoMethod
    ( 
        self, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) self, 1, MUIM_PreferencesWindow_Cancel 
    );
    
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
    
    return (IPTR) self;
    
error:
    if (catalog != NULL) CloseCatalog(catalog);
    
    return (IPTR) NULL;
}

IPTR PreferencesWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct PreferencesWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->pwd_Catalog != NULL) CloseCatalog(data->pwd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}
