/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* gui_zune.c -- here are all functions for the ZUNE gui */

#include "Installer.h"
#include "cleanup.h"
#include "execute.h"
#include "texts.h"
#include "more.h"
#include "misc.h"
#include "gui.h"
#include "variables.h"

/* External variables */
extern BPTR inputfile;
extern char buffer[MAXARGSIZE];
extern char *filename;
extern InstallerPrefs preferences;
extern int error, grace_exit;
extern int doing_abort;

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <libraries/coolimages.h>

/* Zune Macros */
#define HBar(x)		MUI_MakeObject(MUIO_HBar,x)

Object *app;
Object *wnd;
Object *root;
Object *btproceed, *btabort, *btskip, *bthelp;

enum
{
Push_NULL,
Push_Proceed,
Push_Abort,
Push_Skip,
Push_Help,
Push_About,
Push_Last
};

/* ######################################################################## */


const char GuiWinTitle[] ="AROS - Installer V43.3";
struct Screen *scr;


#define WINDOWWIDTH  400
#define WINDOWHEIGHT 250

/*
 * Initialize the GUI
 */
void init_gui( )
{

    scr = LockPubScreen( NULL );

    app = ApplicationObject,
	MUIA_Application_Title, "AROS - Installer",

   	SubWindow, wnd = WindowObject,
	    MUIA_Window_Title,	GuiWinTitle,
	    MUIA_Window_Width,	400,
	    MUIA_Window_Height,	300,
//	    MUIA_Window_CloseGadget,	FALSE,
	    MUIA_Window_ID,	MAKE_ID('A','I','N','S'),
	    WindowContents,
	    VGroup,
		Child, root = VGroup, End,
		Child, HBar(TRUE),
		Child, HGroup,
		MUIA_Group_SameSize, TRUE,
		Child, btproceed = CoolImageIDButton("Proceed"   ,COOL_USEIMAGE_ID),
		Child, btabort   = CoolImageIDButton("Abort",COOL_CANCELIMAGE_ID),
		Child, btskip    = CoolImageIDButton("Skip"  ,COOL_WARNIMAGE_ID),
		Child, bthelp    = CoolImageIDButton("Help"  ,COOL_INFOIMAGE_ID),
                 End,
	    End,
	End,
    End;
    if (app == NULL)
    {
	/* failed to initialize GUI */
#if DEBUG
kprintf("Failed to intialize Zune GUI\n");
#endif
    }
    DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
	MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(btproceed, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Proceed);
    DoMethod(btabort, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Abort);
    DoMethod(btskip, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Skip);
    DoMethod(bthelp, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Help);
    set(wnd, MUIA_Window_Open, TRUE);
}

/*
 * Close GUI
 */
void deinit_gui( )
{
    set(wnd, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
}


/*
 * Clean the GUI display
 */
void clear_gui()
{
}


/*
 * Show user that we are going to "(abort)" install
 * Don't confuse NOVICE...
 */
void show_abort( char *msg )
{
}


/*
 * Show user how much we have completed yet
 */
void show_complete( long int percent )
{
char *text;

  text = AllocVec( strlen( GuiWinTitle ) + 13, MEMF_PUBLIC );
  if( text == NULL )
  {
    end_alloc();
  }
  sprintf( text, "%s (Done %3ld%%)", GuiWinTitle, percent );
  set(wnd, MUIA_Window_Title, text);
  FreeVec(text);
}


/*
 * Show user that we "(exit)" the installation
 */
void show_exit( char *msg )
{
}


/*
 * Show the line which caused the parse-error
 */
void show_parseerror( char * msg, int errline )
{
}


/*
 * Tell user that some big task is to be done
 * "Be patient..."
 */
void show_working( char *msg )
{
}


/*
 * Display a "(message)" to the user
 * Don't confuse NOVICE unless "(all)" users want to get this info
 */
void show_message( char * msg ,struct ParameterList * pl )
{
}


/*
 * Show the help-window for topic: User-Level
 */
void show_help_userlevel( )
{
#warning TODO: help for userlevel-requester

  moremain( HELP_ON_USERLEVEL, USERLEVEL_HELP );
}


/*
 * Show the help-window for topic: Log-File
 */
void show_help_logfile()
{
char *helptext;

#warning TODO: help for logfile-requester
  helptext = AllocVec( 512 * sizeof(char), MEMF_PUBLIC );
  sprintf( helptext, LOG_HELP, preferences.transcriptfile );
  moremain( HELP_ON_LOGFILES, helptext );
  FreeVec(helptext);
}


/*
 * Show the help-window for topic: Pretend to install
 */
void show_help_pretend()
{
#warning TODO: help for pretend-requester
  moremain( HELP_ON_PRETEND, PRETEND_HELP );
}


/*
 * Show the help-window for topic: Installer
 */
void show_help_installer( )
{
char *helptext;

#warning TODO: help/about for Installer
  helptext = AllocVec( 512 * sizeof(char), MEMF_PUBLIC );
  sprintf( helptext, ABOUT_INSTALLER, INSTALLER_VERSION, INSTALLER_REVISION );
  moremain( ABOUT_ON_INSTALLER, helptext );
  FreeVec(helptext);
}


/*
 * Ask user for his user-level
 */
void request_userlevel( char *msg )
{
BOOL running = TRUE;
ULONG sigs = 0, val;
Object *wc;
Object *st;
Object *btabout;

    	wc = VGroup,
	    Child, VGroup, GroupFrameT(" Welcome to DemoApp Installation Utility!"),           
		Child, st  = StringObject,
		    MUIA_String_Contents,	"Welcome to AROS Installer",
		    MUIA_String_MaxLen,		80,
		End,
		Child, btabout    = CoolImageIDButton("About Installer"  ,COOL_ASKIMAGE_ID),
		End,
	    End;

    if(wc)
    {
	DoMethod(root, MUIM_Group_InitChange);
	DoMethod(btabout, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	    MUIM_Application_ReturnID, Push_About);
	DoMethod(root, OM_ADDMEMBER, (IPTR)wc);

	set(btskip, MUIA_Disabled, TRUE);
	DoMethod(root, MUIM_Group_ExitChange);

	while (running)
	{
	    switch (val = DoMethod(app,MUIM_Application_Input,(IPTR)&sigs))
	    {
		case MUIV_Application_ReturnID_Quit:
		    running = FALSE;
		break;
		case Push_Proceed:
		    running = FALSE;
		break;
		case Push_Abort:
		    DoMethod(root, MUIM_Group_InitChange);
		    DoMethod(root, OM_REMMEMBER, (IPTR)wc);
		    DoMethod(root, MUIM_Group_ExitChange);
		    MUI_DisposeObject(wc);

		    cleanup();
		    exit(-1);
		break;
		case Push_Skip:
kprintf("Skip\n");
		break;
		case Push_Help:
		    show_help_userlevel();
		break;
		case Push_About:
		    show_help_installer();
		break;
		default:
		    if(sigs)
		    {
			sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
			if (sigs & SIGBREAKF_CTRL_C) break;
			if (sigs & SIGBREAKF_CTRL_D) break;
		    }
		break;
// Extra Input loop. For this window only.
// Note: The special value
// MUIV_Application_ReturnID_Quit should be recognized
// as well
	    }                                                                  
	}
/*
char *contents;
SetAttrs(MyString,MUIA_String_Contents,"look",TAG_DONE);
GetAttr(MUIA_String_Contents,MyString,&contents);
printf("Always %s on the bright side of life.",contents);
*/

	DoMethod(root, MUIM_Group_InitChange);
	DoMethod(root, OM_REMMEMBER, (IPTR)wc);
	set(btskip, MUIA_Disabled, FALSE);
	DoMethod(root, MUIM_Group_ExitChange);
	MUI_DisposeObject(wc);
    }
}


/*
 * Ask user for a boolean
 */
long int request_bool( struct ParameterList *pl)
{
long int retval = 0;

return retval;
}


/*
 * Ask user for a number
 */
long int request_number( struct ParameterList *pl)
{
long int retval = 0;

return retval;
}


/*
 * Ask user for a string
 */
char *request_string( struct ParameterList *pl)
{
char *retval = NULL;

return retval;
}

/*
 * Ask user to choose one of N items
 */
long int request_choice( struct ParameterList *pl )
{
long int retval = 0;

return retval;
}


/*
 * Ask user for a directory
 */
char *request_dir( struct ParameterList *pl )
{
char *retval = NULL;

return retval;
}


/*
 * Ask user to insert a specific disk
 */
char *request_disk( struct ParameterList *pl )
{
char *retval = NULL;

return retval;
}


/*
 * Ask user for a file
 */
char *request_file( struct ParameterList *pl )
{
char *retval = NULL;

return retval;
}


/*
 * Ask user for a selection of multiple items (choose m of n items)
 */
long int request_options( struct ParameterList *pl )
{
long int retval;

  retval = GetPL( pl, _DEFAULT ).intval;

return retval;
}


/*
 * Ask user to confirm
 */
int request_confirm( struct ParameterList * pl )
{
int retval = 1;

return retval;
}


/*
 * Ask user if he really wants to abort
 */
void abort_install( VOID_FUNC destructor )
{
}


/*
 * Give a short summary on what was done
 */
void final_report( )
{
}


void display_text( char * msg )
{
}


int user_confirmation( char *message )
{
int retval = FALSE;

return retval;
}


