/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* gui_zune.c -- here are all functions for the ZUNE gui */

#include "Installer.h"
#include "cleanup.h"
#include "execute.h"
#include "locale.h"
#include "texts.h"
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

Object *app;
Object *wnd;
Object *reqwnd, *helpwnd, *helptext;
Object *reqroot, *root;
Object *btproceed, *btabort, *btskip, *bthelp;
Object *intermediate = NULL;

enum
{
    Push_NULL,
    Push_Proceed,
    Push_Abort,
    Push_Skip,
    Push_Help,
    Push_About,
    Push_Ok,
    Push_Cancel,
    Push_Last
};


/* ######################################################################## */

void AddContents(Object *obj)
{
    DoMethod(root, MUIM_Group_InitChange);
    if (intermediate != NULL)
    {
	DoMethod(root, OM_REMMEMBER, (IPTR)intermediate);
	MUI_DisposeObject(intermediate);
	intermediate = NULL;
    }
    DoMethod(root, OM_ADDMEMBER, (IPTR)obj);
    DoMethod(root, MUIM_Group_ExitChange);
}
void DelContents(Object *obj)
{
    DoMethod(root, MUIM_Group_InitChange);
    DoMethod(root, OM_REMMEMBER, (IPTR)obj);
    DoMethod(root, MUIM_Group_ExitChange);
    MUI_DisposeObject(obj);
}

#define WaitCTRL(sigs)							\
	if (sigs)							\
	{								\
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);	\
	    if (sigs & SIGBREAKF_CTRL_C) break;				\
	    if (sigs & SIGBREAKF_CTRL_D) break;				\
	}

#define disable_abort(val)	set(btabort, MUIA_Disabled, val)
#define disable_skip(val)	set(btskip, MUIA_Disabled, val)
#define disable_help(val)	set(bthelp, MUIA_Disabled, val)

#define NeedPROMPT(pl)					\
	if ( GetPL(pl, _PROMPT).intval == 0 )		\
	{						\
	    error = SCRIPTERROR;			\
	    traperr("Missing prompt!\n", NULL);	\
	}

#define NeedHELP(pl)					\
	if ( GetPL(pl, _HELP).intval == 0 )		\
	{						\
	    error = SCRIPTERROR;			\
	    traperr("Missing help!\n", NULL);		\
	}

#define TRANSSCRIPT()						\
    if ( preferences.transcriptstream != NULL )			\
    {								\
    int len = 0;						\
    char *out;							\
    int m = GetPL(pl, _PROMPT).intval;				\
	for ( i = 0 ; i < m ; i ++ )				\
	{							\
	    len += strlen(GetPL(pl, _PROMPT).arg[i]) + 2;	\
	}							\
	out = AllocVec((len+2)*sizeof(char), MEMF_PUBLIC);	\
	outofmem(out);						\
	out[0] = 0;						\
	for ( i = 0 ; i < m ; i ++ )				\
	{							\
	    strcat(out,">");					\
	    strcat(out,GetPL(pl, _PROMPT).arg[i]);		\
	    strcat(out,"\n");					\
	}							\
	Write(preferences.transcriptstream, out, len);		\
	FreeVec(out);						\
    }

/*
 * Ask user if he really wants to abort
 */
void abort_install()
{
BOOL running = TRUE, quit = FALSE;
LONG sigs = 0;
Object *wc;
Object *btok, *btcancel;

    wc = VGroup,
	Child, TextObject,
	    GroupFrameT(_(MSG_MESSAGE)),
	    MUIA_Text_Contents, (IPTR)ASKQUIT_STRING,
	End,
	Child, HBar(TRUE),
	Child, HGroup,
	    MUIA_Group_SameSize, TRUE,
	    Child, btok = CoolImageIDButton(_(MSG_OK), COOL_USEIMAGE_ID),
	    Child, btcancel = CoolImageIDButton(_(MSG_CANCEL), COOL_CANCELIMAGE_ID),
	End,
    End;
    set(btok,MUIA_CycleChain,1);
    set(btcancel,MUIA_CycleChain,1);
    DoMethod(btok, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Ok);
    DoMethod(btcancel, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Cancel);
    DoMethod(reqroot, OM_ADDMEMBER, (IPTR)wc);

    set(reqwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
	switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
	{
	    case Push_Cancel:
		running = FALSE;
		break;
	    case Push_Ok:
		quit = TRUE;
		running = FALSE;
		break;
	    default:
		break;
	}
	WaitCTRL(sigs);
    }
    set(reqwnd, MUIA_Window_Open, FALSE);

    DoMethod(reqroot, OM_REMMEMBER, (IPTR)wc);
    MUI_DisposeObject(wc);

    if (quit)
    {
	error = USERABORT;
	grace_exit = TRUE;
	/* Execute trap(1) */
	traperr( "User aborted!\n", NULL );
    }
}


/* ######################################################################## */


const char GuiWinTitle[] ="AROS - Installer V43.3";


#define WINDOWWIDTH  400
#define WINDOWHEIGHT 300

void helpwin(char *title, char *text)
{
BOOL running = TRUE;
LONG sigs = 0;

    set(helptext, MUIA_Text_Contents, text);
    set(helpwnd, MUIA_Window_Title, title);
    set(helpwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
	switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
	{
	    case MUIV_Application_ReturnID_Quit:
		running = FALSE;
		break;
	    default:
		break;
	}
	WaitCTRL(sigs);
    }
    set(helpwnd, MUIA_Window_Open, FALSE);
}


void helpwinpl(char *title, struct ParameterList *pl, int what)
{
char *text;

    text = collatestrings(GetPL(pl, what).intval, GetPL(pl, what).arg);
    if ( text != NULL )
    {
	helpwin(title, text);
	FreeVec(text);
    }
}


/*
 * Initialize the GUI
 */
void init_gui()
{
struct Screen *scr;

    scr = LockPubScreen(NULL);

    app = ApplicationObject,
	MUIA_Application_Title, "AROS - Installer",
	MUIA_Application_DoubleStart, TRUE,

   	SubWindow, wnd = WindowObject,
	    MUIA_Window_Title,	GuiWinTitle,
	    MUIA_Window_Width,	WINDOWWIDTH,
	    MUIA_Window_Height,	WINDOWHEIGHT,
	    MUIA_Window_CloseGadget,	FALSE,
	    MUIA_Window_NoMenus,	TRUE,
	    MUIA_Window_ID,	MAKE_ID('A','I','N','S'),
	    WindowContents, VGroup,
		Child, root = VGroup, End,
		Child, HBar(TRUE),
		Child, HGroup,
		    MUIA_Group_SameSize, TRUE,
		    Child, btproceed = CoolImageIDButton("Proceed", COOL_USEIMAGE_ID),
		    Child, btabort   = CoolImageIDButton("Abort", COOL_CANCELIMAGE_ID),
		    Child, btskip    = CoolImageIDButton("Skip", COOL_WARNIMAGE_ID),
		    Child, bthelp    = CoolImageIDButton("Help", COOL_INFOIMAGE_ID),
		End,
	    End,
	End,
   	SubWindow, helpwnd = WindowObject,
	    MUIA_Window_Width,	WINDOWWIDTH,
	    MUIA_Window_Height,	WINDOWHEIGHT,
	    MUIA_Window_CloseGadget,	TRUE,
	    MUIA_Window_SizeGadget,	FALSE,
	    MUIA_Window_NoMenus,	TRUE,
	    WindowContents, VGroup,
		Child, helptext = TextObject,
		    GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    MUIA_Text_Contents, (IPTR)NULL,
		End,
		Child, VSpace(1),
	    End,
	End,
   	SubWindow, reqwnd = WindowObject,
	    MUIA_Window_Width,	WINDOWWIDTH,
	    MUIA_Window_Height,	WINDOWHEIGHT,
	    MUIA_Window_CloseGadget,	FALSE,
	    MUIA_Window_SizeGadget,	FALSE,
	    MUIA_Window_NoMenus,	TRUE,
	    WindowContents, VGroup,
		Child, reqroot = VGroup, End,
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
    set(btproceed,MUIA_CycleChain,1);
    set(btabort,MUIA_CycleChain,1);
    set(btskip,MUIA_CycleChain,1);
    set(bthelp,MUIA_CycleChain,1);
    DoMethod(helpwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2,
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
    UnlockPubScreen(NULL, scr);
}

/*
 * Close GUI
 */
void deinit_gui()
{
    set(wnd, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
}


/*
 * Show user that we are going to "(abort)" install
 * Don't confuse NOVICE...
 */
void show_abort(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;

    if ( get_var_int("@user-level") > _NOVICE )
    {
	disable_abort(TRUE);
	disable_skip(TRUE);
	disable_help(TRUE);

	wc = VGroup,
	    Child, TextObject,
		GroupFrameT("Aborting Installation:"),
		MUIA_Text_Contents, (IPTR)(msg),
	    End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Proceed:
			running = FALSE;
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }

	    DelContents(wc);
	}

	disable_abort(FALSE);
	disable_skip(FALSE);
	disable_help(FALSE);
    }

}


/*
 * Show user how much we have completed yet
 */
void show_complete(long int percent)
{
char *text;

  text = AllocVec(strlen(GuiWinTitle) + 13, MEMF_PUBLIC);
  if ( text == NULL )
  {
    end_alloc();
  }
  sprintf(text, "%s (Done %3ld%%)", GuiWinTitle, percent);
  set(wnd, MUIA_Window_Title, text);
  FreeVec(text);
}


/*
 * Show user that we "(exit)" the installation
 */
void show_exit(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;
char *msg2;

    msg2 = AllocVec((strlen(msg)+strlen(DONE_TEXT))*sizeof(char), MEMF_PUBLIC);
    msg2[0] = 0;
    strcat(msg2,msg);
    strcat(msg2,DONE_TEXT);
    disable_abort(TRUE);
    disable_skip(TRUE);
    disable_help(TRUE);

    wc = VGroup,
	    Child, TextObject,
		GroupFrameT("Aborting Installation:"),
		MUIA_Text_Contents, (IPTR)(msg2),
	    End,
	End;

    if (wc)
    {
	AddContents(wc);

	while (running)
	{
	    switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
	    {
		case Push_Proceed:
		    running = FALSE;
		    break;
		default:
		    break;
	    }
	    WaitCTRL(sigs);
	}

	DelContents(wc);
    }

    disable_abort(FALSE);
    disable_skip(FALSE);
    disable_help(FALSE);
}


/*
 * Show the line which caused the parse-error
 */
void show_parseerror(char * msg, int errline)
{
}


/*
 * Tell user that some big task is to be done
 * "Be patient..."
 */
void show_working(char *msg)
{
    if (intermediate != NULL)
    {
	DelContents(intermediate);
    }
    
    intermediate = VGroup,
	Child, TextObject,
	GroupFrameT(_(MSG_MESSAGE)),
	    MUIA_Text_Contents, (IPTR)(msg),
	End,
    End;

    if (intermediate)
    {
	DoMethod(root, MUIM_Group_InitChange);
	DoMethod(root, OM_ADDMEMBER, (IPTR)intermediate);
	DoMethod(root, MUIM_Group_ExitChange);
    }
}


/*
 * Display a "(message)" to the user
 * Don't confuse NOVICE unless "(all)" users want to get this info
 */
void show_message(char * msg, struct ParameterList * pl)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;

    if ( GetPL(pl, _ALL).used == 1 || get_var_int("@user-level") > _NOVICE )
    {
	disable_skip(TRUE);
	if ( GetPL(pl, _HELP).used != 1 )
	{
	    disable_help(TRUE);
	}

	wc = VGroup,
	    Child, TextObject,
		GroupFrameT(_(MSG_MESSAGE)),
		MUIA_Text_Contents, (IPTR)(msg),
	    End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			helpwinpl(HELP_ON_MESSAGE, pl, _HELP);
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }

	    DelContents(wc);
	}

	disable_skip(FALSE);
	disable_help(FALSE);
    }
}


/*
 * Ask user for his user-level
 */
void request_userlevel(char *msg)
{
BOOL running = TRUE;
ULONG sigs = 0;
Object *wc;
Object *btabout;

LONG usrlevel, logval;
char *welcome;

Object *levelmx;
char **mxlabels;

/* Ask for User Level */
    usrlevel = preferences.defusrlevel;
    disable_skip(TRUE);

    if ( msg != NULL )
    {
	welcome = StrDup(msg);
    }
    else
    {
	welcome = AllocVec(sizeof(char *)*(strlen(WELCOME_TEMPLATE)+strlen(get_var_arg("@app-name"))), MEMF_PUBLIC);
	outofmem(welcome);
	sprintf(welcome, WELCOME_TEMPLATE, get_var_arg("@app-name"));
    }
    mxlabels = AllocVec(4*sizeof(STRPTR), MEMF_PUBLIC);
    mxlabels[0] = StrDup(NOVICE_NAME);
    mxlabels[1] = StrDup(ADVANCED_NAME);
    mxlabels[2] = StrDup(EXPERT_NAME);
    mxlabels[3] = NULL;


    wc = VGroup,
	Child, VGroup, GroupFrame,
	    Child, TextObject,
		GroupFrame,
		MUIA_Background, MUII_GroupBack,
		MUIA_Text_Contents, (IPTR)(welcome),
	    End,
	    Child, levelmx = RadioObject,
		GroupFrameT(USERLEVEL_REQUEST),
		MUIA_Radio_Entries, (IPTR)(mxlabels),
	    End,
	    Child, btabout = CoolImageIDButton("About Installer" ,COOL_ASKIMAGE_ID),
	    End,
	End;

    if (wc)
    {
	set(levelmx, MUIA_Radio_Active, usrlevel);
	DoMethod(btabout, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_About);
	AddContents(wc);

	while (running)
	{
	    switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
	    {
		case Push_Abort:
		    abort_install();
		    break;
		case Push_Proceed:
		    running = FALSE;
		    break;
		case Push_Help:
		    helpwin(HELP_ON_USERLEVEL, USERLEVEL_HELP);
		    break;
		case Push_About:
		    {
		    char *helptext;

#warning TODO: help/about for Installer
			helptext = AllocVec(512 * sizeof(char), MEMF_PUBLIC);
			sprintf(helptext, ABOUT_INSTALLER, INSTALLER_VERSION, INSTALLER_REVISION);
			helpwin(ABOUT_ON_INSTALLER, helptext);
			FreeVec(helptext);
		    }
		    break;
		default:
		    break;
	    }
	    WaitCTRL(sigs);
	}
	GetAttr(MUIA_Radio_Active, levelmx, &usrlevel);
	set_variable("@user-level", NULL, usrlevel);

	DelContents(wc);
    }
    FreeVec(welcome);
    FreeVec(mxlabels[0]);
    FreeVec(mxlabels[1]);
    FreeVec(mxlabels[2]);

/* Ask for Logfile creation */
    if ( usrlevel > 0 )
    {
	mxlabels[0] = StrDup(LOG_FILE_TEXT);
	mxlabels[1] = StrDup(LOG_PRINT_TEXT);
	mxlabels[2] = StrDup(LOG_NOLOG_TEXT);

	wc = VGroup,
	    Child, levelmx = RadioObject,
		GroupFrameT(LOG_QUESTION),
		MUIA_Radio_Entries, (IPTR)(mxlabels),
		End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    running = TRUE;
	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			{
			char *helptext;

#warning TODO: help for logfile-requester
			  helptext = AllocVec(512 * sizeof(char), MEMF_PUBLIC);
			  sprintf(helptext, LOG_HELP, preferences.transcriptfile);
			  helpwin(HELP_ON_LOGFILES, helptext);
			    FreeVec(helptext);
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    GetAttr(MUIA_Radio_Active, levelmx, &logval);
	    switch (logval)
	    {
		case 0: /* Log to file */
#warning TODO: Handle Logging output selection
		    break;
		case 1: /* Log to printer */
		    FreeVec(preferences.transcriptfile);
		    preferences.transcriptfile = StrDup("PRT:");
		    break;
		case 2: /* No Log */
		    FreeVec(preferences.transcriptfile);
		    preferences.transcriptfile = NULL;
		    break;
	    }

	    DelContents(wc);
	}
	FreeVec(mxlabels[0]);
	FreeVec(mxlabels[1]);
	FreeVec(mxlabels[2]);

	if (!preferences.nopretend)
	{
	    mxlabels[0] = StrDup(NOPRETEND_TEXT);
	    mxlabels[1] = StrDup(PRETEND_TEXT);
	    mxlabels[2] = NULL;

	    wc = VGroup,
		Child, levelmx = RadioObject,
		    GroupFrameT(PRETEND_QUESTION),
		    MUIA_Radio_Entries, (IPTR)(mxlabels),
		    End,
		End;

	    if (wc)
	    {
		AddContents(wc);

		running = TRUE;
		while (running)
		{
		    switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		    {
			case Push_Abort:
			    abort_install();
			    break;
			case Push_Proceed:
			    running = FALSE;
			    break;
			case Push_Help:
			    helpwin(HELP_ON_PRETEND, PRETEND_HELP);
			    break;
			default:
			    break;
		    }
		    WaitCTRL(sigs);
		}
		GetAttr(MUIA_Radio_Active, levelmx, &logval);
		switch (logval)
		{
		    case 0: /* Really Install */
			preferences.pretend = FALSE;
			break;
		    case 1: /* Only pretend to install */
			preferences.pretend = TRUE;
			break;
		}

		DelContents(wc);
	    }

	    FreeVec(mxlabels[0]);
	    FreeVec(mxlabels[1]);
	}
    }

    FreeVec(mxlabels);
    disable_skip(FALSE);
}


/*
 * Ask user for a boolean
 */
long int request_bool(struct ParameterList *pl)
{
long int retval;
char **mxlabels;
int i, m;

    NeedPROMPT(pl);
    NeedHELP(pl);

    m = GetPL(pl, _PROMPT).intval;

    retval = ( GetPL(pl, _DEFAULT).intval != 0 );

    mxlabels = AllocVec(3*sizeof(STRPTR), MEMF_PUBLIC);
    outofmem(mxlabels);
    if ( GetPL(pl, _CHOICES).used == 1 )
    {
	if (GetPL(pl, _CHOICES).intval >= 2)
	{
	    mxlabels[0] = StrDup(GetPL(pl, _CHOICES).arg[0]);
	    mxlabels[1] = StrDup(GetPL(pl, _CHOICES).arg[1]);
	}
	else if (GetPL(pl, _CHOICES).intval == 1)
	{
	    mxlabels[0] = StrDup(GetPL(pl, _CHOICES).arg[0]);
	    mxlabels[1] = StrDup(_(MSG_NO));
	}
	else
	{
	    mxlabels[0] = StrDup(_(MSG_YES));
	    mxlabels[1] = StrDup(_(MSG_NO));
	}
    }
    else
    {
	mxlabels[0] = StrDup(_(MSG_YES));
	mxlabels[1] = StrDup(_(MSG_NO));
    }
    mxlabels[2] = NULL;

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

	disable_skip(TRUE);
	out = collatestrings(m, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
		    End,
		    Child, levelmx = RadioObject,
			MUIA_Radio_Entries, (IPTR)(mxlabels),
		    End,
		End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			if (GetPL(pl, _HELP).intval)
			{
			    helpwinpl(HELP_ON_ASKNUMBER, pl, _HELP);
			}
			else
			{
			    helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    GetAttr(MUIA_Radio_Active, levelmx, &retval);

	    DelContents(wc);
	}
	FreeVec(out);
	disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != NULL )
    {
	Write(preferences.transcriptstream, "Ask Question: Result was \"", 26);
	Write(preferences.transcriptstream, mxlabels[retval], strlen(mxlabels[retval]));
	Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    FreeVec(mxlabels[0]);
    FreeVec(mxlabels[1]);
    FreeVec(mxlabels);

return retval;
}


/*
 * Ask user for a number
 */
long int request_number(struct ParameterList *pl)
{
long int retval;
long int i, min, max;
char minmax[MAXARGSIZE];

    retval = GetPL( pl, _DEFAULT ).intval;
    if ( GetPL( pl, _RANGE ).used == 1 )
    {
	min = GetPL( pl, _RANGE ).intval;
	max = GetPL( pl, _RANGE ).intval2;
	/* Wrong order ? Change order */
	if ( max < min )
	{
	    i = min;
	    min = max;
	    max = i;
	}
	sprintf(minmax, "Range = [%ld, %ld]", min, max);
    }
    else
    {
	minmax[0] = 0;
#define INTMAX  32767
	max = INTMAX;
	min = ( retval < 0 ) ? retval : 0;
    }


    TRANSSCRIPT();
    if ( get_var_int( "@user-level" ) > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG sigs = 0;

	disable_skip(TRUE);
	out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
		    End,
		    Child, st  = StringObject,
			StringFrame,
			MUIA_String_Accept,	(IPTR)"-0123456789",
			MUIA_String_Integer,	retval,
			MUIA_String_AdvanceOnCR,TRUE,
			MUIA_CycleChain,	TRUE,
		    End,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(minmax),
		    End,
		End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			GetAttr(MUIA_String_Integer, st, &retval);
			if ( retval <= max && retval >= min)
			{
			    running = FALSE;
			}
			break;
		    case Push_Help:
			if (GetPL(pl, _HELP).intval)
			{
			    helpwinpl(HELP_ON_ASKSTRING, pl, _HELP);
			}
			else
			{
			    helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    GetAttr(MUIA_String_Integer, st, &retval);

	    DelContents(wc);
	}
	FreeVec(out);
	disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != NULL )
    {
    char tmpbuf[MAXARGSIZE];
	Write(preferences.transcriptstream, "Ask Number: Result was \"", 24);
	sprintf(tmpbuf, "%ld", retval);
	Write(preferences.transcriptstream, tmpbuf, strlen(tmpbuf));
	Write(preferences.transcriptstream, "\".\n\n", 4);
    }

return retval;
}


/*
 * Ask user for a string
 */
char *request_string(struct ParameterList *pl)
{
char *retval, *string = NULL;
int i;

    if ( GetPL(pl, _DEFAULT).used == 1 )
    {
	string = StrDup(GetPL(pl, _DEFAULT).arg[0]);
    }
    else
    {
	string = StrDup(EMPTY_STRING);
    }
    TRANSSCRIPT();
    if ( get_var_int( "@user-level" ) > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG sigs = 0;

	disable_skip(TRUE);
	out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
		    End,
		    Child, st  = StringObject,
			StringFrame,
			MUIA_String_Contents,	(IPTR)string,
			MUIA_String_MaxLen,	128,
			MUIA_String_AdvanceOnCR,TRUE,
			MUIA_CycleChain,	TRUE,
		    End,
		End,
	    End;

	if (wc)
	{
	char *str;
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			if (GetPL(pl, _HELP).intval)
			{
			    helpwinpl(HELP_ON_ASKSTRING, pl, _HELP);
			}
			else
			{
			    helpwin(HELP_ON_ASKNUMBER, get_var_arg("@asknumber-help"));
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    FreeVec(string);
	    get(st, MUIA_String_Contents, &str);
	    string = StrDup(str);

	    DelContents(wc);
	}
	FreeVec(out);
	disable_skip(FALSE);
    }
    retval = addquotes(string);
    FreeVec(string);
    if ( preferences.transcriptstream != NULL )
    {
	Write(preferences.transcriptstream, "Ask String: Result was ", 23);
	Write(preferences.transcriptstream, retval, strlen(retval));
	Write(preferences.transcriptstream, ".\n\n", 3);
    }

return retval;
}

/*
 * Ask user to choose one of N items
 */
long int request_choice(struct ParameterList *pl)
{
long int retval;
char **mxlabels = NULL;
int i, max = 0;

    NeedPROMPT(pl);

    retval = GetPL(pl, _DEFAULT).intval;

    if ( GetPL(pl, _CHOICES).used == 1 )
    {
	max = GetPL(pl, _CHOICES).intval;

	if ( max > 32 )
	{ 
	    error = SCRIPTERROR;
	    traperr("More than 32 choices given!\n", NULL);
	}

	mxlabels = AllocVec((max+1)*sizeof(STRPTR), MEMF_PUBLIC);
	outofmem(mxlabels);

	for ( i = 0 ; i < max ; i++ )
	{
	    mxlabels[i] = StrDup(GetPL(pl, _CHOICES).arg[i]);
	}
	mxlabels[i] = NULL;
    }
    else
    {
	error = SCRIPTERROR;
	traperr("No choices given!\n", NULL);
    }

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

	disable_skip(TRUE);
	out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
		    End,
		    Child, levelmx = RadioObject,
			MUIA_Radio_Entries, (IPTR)(mxlabels),
		    End,
		End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			if (GetPL(pl, _HELP).intval)
			{
			    helpwinpl(HELP_ON_ASKCHOICE, pl, _HELP);
			}
			else
			{
			    helpwin(HELP_ON_ASKCHOICE, get_var_arg("@asknumber-help"));
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    GetAttr(MUIA_Radio_Active, levelmx, &retval);

	    DelContents(wc);
	}
	FreeVec(out);
	disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != NULL )
    {
	Write(preferences.transcriptstream, "Ask Choice: Result was \"", 24);
	Write(preferences.transcriptstream, mxlabels[retval], strlen(mxlabels[retval]));
	Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    for ( i = 0 ; i < max ; i++ )
    {
	FreeVec(mxlabels[i]);
    }
    FreeVec(mxlabels);

return retval;
}


/*
 * Ask user for a directory
 */
char *request_dir(struct ParameterList *pl)
{
char *retval, *string;

    if ( GetPL(pl, _DEFAULT).used == 0 )
    {
	error = SCRIPTERROR;
	traperr("No default specified!", NULL);
    }
    string = GetPL(pl, _DEFAULT).arg[0];

#warning TODO: write whole function request_dir()

    retval = addquotes(string);

return retval;
}


/*
 * Ask user to insert a specific disk
 */
char *request_disk(struct ParameterList *pl)
{
char *retval, *string;

    if ( GetPL(pl, _DEST).used == 0 )
    {
	error = SCRIPTERROR;
	traperr("No dest specified!", NULL);
    }
    string = GetPL(pl, _DEST).arg[0];

#warning TODO: write whole function request_disk()

    retval = addquotes(string);

return retval;
}


/*
 * Ask user for a file
 */
char *request_file(struct ParameterList *pl)
{
char *retval, *string;

    if ( GetPL(pl, _DEFAULT).used == 0 )
    {
	error = SCRIPTERROR;
	traperr("No default specified!", NULL);
    }
    string = GetPL(pl, _DEFAULT).arg[0];

#warning TODO: write whole function request_file()

    retval = addquotes(string);

return retval;
}


/*
 * Ask user for a selection of multiple items (choose m of n items)
 */
long int request_options(struct ParameterList *pl)
{
long int retval;
char **mxlabels = NULL;
int i, max = 0;
Object *marks[33];
Object *labels[33];
long int val;
BOOL j;

    NeedPROMPT(pl);

    retval = GetPL(pl, _DEFAULT).intval;

    if ( GetPL(pl, _CHOICES).used == 1 )
    {
	max = GetPL(pl, _CHOICES).intval;

	if ( max > 32 )
	{ 
	    error = SCRIPTERROR;
	    traperr("More than 32 choices given!\n", NULL);
	}

	mxlabels = AllocVec((max+1)*sizeof(STRPTR), MEMF_PUBLIC);
	outofmem(mxlabels);

	for ( i = 0 ; i < max ; i++ )
	{
	    mxlabels[i] = StrDup(GetPL(pl, _CHOICES).arg[i]);
	    marks[i] = MUI_MakeObject(MUIO_Checkmark, (IPTR)mxlabels[i]);
	    labels[i] = LLabel(mxlabels[i]);

	    set(marks[i], MUIA_CycleChain, TRUE);
	    if ( (retval & 1<<i) != 0 )
		set(marks[i], MUIA_Selected, TRUE);
	}
	mxlabels[i] = NULL;
    }
    else
    {
	error = SCRIPTERROR;
	traperr("No choices given!\n", NULL);
    }

    TRANSSCRIPT();

    if ( get_var_int("@user-level") > _NOVICE )
    {
    char *out;
    BOOL running = TRUE;
    Object *levelmx, *wc;
    ULONG sigs = 0;

	disable_skip(TRUE);
	out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    MUIA_Background, MUII_GroupBack,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
		    End,
		    Child, levelmx = ColGroup(2),
			GroupFrame,
			MUIA_Background, MUII_GroupBack,
		    End,
		End,
	    End;

	if (wc)
	{
	    for ( i = 0 ; i < max ; i++ )
	    {
		DoMethod(levelmx, OM_ADDMEMBER, (IPTR)marks[i]);
		DoMethod(levelmx, OM_ADDMEMBER, (IPTR)labels[i]);
	    }
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Help:
			if (GetPL(pl, _HELP).intval)
			{
			    helpwinpl(HELP_ON_ASKCHOICE, pl, _HELP);
			}
			else
			{
			    helpwin(HELP_ON_ASKCHOICE, get_var_arg("@asknumber-help"));
			}
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }
	    retval = 0;
	    for ( i = 0 ; i < max ; i++ )
	    {
		GetAttr(MUIA_Selected, marks[i], &val);
		if (val)
		{
		    retval |= 1<<i;
		}
	    }

	    DelContents(wc);
	}
	FreeVec(out);
	disable_skip(FALSE);
    }
    if ( preferences.transcriptstream != NULL )
    {
	Write(preferences.transcriptstream, "Ask Options: Result was \"", 24);
	j = FALSE;
	for ( i = 0 ; i < max ; i++ )
	{
	    if ( (retval & (1<<i)) != 0 )
	    {
		if ( j )
		{
		    Write(preferences.transcriptstream, "\", \"", 4);
		}
		Write(preferences.transcriptstream, mxlabels[i], strlen(mxlabels[i]));
		j = TRUE;
	    }
	}
	Write(preferences.transcriptstream, "\".\n\n", 4);
    }
    for ( i = 0 ; i < max ; i++ )
    {
	FreeVec(mxlabels[i]);
    }
    FreeVec(mxlabels);

return retval;
}


/*
 * Ask user to confirm
 */
int request_confirm(struct ParameterList * pl)
{
int retval = 1;
BOOL running = TRUE;
Object *wc;
ULONG sigs = 0;
char *out;

    NeedPROMPT(pl);
    NeedHELP(pl);

    if ( get_var_int("@user-level") >= GetPL(pl, _CONFIRM).intval )
    {
	out = collatestrings(GetPL(pl, _PROMPT).intval, GetPL(pl, _PROMPT).arg);

	wc = VGroup,
	    Child, TextObject,
		GroupFrame,
		MUIA_Background, MUII_GroupBack,
		MUIA_Text_Contents, (IPTR)(out),
	    End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case Push_Abort:
			abort_install();
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    case Push_Skip:
			retval = 0;
			running = FALSE;
			break;
		    case Push_Help:
			helpwinpl(HELP_ON_CONFIRM, pl, _HELP);
			break;
		    default:
			break;
		}
		WaitCTRL(sigs);
	    }

	    DelContents(wc);
	}
	FreeVec(out);
    }

return retval;
}


/*
 * Give a short summary on what was done
 */
void final_report()
{
#ifdef DEBUG
    printf("Application has been installed in %s.\n", get_var_arg("@default-dest"));
#endif /* DEBUG */
}


void display_text(char * msg)
{
BOOL running = TRUE;
LONG sigs = 0;
Object *wc;
Object *btok;

    wc = VGroup,
	Child, TextObject,
	    GroupFrameT(_(MSG_MESSAGE)),
	    MUIA_Text_Contents, (IPTR)msg,
	End,
	Child, HBar(TRUE),
	Child, HGroup,
	    Child, btok = CoolImageIDButton(_(MSG_OK), COOL_USEIMAGE_ID),
	End,
    End;
    set(btok,MUIA_CycleChain,1);
    DoMethod(btok, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2, MUIM_Application_ReturnID, Push_Ok);
    DoMethod(reqroot, OM_ADDMEMBER, (IPTR)wc);

    set(reqwnd, MUIA_Window_Open, TRUE);
    while (running)
    {
	switch (DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
	{
	    case Push_Ok:
		running = FALSE;
		break;
	    default:
		break;
	}
	WaitCTRL(sigs);
    }
    set(reqwnd, MUIA_Window_Open, FALSE);

    DoMethod(reqroot, OM_REMMEMBER, (IPTR)wc);
    MUI_DisposeObject(wc);
}


