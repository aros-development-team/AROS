/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include    <exec/types.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>

#include    <proto/alib.h>
#include    <proto/exec.h>
#include    <proto/dos.h>
#include    <proto/intuition.h>
#include    <proto/muimaster.h>

#include    <dos/dos.h>
#include    <intuition/gadgetclass.h>
#include    <intuition/icclass.h>
#include    <clib/alib_protos.h>
#include    <zune/loginwindow.h>

#include <libraries/mui.h>

struct Library *MUIMasterBase;

Object *app;

#define ARG_TEMPLATE	"LOCAL/S"
#define ARG_LOCAL	0
#define TOTAL_ARGS	1

int main(void)
{
    IPTR                    argarray[TOTAL_ARGS] = { NULL };
    struct RDArgs           *args;
    Object                  *LoginWin;
    char                    *userName, *userPass, *loginMeth, *string;
    struct SecurityBase     *SecurityBase;
    int                     error = RETURN_ERROR;

    if ((!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37)))||(!(SecurityBase = OpenLibrary("security.library", 0))))
    {
	error = ERROR_INVALID_RESIDENT_LIBRARY;
	goto LibError;
    }

    args = ReadArgs( ARG_TEMPLATE, argarray, NULL);

    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);


    if (argarray[ARG_LOCAL])
    {
        app = ApplicationObject,
   	    SubWindow, LoginWin = LoginWindowObject,
            End,
	    End;
    }
    else
    {
        string = "Test User";
        app = ApplicationObject,
   	    SubWindow, LoginWin = LoginWindowObject,
                MUIA_LoginWindow_UserName, (IPTR)string,
                MUIA_LoginWindow_Method, (IPTR)"smb: NAHOMENET Domain",
	        MUIA_LoginWindow_Method, (IPTR)"smb: aros.org Domain",
	        MUIA_LoginWindow_Method, (IPTR)"smb: Dialup Connection...",
	        MUIA_LoginWindow_Cancel_Disabled, TRUE,
            End,
	    End;
    }

    if (app)
    {
	ULONG sigs = 0;

	DoMethod
        (
            LoginWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 
            2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );

	set(LoginWin,MUIA_Window_Open,TRUE);

	while (DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
                if (sigs == LWA_RV_OK)
                    break;

                sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		
                if (sigs & SIGBREAKF_CTRL_C)
                    break;
		if (sigs & SIGBREAKF_CTRL_D)
                    break;

	    }
	}

        if (sigs == LWA_RV_OK)
        {
            get(LoginWin, MUIA_LoginWindow_UserName, &string);
            userName = StrDup(string);
            
            get(LoginWin, MUIA_LoginWindow_UserPass, &string);
            userPass = StrDup(string);
            
            get(LoginWin, MUIA_LoginWindow_Method, &string);
            loginMeth = StrDup(string);

            Printf(" Hello %s, welcome to aros..\n",userName);
            Printf("  i shouldnt really say this but your password is '%s'\n",userPass);
            Printf("\n  you have logged on using '%s'\n",loginMeth);

	    FreeVec(string);
	    FreeVec(userName);
	    FreeVec(userPass);
	    FreeVec(loginMeth);

            error = RETURN_OK;
        }
        else
        {
            Printf(" User canceled");
        }
	MUI_DisposeObject(app);
    }
    else
    {
        printf("Failed to intialize Zune GUI\n");
    }
    if (args)    FreeArgs(args);

LibError:
    if (MUIMasterBase) CloseLibrary((struct Library *)MUIMasterBase);

    if (SecurityBase) CloseLibrary((struct Library *)SecurityBase);
    if (DOSBase) CloseLibrary((struct Library *)DOSBase);

    return(error);
}

