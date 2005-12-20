/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    This file is part of the SystemPrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/prefseditor.h>
#include <zune/prefswindow.h>

#include "systemprefswindow.h"
#include "systemprefswindow_private.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct SystemPrefsWindow_DATA *data = INST_DATA(CLASS, self)

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

#define _(id)  MSG(catalog, (id))
#define __(id) (IPTR) MSG(catalog, (id))

/*** Utility functions ******************************************************/
Object *MakeMenuitem(CONST_STRPTR text)
{
    CONST_STRPTR title    = NULL, 
                 shortcut = NULL;
    
    if (text != NM_BARLABEL && text[1] == '\0')
    {
        title    = text + 2;
        shortcut = text;
    }
    else
    {
        title    = text;
        shortcut = NULL;
    }
    
    return MenuitemObject,
        MUIA_Menuitem_Title,      (IPTR) title,
        shortcut != NULL       ?
        MUIA_Menuitem_Shortcut :
        TAG_IGNORE,               (IPTR) shortcut,
    End;
}

/*** Methods ****************************************************************/
Object *SystemPrefsWindow__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct SystemPrefsWindow_DATA *data = NULL; 
    struct TagItem *tag        = NULL;    
    struct Catalog *catalog    = NULL;
    Object         *editor, /* *importMI, *exportMI, */ *testMI, *revertMI, 
                   *saveMI, *useMI, *cancelMI;
    
    tag = FindTagItem(WindowContents, message->ops_AttrList);
    if (tag != NULL) editor = (Object *) tag->ti_Data;
    if (editor == NULL) return NULL;
    
    /*--- Initialize locale ------------------------------------------------*/
    catalog = OpenCatalogA
    (
        NULL, "System/Classes/Zune/SystemPrefsWindow.catalog", NULL
    );
    
    /*--- Create object ----------------------------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Window_Title, XGET(editor, MUIA_PrefsEditor_Name),
        
        MUIA_Window_Menustrip, (IPTR) MenustripObject,
            Child, (IPTR) MenuObject,
                MUIA_Menu_Title, __(MSG_MENU_PREFS),
                
                /* FIXME: implement
                Child, (IPTR) importMI = MakeMenuitem(_(MSG_MENU_PREFS_IMPORT)),
                Child, (IPTR) exportMI = MakeMenuitem(_(MSG_MENU_PREFS_EXPORT)),
                Child, (IPTR) MakeMenuitem(NM_BARLABEL),
                */
                
                Child, (IPTR)(testMI   = MakeMenuitem(_(MSG_MENU_PREFS_TEST))),
                Child, (IPTR)(revertMI = MakeMenuitem(_(MSG_MENU_PREFS_REVERT))),
                Child, MakeMenuitem(NM_BARLABEL),
                Child, (IPTR)(saveMI   = MakeMenuitem(_(MSG_MENU_PREFS_SAVE))),
                Child, (IPTR)(useMI    = MakeMenuitem(_(MSG_MENU_PREFS_USE))),
                Child, (IPTR)(cancelMI = MakeMenuitem(_(MSG_MENU_PREFS_CANCEL))),
            End,
        End, 
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    
    if (self != NULL)
    {
        data = INST_DATA(CLASS, self);
        data->spwd_Catalog = catalog;
        data->spwd_Editor  = editor;
        
        /*-- Handle initial attribute values -------------------------------*/
        SetAttrsA(self, message->ops_AttrList);

        /*-- Setup initial button states -----------------------------------*/
        SET(self, MUIA_PrefsWindow_Test_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Revert_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Save_Disabled, TRUE);
        SET(self, MUIA_PrefsWindow_Use_Disabled, TRUE);
        
        /*-- Setup notifications -------------------------------------------*/
        DoMethod
        (
            editor, MUIM_Notify, MUIA_PrefsEditor_Changed, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_SystemPrefsWindow_UpdateButtons
        );
        DoMethod
        (
            editor, MUIM_Notify, MUIA_PrefsEditor_Testing, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_SystemPrefsWindow_UpdateButtons
        );

        DoMethod
        (
            testMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Test
        );
        DoMethod
        (
            revertMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Revert
        );
        DoMethod
        (
            saveMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Save
        );
        DoMethod
        (
            useMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Use
        );
        DoMethod
        (
            cancelMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            (IPTR) self, 1, MUIM_PrefsWindow_Cancel
        );
    }
    else
    {
        if (catalog != NULL) CloseCatalog(catalog);
    }
    
    return self;
}

IPTR SystemPrefsWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message 
)
{
    SETUP_INST_DATA;

    if (data->spwd_Catalog != NULL) CloseCatalog(data->spwd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}

IPTR SystemPrefsWindow__MUIM_SystemPrefsWindow_UpdateButtons
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;

    BOOL changed = XGET(data->spwd_Editor, MUIA_PrefsEditor_Changed);
    BOOL testing = XGET(data->spwd_Editor, MUIA_PrefsEditor_Testing);
    
    SET(self, MUIA_PrefsWindow_Test_Disabled, !changed);
    SET(self, MUIA_PrefsWindow_Revert_Disabled, !testing);

    SET(self, MUIA_PrefsWindow_Save_Disabled, !(changed || testing));
    SET(self, MUIA_PrefsWindow_Use_Disabled, !(changed || testing));

    return 0;
}

IPTR SystemPrefsWindow__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;
                                  
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PrefsWindow_Save_Disabled:
                if (!XGET(data->spwd_Editor, MUIA_PrefsEditor_CanSave))
                {
                    tag->ti_Tag = TAG_IGNORE;
                }
                break;
                
            case MUIA_PrefsWindow_Test_Disabled:
            case MUIA_PrefsWindow_Revert_Disabled:
                if (!XGET(data->spwd_Editor, MUIA_PrefsEditor_CanTest))
                {
                    tag->ti_Tag = TAG_IGNORE;
                }
                break;
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR SystemPrefsWindow__MUIM_PrefsWindow_Test
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Test))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    return TRUE;
}

IPTR SystemPrefsWindow__MUIM_PrefsWindow_Revert
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Revert))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    return TRUE;
}

IPTR SystemPrefsWindow__MUIM_PrefsWindow_Save
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Save))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    return TRUE;
}

IPTR SystemPrefsWindow__MUIM_PrefsWindow_Use
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Use))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    return TRUE;
}

IPTR SystemPrefsWindow__MUIM_PrefsWindow_Cancel
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    
    if (!DoMethod(data->spwd_Editor, MUIM_PrefsEditor_Cancel))
    {
        // FIXME: error reporting
        return FALSE;
    }
    
    DoMethod
    (
        _app(self), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );

    return TRUE;
}
