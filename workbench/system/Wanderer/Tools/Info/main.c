/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <workbench/startup.h>

#include <zune/iconimage.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    Object *application, *window;
    struct WBStartup *startup;
    BPTR lock;
    STRPTR name;
    BPTR cd;
    
    if (argc != 0) printf("Error: Cannot be started from shell\n");
    
    startup = (struct WBStartup *) argv;
    kprintf("*** %d\n", startup->sm_NumArgs);
    
    if (startup->sm_NumArgs < 2) return 20; /* need atleast 1 arg */
    
    lock = startup->sm_ArgList[1].wa_Lock;
    name = startup->sm_ArgList[1].wa_Name;
    cd = CurrentDir(lock);
    if (name == NULL)
    {
        /* directory */
        name = "[directory non comprende]";
    }
    
    application = ApplicationObject,
        SubWindow, window = WindowObject,
            MUIA_Window_Title,       (IPTR) "Information",
            MUIA_Window_Activate,           TRUE,
            
            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        GroupFrameT("Icon"),
                        Child, IconImageObject,
                            MUIA_InputMode,             MUIV_InputMode_Toggle,
                            MUIA_IconImage_File, (IPTR) name,
                        End,
                    End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) ColGroup(2),
                            Child, Label2("Name:"),
                            Child, (IPTR) TextObject,
                                TextFrame,
                                MUIA_Background,           MUII_TextBack,
                                MUIA_Text_Contents, (IPTR) name,
                            End,
                        End,
                        Child, HVSpace,
                    End,
                End,
                Child, (IPTR) ColGroup(2),
                    Child, Label2("Comment"),
                    Child, (IPTR) StringObject,
                        StringFrame,
                        MUIA_Disabled, TRUE,
                    End,
                End,
            End,
        End,
    End;
    
    CurrentDir(cd);
    
    if (application != NULL)
    {
        ULONG signals = 0;
        
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        
        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);
        
        while
        ( 
               DoMethod(application, MUIM_Application_NewInput, (IPTR) &signals) 
            != MUIV_Application_ReturnID_Quit
        )
        {
            if (signals)
            {
                signals = Wait(signals | SIGBREAKF_CTRL_C);
                if(signals & SIGBREAKF_CTRL_C) break;
            }
        }
        
        SetAttrs(window, MUIA_Window_Open, FALSE, TAG_DONE);
        MUI_DisposeObject(application);
    }

    return 0;
}
