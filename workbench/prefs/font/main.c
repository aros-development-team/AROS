#define MUIMASTER_YES_INLINE_STDARG

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

#define DEBUG 1
#include <aros/debug.h>

CONST_STRPTR version = "$VER: Font 0.14 (14.1.2002)";

extern struct FontPrefs *fontPrefs[3];	// prefs.c
extern struct RDArgs *readArgs; // args.c

void quitApp(STRPTR errorMsg, UBYTE errorCode)
{    
    D(bug("*** Exiting...\n"));
    
    if (errorMsg != NULL)
    {
        ShowError(errorMsg);
    }
    
    Prefs_Deinitialize();
    Locale_Deinitialize();
    
    if(readArgs != NULL) FreeArgs(readArgs);
    
    exit(errorCode);
}


int main( void )
{
    Object *application,  *window;
    
    if (!Locale_Initialize()) return 20;
    if (!Prefs_Initialize()) return 20;
    if (!FPWindow_Initialize()) return 20;
    
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
        SubWindow, window = FPWindowObject,
        EndBoopsi,
    End;

    if (application != NULL)
    {
        ULONG signals = 0;
                
        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);
        
        while
        ( 
               DoMethod(application, MUIM_Application_NewInput, &signals) 
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

    FPWindow_Deinitialize();
    
    quitApp(NULL, RETURN_OK);
    
    return 0;
}
