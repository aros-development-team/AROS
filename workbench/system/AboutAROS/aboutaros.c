/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/build.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include <zune/aboutwindow.h>

#include "aboutaros.h"
#include "locale.h"
#include "logotype.h"

#define WINDOW_BG   ((IPTR) "2:00000000,00000000,00000000")
#define REGISTER_BG ((IPTR) "7:V,00000000,82000000,81000000-00000000,62000000,61000000")
#define LIST_BG     ((IPTR) "2:00000000,62000000,61000000")
          
/*** Private methods ********************************************************/
#define MUIM_AboutAROS_ShowLicense (TAG_USER | 0x20000000)

/*** Instance data **********************************************************/
struct AboutAROS_DATA
{
    Object *aad_Window;
};

/*** Methods ****************************************************************/
IPTR AboutAROS__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct AboutAROS_DATA *data           = NULL;
    Object                *window, 
                          *licenseButton,
                          *authorsList,
                          *sponsorsList,
                          *acksList;
    
    STRPTR                 pages[4]       = { NULL };
    BOOL                   showLogotype;
    BPTR                   lock;
    
    /* Check if the logotype is available ----------------------------------*/
    if ((lock = Lock(LOGOTYPE_IMAGE, ACCESS_READ)) != NULL)
    {
        showLogotype = TRUE;
        UnLock(lock);
    }
    else
    {
        showLogotype = FALSE;
    }
    

    
    /* Initialize page labels ----------------------------------------------*/
    pages[0] = _(MSG_PAGE_AUTHORS);
    pages[1] = _(MSG_PAGE_SPONSORS);
    pages[2] = _(MSG_PAGE_ACKNOWLEDGEMENTS);
    
    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title, __(MSG_TITLE),
        
        SubWindow, (IPTR) window = WindowObject,
            MUIA_Window_Title,    __(MSG_TITLE),
            MUIA_Window_Width,    MUIV_Window_Width_MinMax(0),
            MUIA_Window_NoMenus,  TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, (IPTR) VGroup,
                InnerSpacing(0, 0),
                GroupSpacing(2),
                MUIA_Background, WINDOW_BG,
                
                Child, showLogotype
                    ? (IPTR) ImageObject,
                          MUIA_Image_Spec, (IPTR)    "3:"LOGOTYPE_IMAGE,
                      End
                    : (IPTR) TextObject,
                          MUIA_Font,                 MUIV_Font_Fixed,
                          MUIA_Text_PreParse, (IPTR) "\0333\033b\033c",
                          MUIA_Text_Contents, (IPTR) LOGOTYPE_ASCII,
                          MUIA_Weight,               0,
                      End
                    ,
                Child, (IPTR) VSpace(4),
                Child, (IPTR) HGroup,
                    InnerSpacing(0,0),
                    GroupSpacing(6),
                    
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) "\0333\033b",
                        MUIA_Text_Contents,        __(MSG_BUILD_TYPE),
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) TextObject,
                        MUIA_Font,                 MUIV_Font_Big,
                        MUIA_Text_PreParse, (IPTR) "\0333\033b",
                        MUIA_Text_Contents, (IPTR) DATE,
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) HVSpace,
                End,
                Child, (IPTR) VSpace(4),
                Child, (IPTR) TextObject,
                    MUIA_Text_PreParse, (IPTR) "\0333\033c",
                    MUIA_Text_Contents,        __(MSG_COPYRIGHT),
                End,
                Child, (IPTR) HGroup,
                    InnerSpacing(0,0),
                    GroupSpacing(0),
                    
                    Child, (IPTR) HVSpace,
                    Child, (IPTR) TextObject,
                        MUIA_Text_PreParse, (IPTR) "\0333",
                        MUIA_Text_Contents,        __(MSG_LICENSE_1),
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) TextObject,
                        MUIA_Text_Contents, (IPTR) " ",
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) licenseButton = TextObject,
                        MUIA_InputMode,            MUIV_InputMode_RelVerify,
                        MUIA_Text_PreParse, (IPTR) "\0333\033u",
                        MUIA_Text_Contents,        __(MSG_LICENSE_2),
                        MUIA_Weight,        0,
                    End,
                    Child, (IPTR) TextObject,
                        MUIA_Text_PreParse, (IPTR) "\0333",
                        MUIA_Text_Contents,        __(MSG_LICENSE_3),
                        MUIA_Weight,               0,
                    End,
                    Child, (IPTR) HVSpace,
                End,
                Child, (IPTR) TextObject,
                    MUIA_Text_PreParse, (IPTR) "\0333\033c",
                    MUIA_Text_Contents,        __(MSG_MORE_INFORMATION),
                End,
                Child, (IPTR) VSpace(4),
                /* FIXME
                Child, (IPTR) VGroup,
                    InnerSpacing(4,4),
                    
                    Child, (IPTR) RegisterGroup(pages),
                        MUIA_Background, REGISTER_BG,
                                
                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) authorsList = ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End,
                        End,
                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) sponsorsList = ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End,
                        End,
                        Child, (IPTR) HVSpace,
                    End,
                End,
                */
            End,
        End,
        
        TAG_DONE
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->aad_Window = window;
    
    DoMethod
    ( 
        window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );
    
    DoMethod
    (
        licenseButton, MUIM_Notify, MUIA_Pressed, FALSE,
        (IPTR) self, 1, MUIM_AboutAROS_ShowLicense
    );
    
    return (IPTR) self;
    
error:
    
    return NULL;
}

IPTR AboutAROS__MUIM_Application_Execute
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct AboutAROS_DATA *data    = INST_DATA(CLASS, self);
    ULONG                  signals = 0L;
    
    SetAttrs(data->aad_Window, MUIA_Window_Open, TRUE, TAG_DONE);
    
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
    
    SetAttrs(data->aad_Window, MUIA_Window_Open, FALSE, TAG_DONE);

    return NULL;
}

IPTR AboutAROS__MUIM_AboutAROS_ShowLicense
(     
    Class *CLASS, Object *self, Msg message 
)
{
    OpenWorkbenchObject("HELP:LICENSE", TAG_DONE);
    
    return NULL;
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, AboutAROS_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW: 
            return AboutAROS__OM_NEW(CLASS, self, (struct opSet *) message);
        
        case MUIM_Application_Execute:
            return AboutAROS__MUIM_Application_Execute(CLASS, self, message);
        
        case MUIM_AboutAROS_ShowLicense:
            return AboutAROS__MUIM_AboutAROS_ShowLicense(CLASS, self, message);
        
        default:     
            return DoSuperMethodA(CLASS, self, message);
    }
    
    return NULL;
}

/*** Setup ******************************************************************/
struct MUI_CustomClass *AboutAROS_CLASS;

BOOL AboutAROS_Initialize()
{
    AboutAROS_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_Application, NULL, 
        sizeof(struct AboutAROS_DATA), AboutAROS_Dispatcher
    );

    return AboutAROS_CLASS != NULL ? TRUE : FALSE;
}

void AboutAROS_Deinitialize()
{
    MUI_DeleteCustomClass(AboutAROS_CLASS);
}
