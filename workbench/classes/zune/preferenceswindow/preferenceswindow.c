/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>

#include <string.h>

#include "preferenceswindow.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

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
        
        WindowContents, VGroup,
            Child, contents,
            Child, RectangleObject, 
                MUIA_Rectangle_HBar, TRUE, 
                MUIA_FixHeight, 2, 
            End,
            Child, HGroup,
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    MUIA_Weight,             0,
                    
                    Child, testButton   = ImageButton(MSG(MSG_TEST), "THEME:Images/Gadgets/Preferences/Test.png"),
                    Child, revertButton = ImageButton(MSG(MSG_REVERT), "THEME:Images/Gadgets/Preferences/Revert.png"),
                End,
                Child, RectangleObject,
                    MUIA_Weight, 50,
                End,
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    MUIA_Weight,             0,
                    
                    Child, saveButton   = ImageButton(MSG(MSG_SAVE), "THEME:Images/Gadgets/Preferences/Save.png"),
                    Child, useButton    = ImageButton(MSG(MSG_USE), "THEME:Images/Gadgets/Preferences/Use.png"),
                    Child, cancelButton = ImageButton(MSG(MSG_CANCEL), "THEME:Images/Gadgets/Preferences/Cancel.png"),
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
    
    return self;
    
error:
    if (catalog != NULL) CloseCatalog(catalog);
    
    return NULL;
}

IPTR PreferencesWindow$OM_DISPOSE
(
    struct IClass *CLASS, Object *self, Msg message 
)
{
    struct PreferencesWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->pwd_Catalog != NULL) CloseCatalog(data->pwd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}
