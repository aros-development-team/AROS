/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <libraries/coolimages.h>

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
    {&GadToolsBase      , "gadtools.library" 	 , 39, TRUE  },
    {&UtilityBase       , "utility.library"  	 , 39, TRUE  },
    {&IFFParseBase      , "iffparse.library" 	 , 39, TRUE  },
    {&CyberGfxBase  	, "cybergraphics.library", 39, FALSE },
    {&MUIMasterBase 	, "muimaster.library"	 , 0 , TRUE  },
    {NULL                                            	     }
};

/*********************************************************************************************/

static struct RDArgs        	*myargs;
static IPTR                 	args[NUM_ARGS];

static STRPTR monthlabels[] =
{
    "January",
    "Frebruary",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
    NULL
};

/*********************************************************************************************/

static void CloseLibs(void);
static void CloseTimerDev(void);
static void FreeArguments(void);
static void FreeVisual(void);
static void KillGUI(void);

/*********************************************************************************************/

WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(win, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("Time", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("Time: %s\n", msg);
	}
    }
    
    KillGUI();
    FreeVisual();
    FreeArguments();
    CloseTimerDev();
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

static void OpenTimerDev(void)
{
    if ((TimerMP = CreateMsgPort()))
    {
    	if ((TimerIO = (struct timerequest *)CreateIORequest(TimerMP, sizeof(struct timerequest))))
	{
	    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)TimerIO, 0);
	}
    }
}

/*********************************************************************************************/

static void CloseTimerDev(void)
{
    if (TimerIO)
    {
    	CloseDevice((struct IORequest *)TimerIO);
	DeleteIORequest((struct IORequest *)TimerIO);
    }
    
    if (TimerMP)
    {
    	DeleteMsgPort(TimerMP);
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
    
    // if (!args[ARG_FROM]) args[ARG_FROM] = (IPTR)CONFIGNAME_ENV;
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

static void MakeGUI(void)
{
    extern struct NewMenu nm;
    Object *menu;
    
    if (LocaleBase)
    {
    	struct Locale *locale = OpenLocale(NULL);
	
	if (locale)
	{
	    WORD i;
	    
	    for(i = 0; i < 12; i++)
	    {
	    	monthlabels[i] = GetLocaleStr(locale, MON_1 + i);
	    }
	}
	
    }
    
    menu = MUI_MakeObject(MUIO_MenustripNM, &nm, 0);
        
    app = ApplicationObject,
	MUIA_Application_Title, (IPTR)"Time",
	MUIA_Application_Version, (IPTR)VERSIONSTR,
	MUIA_Application_Copyright, (IPTR)"Copyright © 1995-2002, The AROS Development Team",
	MUIA_Application_Author, (IPTR)"The AROS Development Team",
	MUIA_Application_Description, (IPTR)MSG(MSG_WINTITLE),
	MUIA_Application_Base, (IPTR)"Time",
	menu ? MUIA_Application_Menustrip : TAG_IGNORE, menu,
  	SubWindow, wnd = WindowObject,
	    MUIA_Window_Title, (IPTR)MSG(MSG_WINTITLE),
	    MUIA_Window_ID, MAKE_ID('T','W','I','N'),
	    WindowContents, VGroup,
	    	Child, HGroup,
		    MUIA_Group_SameWidth, TRUE,
		    Child, VGroup,
		    	GroupFrame,
			Child, HGroup,
		    	    Child, MUI_MakeObject(MUIO_Cycle, NULL, monthlabels),
			    Child, HVSpace,
			    Child, StringObject,
				StringFrame,
				End,
			    End,
			End,
		    Child, VGroup,
		    	GroupFrame,
			Child, HVSpace,
		    	End,
		    End,
		Child, HGroup,
		    MUIA_FixHeight, 1,
		    MUIA_Group_SameWidth, TRUE,
		    Child, CoolImageIDButton(MSG(MSG_GAD_SAVE), COOL_SAVEIMAGE_ID),
		    Child, CoolImageIDButton(MSG(MSG_GAD_USE), COOL_USEIMAGE_ID),
		    Child, CoolImageIDButton(MSG(MSG_GAD_CANCEL), COOL_CANCELIMAGE_ID),
		    End,
		End,
	    End,
	End;
	
    if (!app) Cleanup(MSG(MSG_CANT_CREATE_APP));

    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(wnd, MUIM_Notify, MUIA_Window_MenuAction, MSG_MEN_PROJECT_QUIT, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/*********************************************************************************************/

static void KillGUI(void)
{
    DisposeObject(app);
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs = 0;
    
    set (wnd, MUIA_Window_Open, TRUE);
    
    while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
    {
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
    InitLocale("Sys/timeprefs.catalog", 1);
    InitMenus();
    OpenLibs();
    OpenTimerDev();
    GetArguments();
    GetVisual();
    MakeGUI();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


