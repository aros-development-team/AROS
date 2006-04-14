/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/alib.h>

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

#define ARG_TEMPLATE    "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N,PUBSCREEN/K"

enum {
    ARG_LEFT = 0,
    ARG_TOP,
    ARG_WIDTH,
    ARG_HEIGHT,
    ARG_PUBSCREEN,
    NUM_ARGS
};

/*** Variables **************************************************************/
/* Options ******************************************************************/

static LONG optionLeft     = MUIV_Window_LeftEdge_Centered,
            optionTop      = MUIV_Window_TopEdge_Centered,
            optionWidth    = 150,
            optionHeight   = 150;
static STRPTR optionPubscr = NULL;

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
    
    FreeVec(optionPubscr);
    Locale_Deinitialize();
    
    if (message != NULL)
        exit(20);
    else
        exit(0);
}

static void GetArguments(int argc, char **argv)
{
    if (argc == 0) /* started from WB */
    {
	UBYTE **array = ArgArrayInit(argc, (UBYTE **)argv);
	optionLeft = ArgInt(array, "LEFT", optionLeft);
	optionTop  = ArgInt(array, "TOP", optionTop);
	optionWidth = ArgInt(array, "WIDTH", optionWidth);
	optionHeight = ArgInt(array, "HEIGHT", optionHeight);
	optionPubscr = StrDup(ArgString(array, "PUBSCREEN", optionPubscr));
	ArgArrayDone();
    }
    else
    {
#       define TMPSIZE 256
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

	if (args[ARG_LEFT])   optionLeft   = *(LONG*)args[ARG_LEFT];
	if (args[ARG_TOP])    optionTop    = *(LONG*)args[ARG_TOP];
	if (args[ARG_WIDTH])  optionWidth  = *(LONG*)args[ARG_WIDTH];
	if (args[ARG_HEIGHT]) optionHeight = *(LONG*)args[ARG_HEIGHT];
	if (args[ARG_HEIGHT]) optionPubscr = StrDup((STRPTR)args[ARG_PUBSCREEN]);

	FreeArgs(rdargs);
#       undef TMPSIZE
    }
    D(bug("Clock left %d top %d width %d height %d pubscr %s\n",
		optionLeft, optionTop, optionWidth, optionHeight, optionPubscr));
}

int main(int argc, char **argv)
{
    Locale_Initialize();
    GetArguments(argc, argv);
    
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
            MUIA_Window_PublicScreen,    (IPTR) optionPubscr,
            WindowContents, (IPTR) ClockObject,
            End,
        End),
    End;

    if (application)
    {
        DoMethod
        ( 
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) application, 2, 
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        
        SetAttrs(window, MUIA_Window_Open, TRUE, TAG_DONE);
        
	DoMethod(application, MUIM_Application_Execute);
        
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
