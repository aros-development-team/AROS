/*
    Copyright (C) 2013, The AROS Development Team.
    $Id: main.c 41537 2011-09-22 08:33:28Z sonic $
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "locale.h"
#include "sysexplorer_cl.h"

#define DEBUG 1
#include <aros/debug.h>

#define APPNAME "SysExplorer"
#define VERSION "SysExplorer 0.1"

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

Object *app, *window, *menu_quit;;

static BOOL GUIinit()
{
    BOOL retval = FALSE;

    app = ApplicationObject,
        MUIA_Application_Title,         (IPTR)APPNAME,
        MUIA_Application_Version,       (IPTR)VERSION,
        MUIA_Application_Copyright,     (IPTR)"(C) 2013, The AROS Development Team",
        MUIA_Application_Author,        (IPTR)"Pavel Fedin",
        MUIA_Application_Base,          (IPTR)APPNAME,
        MUIA_Application_Description,   __(MSG_DESCRIPTION),

        MUIA_Application_Menustrip, (IPTR)(MenuitemObject,
            MUIA_Family_Child, (IPTR)(MenuitemObject,
                MUIA_Menuitem_Title, __(MSG_MENU_PROJECT),
                MUIA_Family_Child, (IPTR)(menu_quit = MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_QUIT),
                End),
            End),
        End),

        SubWindow, (IPTR)(window = WindowObject,
            MUIA_Window_Title,	__(MSG_WINTITLE),
            MUIA_Window_ID, MAKE_ID('S', 'Y', 'E', 'X'),
            WindowContents, (IPTR)SysExplorerObject,
            End,
        End),
    End;

    if (app)
    {
        /* Quit application if the windowclosegadget or the esc key is pressed. */
        DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                 app, 2, 
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        DoMethod(menu_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        retval = TRUE;
    }

    return retval;
}

int __nocommandline = 1;

int main(void)
{
    if (!Locale_Initialize())
        return 20;

    if (GUIinit())
    {
        SET(window, MUIA_Window_Open, TRUE);

        if (XGET(window, MUIA_Window_Open))
        {
            DoMethod(app, MUIM_Application_Execute);
            SET(window, MUIA_Window_Open, FALSE);
        }

        DisposeObject(app);
    }

    Locale_Deinitialize();
    return 0;
}
