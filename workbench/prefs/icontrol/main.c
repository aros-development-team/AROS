/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <libraries/coolimages.h>
#include <aros/debug.h>

#include <zune/clock.h>
#include <zune/calendar.h>

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

#define ARG_TEMPLATE    "FROM,EDIT/S,USE/S,SAVE/S,PUBSCREEN/K"

#define ARG_FROM        0
#define ARG_EDIT    	1
#define ARG_USE     	2
#define ARG_SAVE      	3
#define ARG_PUBSCREEN   4

#define NUM_ARGS        5

#define RETURNID_USE 	1
#define RETURNID_SAVE 	2

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
    BOOL    	required;
}
libtable[] =
{
    {&IntuitionBase     , "intuition.library"	 , 39, TRUE  },
    {&GfxBase           , "graphics.library" 	 , 40, TRUE  }, /* 40, because of WriteChunkyPixels */
    {&UtilityBase       , "utility.library"  	 , 39, TRUE  },
    {&IFFParseBase      , "iffparse.library" 	 , 39, TRUE  },
    {&MUIMasterBase 	, "muimaster.library"	 , 0 , TRUE  },
    {NULL                                            	     }
};

/*********************************************************************************************/

static struct RDArgs        	*myargs;
static IPTR                 	args[NUM_ARGS];

/*********************************************************************************************/

static void CloseLibs(void);
static void FreeArguments(void);
static void FreeVisual(void);

/*********************************************************************************************/

WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(NULL, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("IControl", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("IControl: %s\n", msg);
	}
    }
    
    KillGUI();
    FreeVisual();
    FreeArguments();
    CloseLibs();
    CleanupLocale();
    
    exit(prog_exitcode);
}


/*********************************************************************************************/

static void OpenLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (!((*(struct Library **)li->var) = OpenLibrary(li->name, li->version)))
	{
	    if (li->required)
	    {
	    	sprintf(s, MSG(MSG_CANT_OPEN_LIB), li->name, li->version);
	    	Cleanup(s);
	    }
	}       
    }
       
}

/*********************************************************************************************/

static void CloseLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (*(struct Library **)li->var) CloseLibrary((*(struct Library **)li->var));
    }
}

/*********************************************************************************************/

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), 0, s, 256);
	Cleanup(s);
    }
    
    if (!args[ARG_FROM]) args[ARG_FROM] = (IPTR)CONFIGNAME_ENV;
}

/*********************************************************************************************/

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

/*********************************************************************************************/

static void GetVisual(void)
{
}

/*********************************************************************************************/

static void FreeVisual(void)
{
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs = 0;
    LONG returnid;
    
    set (wnd, MUIA_Window_Open, TRUE);
    
    for(;;)
    {
    	returnid = (LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs);

	if ((returnid == MUIV_Application_ReturnID_Quit) ||
	    (returnid == RETURNID_SAVE) || (returnid == RETURNID_USE)) break;
	
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C);
	    if (sigs & SIGBREAKF_CTRL_C) break;
	}
    }

}

/*********************************************************************************************/

int main(void)
{
    InitLocale("System/Prefs/IControl.catalog", 1);
    InitMenus();
    OpenLibs();
    GetArguments();
    InitPrefs((STRPTR)args[ARG_FROM], (args[ARG_USE] ? TRUE : FALSE), (args[ARG_SAVE] ? TRUE : FALSE));
    GetVisual();
    MakeGUI();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


