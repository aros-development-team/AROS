/*
    Copyright � 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <aros/debug.h>
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

#include <string.h>

#include "aboutaros.h"
#include "locale.h"
#include "logotype.h"

#include "authors.h"
#include "sponsors.h"
#include "acknowledgements.h"

#define WINDOW_BG   ((IPTR) "2:00000000,00000000,00000000")
#define REGISTER_BG ((IPTR) "7:V,00000000,92000000,91000000-00000000,82000000,81000000")
#define LIST_BG     ((IPTR) "2:00000000,82000000,81000000")
          
/*** Private methods ********************************************************/
#define MUIM_AboutAROS_ShowLicense (TAG_USER | 0x20000000)

/*** Instance data **********************************************************/
struct AboutAROS_DATA
{
    Object *aad_Window;
    APTR    aad_Pool;
};

/*** Utility functions ******************************************************/
STRPTR Section2Name(ULONG section)
{
    switch (section)
    {
        case SID_COORDINATION:
            return _(MSG_SECTION_COORDINATION);
        
        case SID_EVANGELISM:
            return _(MSG_SECTION_EVANGELISM);
        
        case SID_HIDD:
            return _(MSG_SECTION_HIDD);
        
        case SID_INTUITION:
            return _(MSG_SECTION_INTUITION);
        
        case SID_GRAPHICS:
            return _(MSG_SECTION_GRAPHICS);
        
        case SID_SHELL_COMMANDS:
            return _(MSG_SECTION_SHELL_COMMANDS);
        
        case SID_WORKBENCH:
            return _(MSG_SECTION_WORKBENCH);
        
        case SID_TOOLS:
            return _(MSG_SECTION_TOOLS);
        
        case SID_PREFERENCES:
            return _(MSG_SECTION_PREFERENCES);
        
        case SID_BGUI:
            return _(MSG_SECTION_BGUI);
        
        case SID_ZUNE:
            return _(MSG_SECTION_ZUNE);
        
        case SID_KERNEL:
            return _(MSG_SECTION_KERNEL);
        
        case SID_DOS:
            return _(MSG_SECTION_DOS);
        
        case SID_LIBC_POSIX:
            return _(MSG_SECTION_LIBC_POSIX);
        
        case SID_DOCUMENTATION:
            return _(MSG_SECTION_DOCUMENTATION);
        
        case SID_TRANSLATION:
            return _(MSG_SECTION_TRANSLATION);
        
        case SID_ARTISTRY:
            return _(MSG_SECTION_ARTISTRY);
        
        case SID_WEBSITE:
            return _(MSG_SECTION_WEBSITE);
        
        default:
            return NULL;
    }
}

BOOL NamesToList
(
    Object *list, struct TagItem *tags, struct AboutAROS_DATA *data
)
{
    struct TagItem *tstate       = tags,
                   *tag          = NULL;
    BOOL            success      = TRUE;
    IPTR            section      = SID_NONE;
    STRPTR          sectionName  = NULL;
    BOOL            sectionFirst = TRUE;
    STRPTR          name;
    STRPTR          buffer;
    ULONG           length       = 0;
    
    if (tags == NULL) return FALSE;
    
    while ((tag = NextTagItem(&tstate)) != NULL && success == TRUE)
    {
        switch (tag->ti_Tag)
        {
            case SECTION_ID:
                section     = tag->ti_Data;
                sectionName = Section2Name(section);
                
                if (sectionName != NULL)
                {
                    sectionFirst 
                        ? sectionFirst = FALSE
                        : DoMethod
                          (
                              list, MUIM_List_InsertSingle, (IPTR) "", 
                              MUIV_List_Insert_Bottom
                          );
                    
                    length = strlen(MUIX_B) + strlen(sectionName) + 1;
                    buffer = AllocPooled(data->aad_Pool, length);
                    if (buffer != NULL)
                    {
                        buffer[0] = '\0';
                        strcat(buffer, MUIX_B);
                        strcat(buffer, sectionName);
                        
                        DoMethod
                        (
                            list, MUIM_List_InsertSingle, 
                            (IPTR) buffer, MUIV_List_Insert_Bottom
                        );
                    }
                    else
                    {
                        success = FALSE;
                        break;
                    }
                }
                
                break;
                
            case NAME_STRING:
                name   = (STRPTR) tag->ti_Data;
                
                length = strlen(name) + 1;
                if (sectionName != NULL) length += 4;
                
                buffer = AllocPooled(data->aad_Pool, length);
                if (buffer != NULL)
                {
                    buffer[0] = '\0';
                    if (sectionName != NULL) strcat(buffer, "    ");
                    strcat(buffer, name);
                    
                    DoMethod
                    (
                        list, MUIM_List_InsertSingle, 
                        (IPTR) buffer, MUIV_List_Insert_Bottom
                    );
                }
                else
                {
                    success = FALSE;
                    break;
                }
                
                break;
        }
    }
    
    return success;
}


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
                          *acknowledgementsList;
    
    STRPTR                 pages[4]       = { NULL };
    BOOL                   showLogotype;
    BPTR                   lock;
    APTR                   pool;

    /* Allocate memory pool ------------------------------------------------*/
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    if (pool == NULL) return NULL;
        
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
                        Child, (IPTR) ListviewObject,
                            MUIA_Listview_List, (IPTR) acknowledgementsList = ListObject,
                                TextFrame,
                                MUIA_Background, LIST_BG,
                            End,
                        End,
                    End,
                End,
            End,
        End,
        
        TAG_DONE
    );
    
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->aad_Window = window;
    data->aad_Pool   = pool;
    
    /*-- Initialize lists --------------------------------------------------*/
    NamesToList(authorsList, AUTHORS, data);
    NamesToList(sponsorsList, SPONSORS, data);
    
    DoMethod
    (
        acknowledgementsList, MUIM_List_Insert,
        (IPTR) ACKNOWLEDGEMENTS, ACKNOWLEDGEMENTS_SIZE, MUIV_List_Insert_Top
    );
    
    /*-- Setup notifications -----------------------------------------------*/
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

IPTR AboutAROS__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message 
)
{
    struct AboutAROS_DATA *data   = INST_DATA(CLASS, self);
    
    if (data->aad_Pool != NULL) DeletePool(data->aad_Pool);
    
    return DoSuperMethodA(CLASS, self, message);
}


IPTR AboutAROS__MUIM_Application_Execute
(     
    Class *CLASS, Object *self, Msg message 
)
{
    struct AboutAROS_DATA *data = INST_DATA(CLASS, self);
    
    SET(data->aad_Window, MUIA_Window_Open, TRUE);
    DoSuperMethodA(CLASS, self, message);
    SET(data->aad_Window, MUIA_Window_Open, FALSE);

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
        
        case OM_DISPOSE:
            return AboutAROS__OM_DISPOSE(CLASS, self, message);
        
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
