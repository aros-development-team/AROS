/*
    Copyright © 2003-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/* "muizunesupport.h" contains misc includes and
   init stuff which is not important at the moment. */

#include "muizunesupport.h"


/* Objects */

Object *app;
Object *WD_Main;
Object *TX_Info, *BT_Hello, *BT_Reset;


/****************************************************************
 Allocate resources for gui
*****************************************************************/

BOOL init_gui(void)
{
    app = ApplicationObject,
              MUIA_Application_Title      , (IPTR) "Notify",
              MUIA_Application_Version    , (IPTR) "$VER: HelloZune 0.1 (14.01.03)",
              MUIA_Application_Copyright  , (IPTR) "© 2003-2011, The AROS Development Team",
              MUIA_Application_Author     , (IPTR) "The AROS Development Team",
              MUIA_Application_Description, (IPTR) "How to use MUIM_Notify",
              MUIA_Application_Base       , (IPTR) "NOTIFY",

              /*
                 SimpleButton("Hello Notify") -> Set text to "Hello Notify! Press reset"
                 SimpleButton("Reset text")   -> Reset text back to "Please press 'Hello MUIM_Notify'"
              */

              SubWindow, WD_Main = WindowObject,
                  MUIA_Window_Title, (IPTR) "How to use MUIM_Notify",

                  WindowContents, VGroup,
                      Child, HGroup,
                          Child, TX_Info = SimpleText("Please press 'Hello Notify'"),
                      End, /* HGroup */

                      Child, HGroup,
                          Child, BT_Hello = SimpleButton("Hello Notify"),
                          Child, BT_Reset = SimpleButton("Reset text"),
                      End, /* HGroup */
                  End, /* VGroup */

              End, /* WindowObject */

          End; /* ApplicationObject */

    if(app)
    {
        /* Bring buttons to live with MUIM_Notify. */

        DoMethod(BT_Hello, MUIM_Notify,    /* Source object which triggers the notification.            */
                 MUIA_Pressed, FALSE,      /* Trigger if the button is released (MUIA_Pressed -> FALSE) */
                 TX_Info,                  /* Destination object to receive the notification.           */
                 3,                        /* Send three parameters.                                    */
                                           /* Set MUIA_Text_Contents to given string.                   */
                 MUIM_Set, MUIA_Text_Contents, (IPTR) "Hello Notify! Press reset");


        DoMethod(BT_Reset, MUIM_Notify,    /* Source object which triggers the notification.            */
                 MUIA_Pressed, FALSE,      /* Trigger if the button is released (MUIA_Pressed -> FALSE) */
                 TX_Info,                  /* Destination object to receive the notification.           */
                 3,                        /* Send three parameters.                                    */
                                           /* Set MUIA_Text_Contents to given string.                   */
                 MUIM_Set, MUIA_Text_Contents, (IPTR) "Please press 'Hello Notify'");


        /* Quit application if the windowclosegadget or the esc key is pressed. */

        DoMethod(WD_Main, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                 app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        return(TRUE);
    }

    return(FALSE);
} /* init_gui(void) */


/****************************************************************
 Deallocates all gui resources
*****************************************************************/

void deinit_gui(void)
{
    if(app){MUI_DisposeObject(app);}
} /* deinit_gui(void) */



/****************************************************************
 The message loop
*****************************************************************/

void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
    {
        if (sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
            if(sigs & SIGBREAKF_CTRL_C){break;}
            if(sigs & SIGBREAKF_CTRL_D){break;}
        }
    }
} /* loop(void) */


/****************************************************************
 The main entry point
*****************************************************************/

int main(int argc, char *argv[])
{
    if(open_libs())
    {
        if(init_gui())
        {
            set(WD_Main, MUIA_Window_Open, TRUE);

            if(xget(WD_Main, MUIA_Window_Open))
            {
                loop();
            }

            set(WD_Main, MUIA_Window_Open, FALSE);

            deinit_gui();
        }

        close_libs();
    }

    return RETURN_OK;
} /* main(int argc, char *argv[]) */
