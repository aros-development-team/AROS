/*
    Copyright © 2009-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Request string from user
*/

/******************************************************************************
 
 
    NAME
 
        RequestString [STRING] [TEXT] [TITLE] [NOGADS] [WIDTH] [SAFE] [PERSIST]
                      [ENCRYPT] [COMPARE] [PUBSCREEN]
 
    SYNOPSIS
 
        STRING, TEXT/K, TITLE/K, NOGADS/S, WIDTH/N, SAFE/S, PERSIST/S,
        ENCRYPT/S, COMPARE/K, PUBSCREEN/K
 
    LOCATION
 
        C:
 
    FUNCTION

        Shows a requester with a string gadget for user input.

    INPUTS
 
        STRING     -- Initial content of string gadget.
        TEXT       -- Label string.
        TITLE      -- Title string of requester. This also adds dragbar, closegadget
                      and a depthgadget.
        NOGADS     -- Suppress gadgets when TITLE argument is given.
        WIDTH      -- Minimal width as number of characters.
        SAFE       -- Hide user input with "*".
        PERSIST    -- Intuition is blocked until requester is quitted.
        ENCRYPT    -- Encrypt result before returning. Requires that one of these
                      environment variables is set: USER, USERNAME or LOGIN.
        COMPARE    -- If the input string is not equal to the argument
                      of COMPARE return WARN.
        PUBSCREEN  -- Open requester on given pubscreen.

    RESULT

    NOTES

    EXAMPLE

    BUGS
        PERSIST doesn't allways work.
        WIDTH not implemented.

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <aros/debug.h>
#include <libraries/mui.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#include <string.h>

AROS_UFH3(VOID, persistfunc,
    AROS_UFPA(struct Hook *     , hook, A0),
    AROS_UFPA(Object *          , obj,  A2),
    AROS_UFPA(Msg               , msg,  A1))
{
    AROS_USERFUNC_INIT

    DoMethod(obj, MUIM_Window_ScreenToFront);
    DoMethod(obj, MUIM_Window_ToFront);
    SET(obj, MUIA_Window_Activate, TRUE);

    AROS_USERFUNC_EXIT
}

#define  TEMPLATE "STRING,TEXT/K,TITLE/K,NOGADS/S,WIDTH/N,SAFE/S,PERSIST/S,ENCRYPT/S,COMPARE/K,PUBSCREEN/K"

enum {
    ARG_STRING,
    ARG_TEXT,
    ARG_TITLE,
    ARG_NOGADS,
    ARG_WIDTH,
    ARG_SAFE,
    ARG_PERSIST,
    ARG_ENCRYPT,
    ARG_COMPARE,
    ARG_PUBSCREEN,
    ARG_COUNT
};

const char version[] = "\0$VER: RequestString 40.0 (24.10.2010)";

int main(void)
{
    IPTR                args[ARG_COUNT] = {0};
    struct RDArgs      *rdargs = NULL;
    Object             *app, *win, *string;
    TEXT                cbuffer[12], uname[48];
    struct Hook         persisthook = { {0} , 0, 0, 0};
    STRPTR              returntext = NULL;
    BOOL                havegads = FALSE;
    LONG                retval = RETURN_FAIL;

    if ((rdargs = ReadArgs(TEMPLATE, args, NULL)) == NULL)
    {
        PrintFault(IoErr(), "RequestString");
        return RETURN_FAIL;
    }

    if (args[ARG_TITLE] && !args[ARG_NOGADS])
    {
        havegads = TRUE;
    }

    app = ApplicationObject,
        MUIA_Application_Title, "RequestString",
        MUIA_Application_Version, version,
        MUIA_Application_Copyright, "© 2009-2010 The AROS Development Team",
        MUIA_Application_Author, "The AROS Development Team",
        MUIA_Application_Description, "Request string from user",
        MUIA_Application_Base, "REQUESTSTRING",
        MUIA_Application_UseCommodities, FALSE,
        MUIA_Application_UseRexx, FALSE,
        SubWindow, win = WindowObject,
            MUIA_Window_Title, args[ARG_TITLE], // safe to use with NULL
            MUIA_Window_PublicScreen, args[ARG_PUBSCREEN], // silently ignored when NULL or not available
            MUIA_Window_CloseGadget, havegads,
            MUIA_Window_DepthGadget, havegads,
            MUIA_Window_DragBar, havegads,
            MUIA_Window_NoMenus, TRUE,
            WindowContents, VGroup,
                Child, TextObject,
                    MUIA_Text_PreParse, "\33c", // center text
                    MUIA_Text_Contents, args[ARG_TEXT],  // safe to use with NULL
                End,
                Child, string = StringObject,
                    StringFrame,
                    MUIA_String_Contents, args[ARG_STRING], // safe to use with NULL
                    MUIA_String_Secret, args[ARG_SAFE],
                End,
            End,
        End,
    End;

    if (app)
    {
        persisthook.h_Entry = (HOOKFUNC)persistfunc;

        DoMethod
        (
            win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
        DoMethod
        (
            string, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
            app, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );

        if (args[ARG_PERSIST])
        {
            DoMethod
            (
                win, MUIM_Notify, MUIA_Window_Activate, FALSE,
                win, 2,
                MUIM_CallHook, &persisthook
            );
        }

        SET(win, MUIA_Window_Open, TRUE);
        SET(win, MUIA_Window_ActiveObject, string);
        DoMethod(app, MUIM_Application_Execute);

        returntext = (STRPTR)XGET(string, MUIA_String_Contents);

        /* Try to find out who the user is if we are to encrypt the output.
         * I really don't know how to acquire the username, but this might
         * be a good guess of how to do it.
         */
        if (args[ARG_ENCRYPT] != 0)
        {
            if (GetVar("USER", uname, 47, 0) == -1)
                if (GetVar("USERNAME", uname, 47, 0) == -1)
                    if (GetVar("LOGIN", uname, 47, 0) == -1)
                        uname[0] = 0;
            ACrypt(cbuffer, returntext, uname);
            returntext = cbuffer;
        }

        Printf("\"%s\"\n", returntext);

        /* Here follows the COMPARE parameter. If the input string is not equal
         * to the argument of COMPARE we return WARN.
         */
        if (args[ARG_COMPARE] != 0)
            retval = (strcmp(returntext, (STRPTR)args[ARG_COMPARE])) ? RETURN_WARN : 0;
        else
            retval = 0;

        MUI_DisposeObject(app);
    }

    return retval;
}
