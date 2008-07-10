/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/


#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>


#include <linklibs/coolimages.h>

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/iobsolete.h>
#include <libraries/gadtools.h>

#include <libraries/mui.h>
#include <zune/systemprefswindow.h>

#include <prefs/serial.h>

#include "locale.h"
#include "global.h"
#include "sereditor.h"

/* #define DEBUG 1 */
#include <aros/debug.h>

#define VERSION "$VER: Serial 2.0 (09.06.2008) AROS Dev Team"
/*********************************************************************************************/


#define ARG_FROM        0
#define ARG_EDIT    	1
#define ARG_USE     	2
#define ARG_SAVE      	3
#define ARG_MAP     	4
#define ARG_PUBSCREEN   5

#define NUM_ARGS        6

/*********************************************************************************************/

STATIC CONST_STRPTR   TEMPLATE=(CONST_STRPTR) "FROM,EDIT/S,USE/S,SAVE/S,MAP/K,PUBSCREEN/K";
static struct RDArgs  *myargs;
static IPTR           args[NUM_ARGS];

/*********************************************************************************************/

#define NUM_BUTTONS 2

/*********************************************************************************************/

/*
 * safe (?) error display
 */

VOID ShowMsg(char *msg)
{
    struct EasyStruct es;

    if (msg)
    {
	if (IntuitionBase)
	{
	    es.es_StructSize   = sizeof(es);
	    es.es_Flags        = 0;
	    es.es_Title        = (CONST_STRPTR) "Serial";
	    es.es_TextFormat   = (CONST_STRPTR) msg;
	    es.es_GadgetFormat = MSG(MSG_OK);
   
	    EasyRequestArgs(NULL, &es, NULL, NULL); /* win=NULL -> wb screen */
	} else {
	    printf("Serial: %s\n", msg);
	}
    }
}

/*********************************************************************************************/

STATIC ULONG GetArguments(void)
{
    char buf[256];
    if (!(myargs = ReadArgs(TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), NULL, (STRPTR) buf, 255);
	ShowMsg(buf);
	return 0;
    }
    
    if (!args[ARG_FROM]) args[ARG_FROM] = (IPTR)CONFIGNAME_ENV;

    return 1;
}

/*********************************************************************************************/

STATIC VOID FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

    
int main(void)
{
    Object *application;
    Object *window;

    D(bug("[serial prefs] InitLocale\n"));
    InitLocale();

    D(bug("[serial prefs] started\n"));

    /* init */
    if( GetArguments() &&
	InitPrefs((STRPTR)args[ARG_FROM], (args[ARG_USE] ? TRUE : FALSE), (args[ARG_SAVE] ? TRUE : FALSE)) )
    {
    
	D(bug("[serial prefs] initialized\n"));

	application = ApplicationObject,
			MUIA_Application_Title, MSG(MSG_WINTITLE),
			MUIA_Application_Version, (IPTR) VERSION,
			MUIA_Application_Description, MSG(MSG_WINTITLE),
			MUIA_Application_Base, (IPTR) "SERIALPREF",
			SubWindow, (IPTR) (window = SystemPrefsWindowObject,
			  //MUIA_Window_ID, MAKE_ID('S','O','1','I'),
			  MUIA_Window_ID, ID_SERL,
			  WindowContents, (IPTR) SerEditorObject,
			  TAG_DONE),
			End),
		      End;
	
	if (application != NULL && window != NULL)
	{
	    SET(window, MUIA_Window_Open, TRUE);
	    DoMethod(application, MUIM_Application_Execute);
	    SET(window, MUIA_Window_Open, FALSE);

	    MUI_DisposeObject(application);
	}
    } /* if init */

    FreeArguments();
    CleanupLocale();
    return 0;
}

/*********************************************************************************************/


