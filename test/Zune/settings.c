/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <utility/hooks.h>

static void UseFunc(struct Hook *hook, Object *app, void *arg)
{
    DoMethod(app, MUIM_Application_Save, MUIV_Application_Save_ENV);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

static void SaveFunc(struct Hook *hook, Object *app, void *arg)
{
    DoMethod(app, MUIM_Application_Save, MUIV_Application_Save_ENV);
    DoMethod(app, MUIM_Application_Save, MUIV_Application_Save_ENVARC);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

int main(int argc,char *argv[])
{
    Object *app, *window, *bt_save, *bt_use, *bt_cancel, *checkbox;
    ULONG sigs;
    struct Hook saveHook, useHook;

    const char *sex[] = 
    {
	"male",
	"female",
	NULL
    };
    
    const char *hair_color[] =
    {
       "Blond",
       "Redhead",
       "Brunet",
       NULL
    };
    
    useHook.h_Entry = HookEntry;
    useHook.h_SubEntry = (HOOKFUNC) UseFunc;
    saveHook.h_Entry = HookEntry;
    saveHook.h_SubEntry = (HOOKFUNC) SaveFunc;
    
    app = ApplicationObject,
        MUIA_Application_Base, "SETTINGS",
        SubWindow, window = WindowObject,
            MUIA_Window_Title, "MUIM_Application_Load / MUIM_Application_Save test",
            WindowContents, VGroup,
                Child, ColGroup(2),
                    Child, Label2("Username:"),
                    Child, StringObject, StringFrame, MUIA_ObjectID, 1, MUIA_CycleChain, 1, End,
                    Child, Label1("Password:"),
                    Child, StringObject, StringFrame, MUIA_ObjectID, 4, MUIA_String_Secret, TRUE, MUIA_CycleChain, 1, End,
                    Child, Label1("Sex:"),
                    Child, CycleObject, MUIA_Cycle_Entries, sex, MUIA_ObjectID, 6, MUIA_CycleChain, 1, End,
                    Child, Label("Age:"),
                    Child, SliderObject, MUIA_ObjectID, 5, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 99, MUIA_CycleChain, 1, End,
                    Child, Label("Hair color:"),
                    Child, RadioObject, MUIA_ObjectID, 7, MUIA_Radio_Entries, hair_color, MUIA_CycleChain, 1, End,
                    Child, Label("AROS user:"),
                    Child, checkbox = MUI_MakeObject(MUIO_Checkmark, FALSE),
                    End,
                Child, HGroup,
                    Child, bt_save = SimpleButton("Save"),
                    Child, bt_use = SimpleButton("Use"),
                    Child, bt_cancel = SimpleButton("Cancel"),
                    End,
                End,
            End,
        End;

    if (!app)
        return 1;

    set(checkbox, MUIA_ObjectID, 8);
    
    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest,TRUE,
        app, (IPTR) 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
        app, (IPTR) 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(bt_save,MUIM_Notify,MUIA_Pressed,FALSE,
        app, (IPTR) 2,
        MUIM_CallHook, &saveHook, NULL);

    DoMethod(bt_use, MUIM_Notify, MUIA_Pressed, FALSE,
        app, (IPTR) 2,
        MUIM_CallHook, &useHook, NULL);

    DoMethod(app, MUIM_Application_Load, MUIV_Application_Load_ENV);

    set(window, MUIA_Window_Open, TRUE);

    /* Main loop */
    while((LONG)DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
          != MUIV_Application_ReturnID_Quit)
    {
        if (sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
        	break;
        }
    }

    set(window, MUIA_Window_Open, FALSE);

    MUI_DisposeObject(app);
    
    return 0;
}
