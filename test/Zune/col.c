/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include <libraries/mui.h>

#define DEBUG 1
#include <aros/debug.h>

Object *app;

Object *MakeLabel(STRPTR str)
{
  return (MUI_MakeObject(MUIO_Label, str, 0));
}

int main(void)
{
    Object *wnd;
    Object *img;
    Object *a, *b, *c, *d;

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "col",
	    MUIA_Window_Activate, TRUE,
    	    WindowContents, d = HGroup,
		   GroupFrameT("Background"),
			       Child, c = VGroup,
		   Child, a = PopimageObject, End,
		   Child, b = MakeLabel("Window"),
			       End,
			       Child, VGroup,
		   Child, PopimageObject, End,
		   Child, MakeLabel("Requester"),
			       End,
	End,
	End,
	End;

    if (app)
    {
	ULONG sigs = 0;
	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	set(wnd, MUIA_Window_Open, TRUE);

	printf("%d[%p] %d[%p] %d[%p] %d\n", _maxwidth(a), a, _maxwidth(b), b, _maxwidth(c), c, _maxwidth(d));

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		
	    }
	}
	set(wnd, MUIA_Window_Open, FALSE);
	MUI_DisposeObject(app);
    }
    
    return 0;
}

