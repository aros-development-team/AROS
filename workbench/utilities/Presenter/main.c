/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#include <dos/dos.h>
#include <libraries/mui.h>

#include "imagesequence.h"

int main( void )
{
    Object *application, *window;
    
    ImageSequence_Create();
    
    application = ApplicationObject,
        SubWindow, (IPTR) window = WindowObject,
            MUIA_Window_Title,    (IPTR) "Presenter",
            MUIA_Window_Activate,        TRUE,
            
            WindowContents, (IPTR) ImageSequenceObject,
            End,
        End,
    End;

    if( application )
    {
        ULONG signals = 0L;
        
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) application, 2, 
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        
        SET(window, MUIA_Window_Open, TRUE);
        
        while
        ( 
               DoMethod( application, MUIM_Application_NewInput, (IPTR) &signals ) 
            != MUIV_Application_ReturnID_Quit
        )
        {
            if( signals )
            {
                signals = Wait( signals | SIGBREAKF_CTRL_C );
                if( signals & SIGBREAKF_CTRL_C ) break;
            }
        }
        
        SET(window, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(application);
    }
    
    ImageSequence_Destroy();

    return 0;
}
