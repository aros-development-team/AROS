/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

#define ARG_TEMPLATE    "FROM,EDIT/S,USE/S,SAVE/S,MAP/K,PUBSCREEN/K"

#define ARG_FROM        0
#define ARG_EDIT    	1
#define ARG_USE     	2
#define ARG_SAVE      	3
#define ARG_MAP     	4
#define ARG_PUBSCREEN   5

#define NUM_ARGS        6

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
} libtable[] =
{
    {&IntuitionBase     , "intuition.library"           , 39    },
    {&GfxBase           , "graphics.library"            , 39    },
    {&GadToolsBase      , "gadtools.library"            , 39    },
    {&UtilityBase       , "utility.library"             , 39    },
    {&IFFParseBase      , "iffparse.library"            , 39    },
    {NULL                                                       }
};

static struct RDArgs    *myargs;
static IPTR             args[NUM_ARGS];

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
   
    return EasyRequestArgs(win, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("Locale", msg, MSG(MSG_OK));     
	} else {
	    printf("Locale: %s\n", msg);
	}
    }
    
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
	    sprintf(s, MSG(MSG_CANT_OPEN_LIB), li->name, li->version);
	    Cleanup(s);
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
}

/*********************************************************************************************/

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

/*********************************************************************************************/

static void GetVisual(void)
{
    scr = LockPubScreen((CONST_STRPTR)args[ARG_PUBSCREEN]);
    if (!scr) Cleanup(MSG(MSG_CANT_LOCK_SCR));
    
    dri = GetScreenDrawInfo(scr);
    if (!dri) Cleanup(MSG(MSG_CANT_GET_DRI));
    
    vi = GetVisualInfoA(scr, NULL);
    if (!vi) Cleanup(MSG(MSG_CANT_GET_VI));
}

/*********************************************************************************************/

static void FreeVisual(void)
{
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
}

/*********************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    struct MenuItem     *item;
    UWORD               men;
    BOOL                quitme = FALSE;
    
    while (!quitme)
    {
	WaitPort(win->UserPort);
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_VANILLAKEY:
		    switch(msg->Code)
		    {
			case 27: /* ESC */
			    quitme = TRUE;
			    break;
			    
		    } /* switch(msg->Code) */
		    break;

		case IDCMP_MENUPICK:
		    men = msg->Code;            
		    while(men != MENUNULL)
		    {
			if ((item = ItemAddress(menus, men)))
			{
			    switch((ULONG)GTMENUITEM_USERDATA(item))
			    {
    	    	    	    	default:
				    break;
				
			    } /* switch(GTMENUITEM_USERDATA(item)) */
			    
			    men = item->NextSelect;
			} else {
			    men = MENUNULL;
			}
			
		    } /* while(men != MENUNULL) */
		    break;
		    		
	    } /* switch (msg->Class) */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while (!quitme) */
}

/*********************************************************************************************/

int main(void)
{
    InitLocale("Sys/localeprefs.catalog", 1);
    InitMenus();
    OpenLibs();
    GetArguments();
    GetVisual();
    MakeMenus();
    //HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


