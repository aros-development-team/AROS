/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include "executecommand.h"
//#include "locale.h"

          
/*** Private methods ********************************************************/
#define MUIM_ExecuteCommand_Execute (TAG_USER | 0x20000000)

/*** Instance data **********************************************************/
struct ExecuteCommand_DATA
{
    Object *ecd_Window,
           *ecd_CommandString;
    BPTR    ecd_Lock;
};

/*** Methods ****************************************************************/
IPTR ExecuteCommand__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct ExecuteCommand_DATA *data           = NULL;
    struct TagItem             *tstate         = message->ops_AttrList,
                               *tag            = NULL;
    BPTR                        lock           = NULL;
    STRPTR                      name           = NULL;
    Object                     *window,
                               *commandString,
                               *executeButton,
                               *cancelButton;
    
    /* Parse initial taglist -----------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ExecuteCommand_Lock:
                lock = (BPTR) tag->ti_Data;
                D(bug("* got lock %p\n", lock));
                break;
                
            case MUIA_ExecuteCommand_Name:
                name = (STRPTR) tag->ti_Data;
                D(bug("* got name %s\n", name));
                break;
        }
    }
    
    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title, (IPTR) "Execute Command",
        
        SubWindow, (IPTR) window = WindowObject,
            MUIA_Window_Title,       (IPTR) "Execute Command",
            MUIA_Window_Activate,           TRUE,
            MUIA_Window_CloseGadget,        FALSE,
            
            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    GroupFrameT("Command and arguments"),
                    Child, (IPTR) PopaslObject,
                        MUIA_Popstring_String, (IPTR) commandString = StringObject,
                            MUIA_String_Contents, name != NULL ? (IPTR) name : (IPTR) "",
                            StringFrame,
                        End,
                        MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile),
                    End,
                End,
                Child, (IPTR) HGroup,
                    InnerSpacing(0, 0),
                    GroupSpacing(0),
                    
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) HGroup,
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Weight,         0,
                        
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) HVSpace,
                        Child, (IPTR) executeButton = SimpleButton("_Execute"),
                        Child, (IPTR) cancelButton  = SimpleButton("_Cancel"),
                    End,
                End,
            End,
        End,
        
        TAG_DONE
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->ecd_Window        = window;
    data->ecd_CommandString = commandString;
    data->ecd_Lock          = lock;
    
    DoMethod
    ( 
        window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    DoMethod
    ( 
        cancelButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
            
    DoMethod
    (
        commandString, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, 
        (IPTR) self, 1, MUIM_ExecuteCommand_Execute
    );
    
    DoMethod
    (
        executeButton, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR) self, 1, MUIM_ExecuteCommand_Execute
    );
    
    return (IPTR) self;
    
error:
    
    return NULL;
}

IPTR ExecuteCommand__MUIM_Application_Execute
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct ExecuteCommand_DATA *data    = INST_DATA(CLASS, self);
    ULONG                       signals = 0L;
    
    SET(data->ecd_Window, MUIA_Window_Open, TRUE);
    SET(data->ecd_Window, MUIA_Window_ActiveObject, (IPTR) data->ecd_CommandString);
    
    while
    ( 
           DoMethod(self, MUIM_Application_NewInput, (IPTR) &signals) 
        != MUIV_Application_ReturnID_Quit
    )
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C);
            if (signals & SIGBREAKF_CTRL_C) break;
        }
    }
    
    SET(data->ecd_Window, MUIA_Window_Open, FALSE);

    return NULL;
}

IPTR ExecuteCommand__MUIM_ExecuteCommand_Execute
(
    Class *CLASS, Object *self, Msg message
)
{
    struct ExecuteCommand_DATA *data = INST_DATA(CLASS, self);
    BPTR                        console, cd = NULL;
    STRPTR                      command;
    
    SET(data->ecd_Window, MUIA_Window_Open, FALSE);
    GET(data->ecd_CommandString, MUIA_String_Contents, &command);
    
    if (data->ecd_Lock != NULL) cd = CurrentDir(data->ecd_Lock);
    
    console = Open("CON:////Output Window/CLOSE/AUTO/WAIT", MODE_OLDFILE);
    if (console != NULL)
    {
        if
        (
            SystemTags
            (
                command,
	    	
                SYS_Asynch,	   TRUE,
	    	SYS_Input,  (IPTR) console,
	    	SYS_Output, (IPTR) NULL,
	    	SYS_Error,  (IPTR) NULL,
	    	
                TAG_DONE
            ) == -1
        )
        {
            /* An error occured, so we need to close the filehandle */
            Close(console);
        }
    }
    else
    {
        // FIXME: error dialog
    }
    
    if (cd != NULL) CurrentDir(cd);
    
    DoMethod(self, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    return NULL;
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, ExecuteCommand_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return ExecuteCommand__OM_NEW(CLASS, self, (struct opSet *) message);
        
        case MUIM_Application_Execute:
            return ExecuteCommand__MUIM_Application_Execute(CLASS, self, message);
        
        case MUIM_ExecuteCommand_Execute:
            return ExecuteCommand__MUIM_ExecuteCommand_Execute(CLASS, self, message);
            
        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    
    return NULL;
}

/*** Setup ******************************************************************/
struct MUI_CustomClass *ExecuteCommand_CLASS;

BOOL ExecuteCommand_Initialize()
{
    ExecuteCommand_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_Application, NULL, 
        sizeof(struct ExecuteCommand_DATA), ExecuteCommand_Dispatcher
    );

    return ExecuteCommand_CLASS != NULL ? TRUE : FALSE;
}

VOID ExecuteCommand_Deinitialize()
{
    MUI_DeleteCustomClass(ExecuteCommand_CLASS);
}
