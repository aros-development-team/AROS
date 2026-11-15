/*
    Copyright (C) 2003-2026, The AROS Development Team. All rights reserved.
    This file is part of the PrefsWindow class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
*/

/****** PrefsWindow.mcc/MUIC_PrefsWindow ************
*
*   NAME
*       MUIC_PrefsWindow
*
*   SUPERCLASS
*       Window.mui
*
*   DESCRIPTION
*       Build a preferences-window shell around caller-supplied WindowContents,
*       with Test, Revert, Save, Use, and Cancel buttons. Labels come from
*       PrefsWindow.catalog when available, otherwise compiled defaults. Button
*       images use the corresponding THEME:Images/Gadgets paths. The configured
*       class version is 1.2.
*       Button release sends the corresponding MUIM_PrefsWindow_* method to the
*       window. CloseRequest sends Cancel. All eight action methods in the
*       public header are abstract: this base class implements no preference
*       loading, writing, testing, restoration, or exit policy. A subclass must
*       implement the required actions; SystemPrefsWindow provides the
*       PrefsEditor adapter.
*       Supply a valid content object through one WindowContents tag. The
*       constructor finds the first tag, changes it to TAG_IGNORE, and inserts
*       the supplied object into its own group hierarchy. The tag is not
*       restored. Use writable tag storage, avoid duplicate WindowContents tags,
*       and do not dispose an adopted child separately after successful
*       construction.
*
*   NOTES
*       The constructor disables the normal close gadget by default, then
*       forwards remaining caller tags. It applies initial Disabled attributes
*       through OM_SET after building the buttons. These attributes initialize,
*       set, and get the corresponding button's MUIA_Disabled value; no separate
*       preference state is stored here.
*       Disabling a button affects interaction with that button. It does not
*       implement an authorization check or prevent direct method calls. This
*       class has no Import, Export, or Defaults button or menu. Do not infer
*       behavior for those declared methods from their names.
*       The class opens its catalog at construction and closes it at disposal or
*       superclass-construction failure. The Window superclass owns the content
*       hierarchy. There is no local policy that automatically closes the window
*       or quits the application on an action.
*
*/
/****** PrefsWindow.mcc/MUIA_PrefsWindow_Test_Disabled ************
*
*   NAME
*       MUIA_PrefsWindow_Test_Disabled
*
*   FUNCTION
*       Control whether the Test button is disabled.
*
*   INPUTS
*       Initialize, set, get; BOOL. No explicit local disabled default is
*       assigned; the button supplies its normal initial state.
*
*   NOTES
*       Set forwards the value to the internal button through MUIA_Disabled. Get
*       queries that button and returns TRUE for this attribute. The window then
*       forwards the original setter tags to its superclass. The attribute does
*       not stop direct MUIM_PrefsWindow_Test calls.
*       SystemPrefsWindow can force this value to TRUE according to editor
*       capability flags and updates action buttons from Changed/Testing
*       notifications. Its behavior is documented separately.
*
*/
/****** PrefsWindow.mcc/MUIA_PrefsWindow_Revert_Disabled ************
*
*   NAME
*       MUIA_PrefsWindow_Revert_Disabled
*
*   FUNCTION
*       Control whether the Revert button is disabled.
*
*   INPUTS
*       Initialize, set, get; BOOL. No explicit local disabled default is
*       assigned; the button supplies its normal initial state.
*
*   NOTES
*       Set forwards the value to the internal button through MUIA_Disabled. Get
*       queries that button and returns TRUE for this attribute. The window then
*       forwards the original setter tags to its superclass. The attribute does
*       not stop direct MUIM_PrefsWindow_Revert calls.
*       SystemPrefsWindow can force this value to TRUE according to editor
*       capability flags and updates action buttons from Changed/Testing
*       notifications. Its behavior is documented separately.
*
*/
/****** PrefsWindow.mcc/MUIA_PrefsWindow_Save_Disabled ************
*
*   NAME
*       MUIA_PrefsWindow_Save_Disabled
*
*   FUNCTION
*       Control whether the Save button is disabled.
*
*   INPUTS
*       Initialize, set, get; BOOL. No explicit local disabled default is
*       assigned; the button supplies its normal initial state.
*
*   NOTES
*       Set forwards the value to the internal button through MUIA_Disabled. Get
*       queries that button and returns TRUE for this attribute. The window then
*       forwards the original setter tags to its superclass. The attribute does
*       not stop direct MUIM_PrefsWindow_Save calls.
*       SystemPrefsWindow can force this value to TRUE according to editor
*       capability flags and updates action buttons from Changed/Testing
*       notifications. Its behavior is documented separately.
*
*/
/****** PrefsWindow.mcc/MUIA_PrefsWindow_Use_Disabled ************
*
*   NAME
*       MUIA_PrefsWindow_Use_Disabled
*
*   FUNCTION
*       Control whether the Use button is disabled.
*
*   INPUTS
*       Initialize, set, get; BOOL. No explicit local disabled default is
*       assigned; the button supplies its normal initial state.
*
*   NOTES
*       Set forwards the value to the internal button through MUIA_Disabled. Get
*       queries that button and returns TRUE for this attribute. The window then
*       forwards the original setter tags to its superclass. The attribute does
*       not stop direct MUIM_PrefsWindow_Use calls.
*       SystemPrefsWindow can force this value to TRUE according to editor
*       capability flags and updates action buttons from Changed/Testing
*       notifications. Its behavior is documented separately.
*
*/
/****** PrefsWindow.mcc/MUIA_PrefsWindow_Cancel_Disabled ************
*
*   NAME
*       MUIA_PrefsWindow_Cancel_Disabled
*
*   FUNCTION
*       Control whether the Cancel button is disabled.
*
*   INPUTS
*       Initialize, set, get; BOOL. No explicit local disabled default is
*       assigned; the button supplies its normal initial state.
*
*   NOTES
*       Set forwards the value to the internal button through MUIA_Disabled. Get
*       queries that button and returns TRUE for this attribute. The window then
*       forwards the original setter tags to its superclass. The attribute does
*       not stop direct MUIM_PrefsWindow_Cancel calls.
*       SystemPrefsWindow can force this value to TRUE according to editor
*       capability flags and updates action buttons from Changed/Testing
*       notifications. Its behavior is documented separately.
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Test ************
*
*   NAME
*       MUIM_PrefsWindow_Test
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Test);
*
*   FUNCTION
*       Declare the Test action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       The corresponding button release invokes this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Test
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Revert ************
*
*   NAME
*       MUIM_PrefsWindow_Revert
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Revert);
*
*   FUNCTION
*       Declare the Revert action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       The corresponding button release invokes this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Revert
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Save ************
*
*   NAME
*       MUIM_PrefsWindow_Save
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Save);
*
*   FUNCTION
*       Declare the Save action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       The corresponding button release invokes this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Save
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Use ************
*
*   NAME
*       MUIM_PrefsWindow_Use
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Use);
*
*   FUNCTION
*       Declare the Use action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       The corresponding button release invokes this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Use
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Cancel ************
*
*   NAME
*       MUIM_PrefsWindow_Cancel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Cancel);
*
*   FUNCTION
*       Declare the Cancel action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       The corresponding button release invokes this method.
*       Window_CloseRequest also invokes it. The base method list has no handler
*       for this ID. It does not perform I/O, change editor state, close the
*       window, or request application exit. See the SystemPrefsWindow
*       implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Cancel
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Import ************
*
*   NAME
*       MUIM_PrefsWindow_Import
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Import);
*
*   FUNCTION
*       Declare the Import action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       This base class installs no UI trigger for this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Import
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_Export ************
*
*   NAME
*       MUIM_PrefsWindow_Export
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_Export, withIcon);
*
*   FUNCTION
*       Declare the Export action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       withIcon - ULONG request flag, interpreted by the implementing subclass.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       This base class installs no UI trigger for this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_Export
*
*/
/****** PrefsWindow.mcc/MUIM_PrefsWindow_SetDefaults ************
*
*   NAME
*       MUIM_PrefsWindow_SetDefaults
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PrefsWindow_SetDefaults);
*
*   FUNCTION
*       Declare the SetDefaults action for a preferences-window subclass.
*
*   INPUTS
*       obj - PrefsWindow-derived object implementing the action.
*       No payload arguments.
*
*   RESULT
*       No implementation-specific result is defined by PrefsWindow; the method
*       is abstract here.
*
*   NOTES
*       This base class installs no UI trigger for this method. The base method
*       list has no handler for this ID. It does not perform I/O, change editor
*       state, close the window, or request application exit. See the
*       SystemPrefsWindow implementation for its concrete behavior.
*
*   SEE ALSO
*       SystemPrefsWindow.mcc/MUIM_PrefsWindow_SetDefaults
*
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <dos/dos.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/dos.h>

#include <string.h>

#include "prefswindow.h"
#include "prefswindow_private.h"
#include "catalogs/catalog_version.h"

#define CATCOMP_ARRAY
#include "strings.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Locale functions *******************************************************/
/* Return a borrowed catalog string for a valid CatCompArray index, falling
 * back to the compiled default when no catalog is open. No range validation
 * or allocation occurs; catalog-backed storage must not outlive its
 * catalog.
 */
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
/* Open the optional catalog, take the first WindowContents value and
 * permanently mark its tag ignored, then build the enclosing content/button
 * hierarchy. Apply original remaining tags and install button/CloseRequest
 * notifications to abstract action IDs. Return object or NULL, closing
 * catalog on superclass failure. Require writable tags and a valid content
 * object.
 */
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
       
    catalog = OpenCatalog(NULL, "System/Classes/Zune/PrefsWindow.catalog",
                          OC_Version, CATALOG_VERSION,
                          TAG_DONE);
    
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
                    
                    Child, (IPTR) (testButton   = ImageButton(_(MSG_TEST), "THEME:Images/Gadgets/Test")),
                    Child, (IPTR) (revertButton = ImageButton(_(MSG_REVERT), "THEME:Images/Gadgets/Revert")),
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
                    
                    Child, (IPTR) (saveButton   = ImageButton(_(MSG_SAVE), "THEME:Images/Gadgets/Save")),
                    Child, (IPTR) (useButton    = ImageButton(_(MSG_USE), "THEME:Images/Gadgets/Use")),
                    Child, (IPTR) (cancelButton = ImageButton(_(MSG_CANCEL), "THEME:Images/Gadgets/Cancel")),
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

/* Close the owned catalog when present, then delegate disposal of the
 * Window and its content hierarchy. Button and content objects are not
 * independently freed here. Return the superclass disposal result.
 */
IPTR PrefsWindow__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    struct PrefsWindow_DATA *data = INST_DATA(CLASS, self);

    if (data->pwd_Catalog != NULL) CloseCatalog(data->pwd_Catalog);
    
    return DoSuperMethodA(CLASS, self, message);
}

/* Process all five action Disabled attributes by setting the matching
 * internal button MUIA_Disabled. Forward the original tags afterward and
 * return the superclass result. No action method is blocked and no
 * preference operation is performed.
 */
IPTR PrefsWindow__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct PrefsWindow_DATA *data   = INST_DATA(CLASS, self);
    struct TagItem          *tstate = message->ops_AttrList;
    struct TagItem          *tag;
                                  
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

/* Return the matching button Disabled value for each local attribute with
 * TRUE. Forward all other queries to Window. The result reflects button
 * state rather than editor capability or successful preference persistence.
 */
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
