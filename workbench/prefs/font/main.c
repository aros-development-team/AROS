#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 1

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>
#include <prefs/font.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <stdlib.h>

#include "locale.h"
#include "args.h"
#include "prefs.h"
#include "misc.h"
#include "gui.h"
#include "version.h"

#include <aros/debug.h>

CONST_STRPTR versionString = VERSIONSTR;

extern struct RDArgs *readArgs; // args.c

/*** Prototypes *************************************************************/
BOOL Initialize(void);
void Deinitialize(void);



void quitApp(STRPTR errorMsg, UBYTE errorCode)
{    
    D(bug("*** Exiting...\n"));
    
    if (errorMsg != NULL)
    {
        ShowError(errorMsg);
    }
    
    Deinitialize();
    
    exit(errorCode);
}

BOOL Initialize(void)
{
    Locale_Initialize();
    if (!FP_Initialize()) goto error;
    if (!FPWindow_Initialize()) goto error;

    return TRUE;

error:
    return FALSE;
}

void Deinitialize(void)
{
    FPWindow_Deinitialize();
    FP_Deinitialize();
    Locale_Deinitialize();

    if(readArgs != NULL) FreeArgs(readArgs);
}


int main( void )
{
    Object *application,  *window;

    if (!Initialize()) goto error;
    
    switch (processArguments())
    {
        case APP_STOP:
            quitApp(NULL, RETURN_OK);
            break;

        case APP_FAIL:
            quitApp(NULL, RETURN_FAIL);
            break;
    }
    
    application = ApplicationObject,
        SubWindow, (IPTR)window = FPWindowObject,
        End,
    End;

    if (application != NULL)
    {
        ULONG signals = 0;
                
        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);
        
        while
        ( 
               DoMethod(application, MUIM_Application_NewInput, (IPTR) &signals) 
            != MUIV_Application_ReturnID_Quit
        )
        {
            if(signals)
            {
                signals = Wait(signals | SIGBREAKF_CTRL_C);
                if(signals & SIGBREAKF_CTRL_C) break;
            }
        }
        
        SetAttrs(window, MUIA_Window_Open, FALSE, TAG_DONE);
        MUI_DisposeObject(application);
    }

    quitApp(NULL, RETURN_OK);

error:
    quitApp("Initialization failed.", RETURN_FAIL);
    
    return 0; /* Never reached */
}
