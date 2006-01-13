/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <zune/clock.h>

#include <string.h> /* memset() */
#include <stdlib.h> /* exit() */

#include "locale.h"
#include "version.h"

/*** Version string *********************************************************/

char *versionString = VERSIONSTR;

/*** Argument parsing *******************************************************/

#define ARG_TEMPLATE    "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N"

#define ARG_LEFT    	 0
#define ARG_TOP     	 1
#define ARG_WIDTH     	 2
#define ARG_HEIGHT  	 3

#define NUM_ARGS         4

/*** Variables **************************************************************/
/* Options ******************************************************************/

static IPTR optionLeft   = MUIV_Window_LeftEdge_Centered,
            optionTop    = MUIV_Window_TopEdge_Centered,
            optionWidth  = 150,
            optionHeight = 150;

/* User interface ***********************************************************/
Object *application, *window;


/*** Functions **************************************************************/

void ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadgets)
{
    MUI_RequestA(application, window, 0, title, gadgets, text, NULL);
}

void Cleanup(CONST_STRPTR message)
{
    if(message != NULL)
    {
	if
        ( 
               IntuitionBase != NULL 
            && ((struct Process *) FindTask(NULL))->pr_CLI == NULL
        )
	{
	    ShowMessage("Clock", message, MSG(MSG_OK));     
	}
	else
	{
	    Printf("Clock: %s\n", message);
	}
    }
    
    Locale_Deinitialize();
    
    if (message != NULL)
        exit(20);
    else
        exit(0);
}

static void GetArguments(void)
{
#   define TMPSIZE 256
    TEXT           tmp[TMPSIZE];
    struct RDArgs *rdargs = NULL;
    IPTR           args[NUM_ARGS];
    
    memset(args, 0, sizeof(args));
    rdargs = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (rdargs == NULL)
    {
	Fault(IoErr(), 0, tmp, TMPSIZE);
	Cleanup(tmp);
    }

    if (args[ARG_LEFT])   optionLeft   = *(IPTR *) args[ARG_LEFT];
    if (args[ARG_TOP])    optionTop    = *(IPTR *) args[ARG_TOP];
    if (args[ARG_WIDTH])  optionWidth  = *(IPTR *) args[ARG_WIDTH];
    if (args[ARG_HEIGHT]) optionHeight = *(IPTR *) args[ARG_HEIGHT];
        
    FreeArgs(rdargs);
#   undef TMPSIZE 
}

int main(void)
{
    Locale_Initialize();
    GetArguments();
    
    application = ApplicationObject,
        MUIA_Application_Title, (IPTR) MSG(MSG_WINDOW_TITLE),
        MUIA_Application_Version, (IPTR) versionString,
        MUIA_Application_Copyright, (IPTR)"© 2006, The AROS Development Team",
        MUIA_Application_Description, (IPTR) MSG(MSG_DESCRIPTION),
        SubWindow, (IPTR) (window = WindowObject,
            MUIA_Window_Title,    (IPTR) MSG(MSG_WINDOW_TITLE),
            MUIA_Window_Activate,        TRUE,
            MUIA_Window_NoMenus,         TRUE,
            MUIA_Window_LeftEdge,        optionLeft,
            MUIA_Window_TopEdge,         optionTop,
            MUIA_Window_Width,           optionWidth,
            MUIA_Window_Height,          optionHeight,
            
            WindowContents, (IPTR) ClockObject,
            End,
        End),
    End;

    if (application)
    {
        ULONG signals = 0;
        
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) application, 2, 
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        
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
    else
    {
        Cleanup(MSG(MSG_ERROR_CANT_CREATE_APPLICATION));
    }
    
    Cleanup(NULL);
    
    return 0;
}
