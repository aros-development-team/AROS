/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <libraries/mui.h>
#include <zune/clock.h>

#include <stdlib.h> /* for exit() */

#include "locale.h"

#define ARG_TEMPLATE    "DIGITAL/S,LEFT/N,TOP/N,WIDTH/N,HEIGHT/N," \
    	    	    	"24HOUR/S,SECONDS/S,DATE/S,FORMAT/N,PUBSCREEN/K"

#define ARG_DIGITAL      0
#define ARG_LEFT    	 1
#define ARG_TOP     	 2
#define ARG_WIDTH     	 3
#define ARG_HEIGHT  	 4
#define ARG_24HOUR  	 5
#define ARG_SECONDS   	 6
#define ARG_DATE    	 7
#define ARG_FORMAT    	 8
#define ARG_PUBSCREEN    9

#define NUM_ARGS        10

static WORD           opt_winleft   = MUIV_Window_LeftEdge_Centered,
                      opt_wintop    = MUIV_Window_TopEdge_Centered,
                      opt_winwidth  = 150,
                      opt_winheight = 150;

char s[256];
BYTE    	    	    	opt_analogmode;
BYTE    	    	    	opt_showdate;
BYTE    	    	    	opt_showsecs;
BYTE    	    	    	opt_alarm;
BYTE    	    	    	opt_format;
BYTE    	    	    	opt_24hour;

static struct RDArgs *myargs;
static IPTR           args[NUM_ARGS];


static void FreeArguments(void);


WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return 0; //EasyRequestArgs(win, &es, NULL, NULL);  
}

void Cleanup( STRPTR msg )
{
    if( msg != NULL )
    {
	if
        ( 
               IntuitionBase != NULL 
            && ((struct Process *) FindTask( NULL ))->pr_CLI == NULL)
	{
	    ShowMessage( "Clock", msg, MSG(MSG_OK) );     
	}
	else
	{
	    Printf( "Clock: %s\n", msg );
	}
    }
    
    FreeArguments();
    CleanupLocale();
    
    if( msg != NULL )
        exit( 20 );
    else
        exit( 0 );
}

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), 0, s, 256);
	Cleanup(s);
    }

    if (args[ARG_DIGITAL] == 0) opt_analogmode = 1;
    if (args[ARG_24HOUR]) opt_24hour = 1;
    if (args[ARG_SECONDS]) opt_showsecs = 1;
    if (args[ARG_DATE]) opt_showdate = 1;
    
    if (args[ARG_LEFT]) opt_winleft = *(IPTR *)args[ARG_LEFT];
    if (args[ARG_TOP]) opt_wintop = *(IPTR *)args[ARG_TOP];
    if (args[ARG_WIDTH]) opt_winwidth = *(IPTR *)args[ARG_WIDTH];
    if (args[ARG_HEIGHT]) opt_winheight = *(IPTR *)args[ARG_HEIGHT];
}

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

int main(void)
{
    Object *application, *window;
    
    InitLocale("Sys/clock.catalog", 1);
    GetArguments();
    
    application = ApplicationObject,
        SubWindow, window = WindowObject,
            MUIA_Window_Title,       MSG( MSG_WINTITLE ),
            MUIA_Window_Activate,    TRUE,
            MUIA_Window_NoMenus,     TRUE,
            MUIA_Window_LeftEdge,    opt_winleft,
            MUIA_Window_TopEdge,     opt_wintop,
            MUIA_Window_Width,       opt_winwidth,
            MUIA_Window_Height,      opt_winheight,
            
            WindowContents, ClockObject,
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

    
    Cleanup(NULL);
    
    return 0;
}
