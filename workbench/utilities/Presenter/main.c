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
        SubWindow, window = WindowObject,
            MUIA_Window_Title,    "Presenter",
            MUIA_Window_Activate, TRUE,
            
            WindowContents, ImageSequenceObject,
            End,
        End,
    End;

    if( application )
    {
        ULONG signals = 0;
        
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            application, 2, 
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        
        SetAttrs( window, MUIA_Window_Open, TRUE, TAG_DONE );
        
        while
        ( 
               DoMethod( application, MUIM_Application_NewInput, &signals ) 
            != MUIV_Application_ReturnID_Quit
        )
        {
            if( signals )
            {
                signals = Wait( signals | SIGBREAKF_CTRL_C );
                if( signals & SIGBREAKF_CTRL_C ) break;
            }
        }
        
        SetAttrs( window, MUIA_Window_Open, FALSE, TAG_DONE );
        MUI_DisposeObject( application );
    }
    
    ImageSequence_Destroy();

    return 0;
}
