/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <libraries/coolimages.h>


#define AddContents(obj)			\
	DoMethod(root, MUIM_Group_InitChange);	\
	DoMethod(root, OM_ADDMEMBER, (IPTR)obj);\
	DoMethod(root, MUIM_Group_ExitChange);

#define DelContents(obj)			\
	DoMethod(root, MUIM_Group_InitChange);	\
	DoMethod(root, OM_REMMEMBER, (IPTR)obj);\
	DoMethod(root, MUIM_Group_ExitChange);	\
	MUI_DisposeObject(wc);

Object *app;
Object *wnd;
Object *root;
Object *btabort, *btproceed;

int quit = FALSE;

enum
{
Push_NULL,
Push_Proceed,
Push_Abort,
};

/* ######################################################################## */


const char GuiWinTitle[] ="AROS - Installer V43.3";
struct Screen *scr;


void init_gui()
{

    scr = LockPubScreen(NULL);

    app = ApplicationObject,
	MUIA_Application_Title, "AROS - Installer",

   	SubWindow, wnd = WindowObject,
	    MUIA_Window_Title,	GuiWinTitle,
	    MUIA_Window_Width,	400,
	    MUIA_Window_Height,	300,
	    MUIA_Window_CloseGadget,	FALSE,
	    MUIA_Window_NoMenus,	TRUE,
	    MUIA_Window_ID,	MAKE_ID('A','I','N','S'),
	    WindowContents,
	    VGroup,
		Child, root = VGroup, End,
		Child, HBar(TRUE),
		Child, HGroup,
		MUIA_Group_SameSize, TRUE,
		Child, btproceed = CoolImageIDButton("Proceed", COOL_USEIMAGE_ID),
		Child, btabort   = CoolImageIDButton("Abort", COOL_CANCELIMAGE_ID),
		End,
	    End,
	End,
    End;

    if (app == NULL)
    {
	/* failed to initialize GUI */
printf("Failed to intialize Zune GUI\n");
    }
    set(app,MUIA_Window_DefaultObject, (IPTR)root);
    set(btproceed,MUIA_CycleChain,1);
    set(btabort,MUIA_CycleChain,1);

    DoMethod(btproceed, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Proceed);
    DoMethod(btabort, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)app, 2,
	MUIM_Application_ReturnID, Push_Abort);
    set(wnd, MUIA_Window_Open, TRUE);
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
 * Ask user for a number
 */
long int request_number(long int def)
{
long int retval;
long int min, max;

    retval = def;
    max = 999;
    min = 0;


    {
    char *out;
    BOOL running = TRUE;
    Object *st, *wc;
    ULONG val, sigs = 0;

	out = StrDup("TEST");

	wc = VGroup,
	Child, VGroup, GroupFrame,
		    Child, TextObject,
			MUIA_Text_Contents, (IPTR)(out),
			MUIA_Text_Editable, FALSE,
			MUIA_Text_Multiline, TRUE,
		    End,
		    Child, st  = StringObject,
			StringFrame,
			MUIA_String_Accept,	(IPTR)"-0123456789",
			MUIA_String_Integer,	retval,
			MUIA_String_AdvanceOnCR,TRUE,
			MUIA_CycleChain,	TRUE,
		    End,
		End,
	    End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (val = DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case MUIV_Application_ReturnID_Quit:
		    case Push_Abort:
			quit = TRUE; running = FALSE;
			break;
		    case Push_Proceed:
			GetAttr(MUIA_String_Integer, st, &retval);
			if ( retval < max && retval > min)
			{
			    running = FALSE;
			}
			break;
		    default:
			break;
		}
		
		sigs = CheckSignal(SIGBREAKF_CTRL_C | sigs);

	    }
	    GetAttr(MUIA_String_Integer, st, &retval);

	    DelContents(wc);
	}
	FreeVec(out);
    }


return retval;
}


char *request_string(char *def)
{
char *retval, *string, *secret;
char *out;
BOOL running = TRUE;
Object *st, *tst, *wc;
ULONG val, sigs = 0;

    string = def;

    out = StrDup("test");

    wc = VGroup, GroupFrame,
	    Child, TextObject,
		MUIA_Text_Contents, (IPTR)(out),
		MUIA_Text_Editable, FALSE,
		MUIA_Text_Multiline, TRUE,
	    End,
	    Child, st  = StringObject,
		StringFrame,
		MUIA_String_Contents,	(IPTR)string,
		MUIA_String_MaxLen,	12,
		MUIA_String_AdvanceOnCR,TRUE,
		MUIA_CycleChain,	TRUE,
	    End,
	    Child, tst  = StringObject,
		StringFrame,
		MUIA_String_Contents,	(IPTR)secret,
		MUIA_String_MaxLen,	12,
		MUIA_String_Secret,     TRUE,
                MUIA_String_AdvanceOnCR,TRUE,
		MUIA_CycleChain,	TRUE,
	    End,
        End;

	if (wc)
	{
	    AddContents(wc);

	    while (running)
	    {
		switch (val = DoMethod(app,MUIM_Application_NewInput,(IPTR)&sigs))
		{
		    case MUIV_Application_ReturnID_Quit:
		    case Push_Abort:
			quit = TRUE; running = FALSE;
			break;
		    case Push_Proceed:
			running = FALSE;
			break;
		    default:
			break;
		}
		sigs = CheckSignal(SIGBREAKF_CTRL_C | sigs);

	    }
	    get(st, MUIA_String_Contents, (IPTR *)&string);
	    if (strlen(string)==0)
            {
                get(tst, MUIA_String_Contents, (IPTR *)&secret);
                retval = StrDup(secret);
            }
            else retval = StrDup(string);

	    DelContents(wc);
	}
	else
	{
	    retval = StrDup(string);
	}
	FreeVec(out);

return retval;
}


int main()
{
char *string;
long int intval;

    init_gui();

    for(;;)
    {
	string = request_string("blah");
	printf("you entered string <%s>\n",string);
	FreeVec(string);
	if(quit) break;
	intval = request_number(99);
	printf("you entered number <%ld>\n",intval);
	if(quit) break;
    }
    deinit_gui();

    return 0;
}
