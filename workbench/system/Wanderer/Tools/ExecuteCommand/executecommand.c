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

#include <stdio.h>

#include "executecommand.h"
#include "locale.h"
#include "support.h"
          
/*** Private methods ********************************************************/
#define MUIM_ExecuteCommand_ExecuteCommand  (TAG_USER | 0x20000000)
struct  MUIP_ExecuteCommand_ExecuteCommand  { ULONG MethodID; ULONG delayed; };

/*** Instance data **********************************************************/
struct ExecuteCommand_DATA
{
    Object *ecd_Window,
           *ecd_CommandString,
           *ecd_CommandPop;
    BPTR    ecd_Parent;
    BOOL    ecd_UnlockParent;
    BOOL    ecd_SaveCommand;
};

/*** Methods ****************************************************************/
Object *ExecuteCommand__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct ExecuteCommand_DATA *data           = NULL;
    const struct TagItem       *tstate         = message->ops_AttrList,
                               *tag            = NULL;
    BPTR                        parent         = NULL;
    BOOL                        unlockParent   = FALSE;
    CONST_STRPTR                initial        = NULL;
    BOOL                        freeInitial    = FALSE;
    BOOL                        saveCommand    = FALSE;
    Object                     *window,
                               *commandString,
                               *commandPop,
                               *executeButton,
                               *cancelButton;
    
    /* Parse initial taglist -----------------------------------------------*/
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ExecuteCommand_Parent:
                parent = (BPTR) tag->ti_Data;
                break;
                
            case MUIA_ExecuteCommand_Initial:
                initial = (CONST_STRPTR) tag->ti_Data;
                break;
        }
    }
    
    /* Setup parameters ----------------------------------------------------*/
    if (parent == NULL)
    {
        if ((parent = Lock("RAM:", ACCESS_READ)) != NULL)
        {
            unlockParent = TRUE;
        }
    }
    
    if (initial == NULL)
    {
        if ((initial = GetENV("LastExecutedCommand")) != NULL)
        {
            freeInitial = TRUE;
        }
        else
        {
            initial = "";
        }
        
        saveCommand = TRUE;
    }
    
    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title, __(MSG_TITLE),
        
        SubWindow, (IPTR) (window = WindowObject,
            MUIA_Window_Title,       __(MSG_TITLE),
            MUIA_Window_Activate,    TRUE,
            MUIA_Window_NoMenus,     TRUE,
            MUIA_Window_CloseGadget, FALSE,
            
            WindowContents, (IPTR) VGroup,
                Child, (IPTR) VGroup,
                    Child, (IPTR) LLabel(_(MSG_LABEL_COMMANDLINE)),
                    Child, (IPTR) (commandPop = PopaslObject,
                        MUIA_Popstring_String, (IPTR) (commandString = StringObject,
                            MUIA_String_Contents, (IPTR) initial,
                            MUIA_CycleChain,             1,
                            StringFrame,
                        End),
                        MUIA_Popstring_Button, (IPTR) PopButton(MUII_PopFile),
                    End),
                End,
                Child, (IPTR) HGroup,
                    InnerSpacing(0, 0),
                    GroupSpacing(0),
                    
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) HGroup,
                        MUIA_Group_SameSize, TRUE,
                        MUIA_Weight,         0,
                        
                        Child, (IPTR) (executeButton = SimpleButton(_(MSG_BUTTON_EXECUTE))),
                        Child, (IPTR) (cancelButton  = SimpleButton(_(MSG_BUTTON_CANCEL))),
                    End,
                End,
            End,
        End),
        
        TAG_DONE
    );
    
    /* Not needed anymore */
    if (freeInitial) FreeVec((APTR) initial); 
    
    /* Check if object creation succeeded */
    if (self == NULL)
    {
    	if (unlockParent) UnLock(parent);
	
    	return NULL;
    }
    
    /* Store instance data -------------------------------------------------*/
    data = INST_DATA(CLASS, self);
    data->ecd_Window        = window;
    data->ecd_CommandString = commandString;
    data->ecd_CommandPop    = commandPop;
    data->ecd_Parent        = parent;
    data->ecd_UnlockParent  = unlockParent;
    data->ecd_SaveCommand   = saveCommand;
    
    /* Setup notifications -------------------------------------------------*/
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
        (IPTR) self, 2, MUIM_ExecuteCommand_ExecuteCommand, TRUE
    );
    
    DoMethod
    (
        executeButton, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR) self, 2, MUIM_ExecuteCommand_ExecuteCommand, TRUE
    );
    
    DoMethod
    (
        commandPop, MUIM_Notify, MUIA_Popasl_Active, FALSE,
        (IPTR) window, 3, MUIM_Set, MUIA_Window_ActiveObject, (IPTR) commandString
    );
    
    return self;
}

IPTR ExecuteCommand__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct ExecuteCommand_DATA *data = INST_DATA(CLASS, self);
    
    if (data->ecd_UnlockParent && data->ecd_Parent != NULL)
    {
        UnLock(data->ecd_Parent);
    }
    
    return DoSuperMethodA(CLASS, self, message);
}

IPTR ExecuteCommand__MUIM_Application_Execute
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct ExecuteCommand_DATA *data = INST_DATA(CLASS, self);
    
    SET(data->ecd_Window, MUIA_Window_Open, TRUE);
    SET(data->ecd_Window, MUIA_Window_ActiveObject, (IPTR) data->ecd_CommandString);

    DoSuperMethodA(CLASS, self, message);
    
    SET(data->ecd_Window, MUIA_Window_Open, FALSE);

    return 0;
}

IPTR ExecuteCommand__MUIM_ExecuteCommand_ExecuteCommand
(
    Class *CLASS, Object *self, 
    struct MUIP_ExecuteCommand_ExecuteCommand *message
)
{
    struct ExecuteCommand_DATA *data = INST_DATA(CLASS, self);
    
    if (message->delayed)
    {
        if (!XGET(data->ecd_CommandPop, MUIA_Popasl_Active))
        {
            SET(data->ecd_Window, MUIA_Window_Open, FALSE);
            
            DoMethod
            (
                self, MUIM_Application_PushMethod, (IPTR) self, 2,
                MUIM_ExecuteCommand_ExecuteCommand, FALSE
            );
        }
    }
    else
    {
        BPTR   console, cd = NULL;
        STRPTR command;
        
        GET(data->ecd_CommandString, MUIA_String_Contents, &command);
        
        if (data->ecd_SaveCommand) SetENV("LastExecutedCommand", command);
        
        /* Make sure that the commandline isn't just whitespace or NULL */
        if (command != NULL && command[strspn(command, " \t")] != '\0')
        {
            TEXT   buffer[1024];
            
            if
            (
                sizeof(buffer) > snprintf
                (
                    buffer, sizeof(buffer), 
                    "CON:////%s/CLOSE/AUTO/WAIT", _(MSG_CONSOLE_TITLE)
                )
            )
            {
                /* The string was not truncated */
                console = Open(buffer, MODE_OLDFILE);
            }
            else
            {
                /* The string was truncated; fallback to english title */
                console = Open("CON:////Command Output/CLOSE/AUTO/WAIT", MODE_OLDFILE);
            }
            
            if (console != NULL)
            {
                BPTR searchPath = NULL;
                
                WorkbenchControl
                (
                    NULL, 
                    WBCTRLA_DuplicateSearchPath, (IPTR) &searchPath,
                    TAG_DONE
                );
                
                if (data->ecd_Parent != NULL) cd = CurrentDir(data->ecd_Parent);
                
                if
                (
                    SystemTags
                    (
                        command,
                        
                        SYS_Asynch,	       TRUE,
                        SYS_Input,      (IPTR) console,
                        SYS_Output,     (IPTR) NULL,
                        SYS_Error,      (IPTR) NULL,
                        SYS_Background,        FALSE,
                        NP_Path,        (IPTR) searchPath,
                        
                        TAG_DONE
                    ) == -1
                )
                {
                    /*
                        An error occured, so we need to close the filehandle
                        and free the search path list (which are otherwise
                        automatically freed when the process exits).
                    */
                    
                    ShowError
                    (
                        self, data->ecd_Window, 
                        _(MSG_ERROR_EXECUTE), TRUE
                    );
                    
                    WorkbenchControl
                    (
                        NULL,
                        WBCTRLA_FreeSearchPath, (IPTR) searchPath,
                        TAG_DONE
                    );
                    
                    Close(console);
                }
                
                if (cd != NULL) CurrentDir(cd);
            }
            else
            {
                ShowError
                (
                    self, data->ecd_Window, 
                    _(MSG_ERROR_OPEN_CONSOLE), TRUE
                );
            }            
        }
    
        DoMethod(self, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    }
    
    return 0;
}


/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, ExecuteCommand_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                             return (IPTR) ExecuteCommand__OM_NEW(CLASS, self, (struct opSet *) message);
        case OM_DISPOSE:                         return ExecuteCommand__OM_DISPOSE(CLASS, self, message);
        case MUIM_Application_Execute:           return ExecuteCommand__MUIM_Application_Execute(CLASS, self, message);
        case MUIM_ExecuteCommand_ExecuteCommand: return ExecuteCommand__MUIM_ExecuteCommand_ExecuteCommand(CLASS, self, (struct MUIP_ExecuteCommand_ExecuteCommand *) message);
        default:                                 return DoSuperMethodA(CLASS, self, message);
    }
    
    return 0;
}
BOOPSI_DISPATCHER_END

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
    if (ExecuteCommand_CLASS != NULL)
    {
        MUI_DeleteCustomClass(ExecuteCommand_CLASS);
    }
}
