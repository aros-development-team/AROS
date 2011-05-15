/*
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/* "muizunesupport.h" contains misc includes and
   init stuff which is not important at the moment. */

#include "muizunesupport.h"


/* Objects */

Object *app;
Object *WD_Main;
Object *BT_One;
Object *BT_1, *BT_2, *BT_3;


/****************************************************************
 Allocalte resources for gui
*****************************************************************/

BOOL init_gui(void)
{
    app = ApplicationObject,
              MUIA_Application_Title      , (IPTR) "HVGroup",
              MUIA_Application_Version    , (IPTR) "$VER: HVGroup 0.1 (14.01.03)",
              MUIA_Application_Copyright  , (IPTR) "© 2003, The AROS Development Team",
              MUIA_Application_Author     , (IPTR) "The AROS Development Team",
              MUIA_Application_Description, (IPTR) "Layout with HGroup + VGroup",
              MUIA_Application_Base       , (IPTR) "HVGroup",

              SubWindow, WD_Main = WindowObject,
                  MUIA_Window_Title, (IPTR) "Layout with HGroup + VGroup",

                  /*
                     Start HGroup of window here. All other GUI objects are childs
                     of this HGroup.
                  */

                  WindowContents, HGroup,

                      /*
                          Layout: HGroup + VGroup for more complex layout.

                          One horizontal group with two vertical groups.

                          HGroup
                          |
                          +- VGroup
                          |
                          +- VGroup

                          To display one button on the left and three buttons
                          on the right.


                                       | Button 1 |
                          |    One  |  | Button 2 |
                                       | Button 3 |
                      */

                      /* One button on the left */

                      Child, VGroup,
                            Child, BT_One = SimpleButton("One"),
                      End, /* VGroup */


                      /* Three buttons on the right */

                      Child, VGroup,
                          Child, BT_1 = SimpleButton("Button 1"),
                          Child, BT_2 = SimpleButton("Button 2"),
                          Child, BT_3 = SimpleButton("Button 3"),
                      End, /* VGroup */

                  End, /* HGroup */

              End, /* WindowObject */

          End; /* ApplicationObject */

    if(app)
    {
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
} /* loop(void)*/


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
} /* main(int argc, char *argv[]) */
