
#include <stdio.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <aros/debug.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include "mui.h"
#undef SysBase

struct Library *MUIMasterBase;

Object *MUI_NewObject(char *classname, int tag,...)
{
    return MUI_NewObjectA(classname, (struct TagItem*)&tag);
}

void main(void)
{
    Object *app;
    Object *wnd;
    Object *quit_button;

    MUIMasterBase = OpenLibrary("muimaster.library", 0);
    if (!MUIMasterBase)
    {
    	puts("Could not open muimaster.library!\n");
	exit(0);
    }

    app = ApplicationObject,
    	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "test",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
    	    	Child, TextObject, TextFrame, MUIA_Text_Contents, "\33cHello World!!\nThis is a text object\n\33lLeft \33bbold\33n\n\33rRight",End,
    	    	Child, HGroup,
		    GroupFrameT("A horizontal group"),
		    Child, ColGroup(2),
			GroupFrameT("A column group"),
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button1", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button2", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button3", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button4", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button5", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button6", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			End,
		    Child, VGroup,
			GroupFrameT("A vertical group"),
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button7", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			Child, TextObject, ButtonFrame, MUIA_Background, MUII_ButtonBack, MUIA_Text_PreParse, "\33c", MUIA_Text_Contents, "Button8", MUIA_InputMode, MUIV_InputMode_RelVerify, End,
			End,
		    End,

    	    	Child, HGroup,
    	    	    Child, quit_button = TextObject,
			ButtonFrame,
			MUIA_Background, MUII_ButtonBack,
			MUIA_Text_PreParse, "\33c",
			MUIA_Text_Contents, "Quit",
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			End,
    	    	    End,
    	        End,
    	    End,
    	End;

    if (app)
    {
	ULONG sigs = 0;

    	kprintf("Application Object created at 0x%lx\n",app);
    	kprintf("Window Object created at 0x%lx\n",wnd);

    	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    	DoMethod(quit_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	set(wnd,MUIA_Window_Open,TRUE);

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }
    
    CloseLibrary(MUIMasterBase);
}
