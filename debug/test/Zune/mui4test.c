/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Test for MUI4 features
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

Object *app;
Object *wnd;
Object *flttext;
Object *btappend;

/* ######################################################################## */

void init_gui()
{
    app = ApplicationObject,
        MUIA_Application_Title, "MUI4 Test",
        SubWindow, wnd = WindowObject,
            MUIA_Window_Title, "Floattext Test",
            MUIA_Window_ID,	MAKE_ID('M','4','T','E'),
            WindowContents, VGroup,
                Child, flttext = FloattextObject,
                    MUIA_Floattext_Text, "Hello",
                End,
                Child, HGroup,
                    Child, btappend = SimpleButton("Append"),
                End,
            End,
        End,
    End;

    if (app)
    {
        DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR)app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        /* Test for MUIM_Floattext_Append */
        DoMethod(btappend, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR)flttext, 2,
            MUIM_Floattext_Append, " World!");

        set(wnd, MUIA_Window_Open, TRUE);
    }
}

void deinit_gui()
{
    set(wnd, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
}

int main(void)
{
    init_gui();
    DoMethod(app, MUIM_Application_Execute);
    deinit_gui();

    return 0;
}
